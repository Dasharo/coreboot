/* SPDX-License-Identifier: GPL-2.0-only */

/* ACPI - create the Fixed ACPI Description Tables (FADT) */

#include <acpi/acpi.h>
#include <acpi/acpi_ivrs.h>
#include <amdblocks/acpi.h>
#include <amdblocks/acpimmio.h>
#include <amdblocks/chip.h>
#include <amdblocks/cppc.h>
#include <amdblocks/cpu.h>
#include <amdblocks/psp.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <drivers/amd/opensil/opensil.h>
#include <soc/amd/common/block/psp/psp_def.h>
#include <xPRF-api.h>

#define IOMMU_DOMAIN_INIT(d)	\
	{	\
		.iommu_domain = (d),	\
		.num_covered_domains = ARRAY_SIZE(iommu##d##_domains),	\
		.covered_domain_ids = iommu##d##_domains	\
	}	\

static const unsigned int iommu0_domains[] = { 0, 3 };
static const unsigned int iommu2_domains[] = { 2, 1 };
static const unsigned int iommu5_domains[] = { 5, 6 };
static const unsigned int iommu7_domains[] = { 7, 4 };

static const struct ivrs_iommu_domain ivrs_iommu_domains[] = {
	IOMMU_DOMAIN_INIT(0),
	IOMMU_DOMAIN_INIT(2),
	IOMMU_DOMAIN_INIT(5),
	IOMMU_DOMAIN_INIT(7)
};

unsigned int acpi_ivrs_get_iommu_domains(const struct ivrs_iommu_domain **iommu_domains)
{
	*iommu_domains = ivrs_iommu_domains;
	return ARRAY_SIZE(ivrs_iommu_domains);
}

unsigned long soc_acpi_fill_ivrs40(unsigned long current, acpi_ivrs_ivhd40_t *ivhd,
				   struct device *nb_dev, struct device *iommu_dev)
{
	unsigned long domain = dev_get_domain_id(nb_dev);

	/* Describe UART devices */
	if (domain == 0) {
		current = ivhd_describe_f0_device(current, PCI_DEVFN(0x14, 5),
					"AMDI0020", IVHD_DTE_LINT_0_PASS, 0);
		current = ivhd_describe_f0_device(current, PCI_DEVFN(0x14, 5),
					"AMDI0020", IVHD_DTE_LINT_0_PASS, 1);
		current = ivhd_describe_f0_device(current, PCI_DEVFN(0x14, 5),
					"AMDI0020", IVHD_DTE_LINT_0_PASS, 2);
		current = ivhd_describe_f0_device(current, PCI_DEVFN(0x14, 5),
					"AMDI0020", IVHD_DTE_LINT_0_PASS, 3);
	}

	/* MPDMA devices */
	if (domain == 0 || domain == 7) {
		current = ivhd_describe_f0_device(current,
				(nb_dev->upstream->secondary << 8) | PCI_DEVFN(0, 4),
				domain == 0 ? "AMDI0095" : "AMDI0096",
				IVHD_DTE_SYS_MGT_TRANS,
				domain == 0 ? 0 : 1);
	}

	return current;
}

void acpi_fill_fadt(acpi_fadt_t *fadt)
{
	const struct soc_amd_common_config *cfg = soc_get_common_config();

	/* Fill in pm1_evt, pm1_cnt, pm_tmr, gpe0_blk from openSIL input structure */
	amd_opensil_fill_fadt_io_ports(fadt);

	fadt->pm1_evt_len = 4;	/* 32 bits */
	fadt->pm1_cnt_len = 2;	/* 16 bits */
	fadt->pm_tmr_len = 4;	/* 32 bits */
	fadt->gpe0_blk_len = 8;	/* 64 bits */

	fill_fadt_extended_pm_io(fadt);

	fadt->iapc_boot_arch = cfg->fadt_boot_arch;
	fadt->flags |= cfg->fadt_flags; /* additional board-specific flags */

	/* Below values as per doc #58088 */
	fadt->flags |=	ACPI_FADT_WBINVD | /* See table 5-34 ACPI 6.3 spec */
			ACPI_FADT_C1_SUPPORTED |
			ACPI_FADT_C2_MP_SUPPORTED |
			ACPI_FADT_SLEEP_BUTTON |
			ACPI_FADT_32BIT_TIMER |
			ACPI_FADT_RESET_REGISTER |
			ACPI_FADT_REMOTE_POWER_ON;


	fadt->x_firmware_ctl_l = 0;	/* set to 0 if firmware_ctrl is used */
	fadt->x_firmware_ctl_h = 0;

	fadt->p_lvl2_lat = 0x64;

	fadt->duty_offset = 1;
	fadt->duty_width = 3;

	fadt->flush_size = 0x400;
	fadt->flush_stride = 0x10;

	fadt->preferred_pm_profile = PM_ENTERPRISE_SERVER;
}

static unsigned long acpi_fill_slit(unsigned long current)
{
	*(uint64_t *)current = 1; /* 1 locality */
	current += sizeof(uint64_t);
	*(uint8_t *)current = 10;
	current += sizeof(uint8_t);

	return current;
}

static void send_ivrs_to_psp(struct acpi_rsdp *rsdp)
{
	acpi_xsdt_t *xsdt = (acpi_xsdt_t *)(uintptr_t)rsdp->xsdt_address;
	size_t entries_num = ARRAY_SIZE(xsdt->entry);
	struct acpi_table_header *hdr;
	bool found = false;
	size_t i;
	int cmd_status;
	struct mbox_cmd_ivrs_acpi_table_info buffer;

	/* Locate IVRS in XSDT to get its address */
	for (i = 0; i < entries_num; i++) {
		hdr = (struct acpi_table_header *)xsdt->entry[i];
		if (xsdt->entry[i] == 0)
			return;

		if (strncmp(hdr->signature, "IVRS", 4)) {
			found = true;
			break;
		}
	}

	if (!found)
		return;

	buffer.header.size = sizeof(buffer);
	buffer.info.ivrs_table_buffer = (uint64_t)hdr;
	buffer.info.ivrs_table_size = hdr->length;

	printk(BIOS_DEBUG, "PSP: Sending IVRS ACPI table\n");

	cmd_status = send_psp_command(MBOX_BIOS_CMD_SEND_IVRS_ACPI_TABLE, &buffer);

	/* buffer's status shouldn't change but report it if it does */
	psp_print_cmd_status(cmd_status, &buffer.header);
}

static unsigned long acpi_fill_aspt(unsigned long current)
{
	aspt_global_regs_t *global_regs;
	aspt_sev_mbox_regs_t *sev_mbox_regs;
	aspt_acpi_mbox_regs_t *acpi_mbox_regs;
	acpi_aspt_t *aspt = (acpi_aspt_t *)current;
	const uint64_t psp_mmio = get_psp_mmio_base();

	if (!psp_mmio) {
		printk(BIOS_WARNING, "PSP: PSP MMIO not allocated\n");
		return current;
	}

	/*
	 * The mailboxes are located at offsets 0x10xxx. To fit everything
	 * in one page, add 64K to the MMIO base and adjust the C2P/P2C
	 * offsets by subtracting 64K.
	 */
	aspt->asp_base_address = psp_mmio + 64 * KiB;
	aspt->asp_space_pages = 1;
	aspt->asp_structure_count = 3;

	/* ASP global registers */
	current = (unsigned long)&aspt->asp_structures;
	global_regs = (aspt_global_regs_t *)current;
	memset((void *)global_regs, 0, sizeof(aspt_global_regs_t));

	global_regs->type = ASPT_STRUCTURE_TYPE_ASP_GLOBAL_REGISTERS;
	global_regs->length = sizeof(aspt_global_regs_t);
	global_regs->feature_reg_offset = 0x9fc;
	global_regs->int_enable_reg_offset = 0x510;
	global_regs->int_status_reg_offset = 0x514;

	current += global_regs->length;

	/* SEV mailbox registers */
	sev_mbox_regs = (aspt_sev_mbox_regs_t *)current;
	memset((void *)sev_mbox_regs, 0, sizeof(aspt_sev_mbox_regs_t));

	sev_mbox_regs->type = ASPT_STRUCTURE_TYPE_SEV_MAILBOX_REGISTERS;
	sev_mbox_regs->length = sizeof(aspt_sev_mbox_regs_t);
	sev_mbox_regs->mailbox_innterrupt_id = 1;
	sev_mbox_regs->cmd_respRegisterOffset = 0x980;
	sev_mbox_regs->cmd_buf_addr_lo_offset = 0x9e0;
	sev_mbox_regs->cmd_buf_addr_hi_offset = 0x9e4;

	current += sev_mbox_regs->length;

	/* ACPI mailbox registers */
	acpi_mbox_regs = (aspt_acpi_mbox_regs_t *)current;
	memset((void *)acpi_mbox_regs, 0, sizeof(aspt_acpi_mbox_regs_t));

	acpi_mbox_regs->type = ASPT_STRUCTURE_TYPE_ACPI_MAILBOX_REGISTERS;
	acpi_mbox_regs->length = sizeof(aspt_acpi_mbox_regs_t);
	acpi_mbox_regs->cmd_resp_reg_offset = 0x958;

	current += acpi_mbox_regs->length;

	return current;
}

unsigned long soc_acpi_write_tables(const struct device *device, unsigned long current,
				    struct acpi_rsdp *rsdp)
{
	acpi_slit_t *slit;
	acpi_aspt_t *aspt;

	/* IVRS */
	printk(BIOS_DEBUG, "ACPI:   * IVRS\n");
	current = acpi_add_ivrs_table(current, rsdp);
	send_ivrs_to_psp(rsdp);

	/* SLIT */
	current = ALIGN_UP(current, 8);
	printk(BIOS_DEBUG, "ACPI:   * SLIT at %lx\n", current);
	slit = (acpi_slit_t *)current;
	acpi_create_slit(slit, acpi_fill_slit);
	current += slit->header.length;
	acpi_add_table(rsdp, slit);

	/* ASPT */
	current = ALIGN_UP(current, 8);
	printk(BIOS_DEBUG, "ACPI:   * ASPT at %lx\n", current);
	aspt = (acpi_aspt_t *)current;
	acpi_create_aspt(aspt, acpi_fill_aspt);
	current += aspt->header.length;
	acpi_add_table(rsdp, aspt);

	return current;
}

/* There are only the following 2 C-states reported by the reference firmware */
const acpi_cstate_t cstate_cfg_table[] = {
	[0] = {
		.ctype = 1,
		.latency = 1,
		.power = 0,
	},
	[1] = {
		.ctype = 2,
		.latency = 0x64,
		.power = 0,
	},
};

const acpi_cstate_t *get_cstate_config_data(size_t *size)
{
	*size = ARRAY_SIZE(cstate_cfg_table);
	return cstate_cfg_table;
}

const struct acpi_lpi_state *get_cstate_lpi_config_data(size_t *size)
{
	*size = 0;
	return NULL;
}

enum cb_err get_ccx_cppc_min_frequency(uint32_t *freq)
{
	if (xPrfGetCppcMinFrequency(freq) != SilPass)
		return CB_ERR;

	return CB_SUCCESS;
}

enum cb_err get_ccx_cppc_nom_frequency(uint32_t *freq)
{
	if (xPrfGetCppcNomFrequency(freq) != SilPass)
		return CB_ERR;

	return CB_SUCCESS;
}
