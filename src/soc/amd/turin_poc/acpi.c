/* SPDX-License-Identifier: GPL-2.0-only */

/* ACPI - create the Fixed ACPI Description Tables (FADT) */

#include <acpi/acpi.h>
#include <acpi/acpi_ivrs.h>
#include <amdblocks/acpi.h>
#include <amdblocks/acpimmio.h>
#include <amdblocks/cpu.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <drivers/amd/opensil/opensil.h>

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

static unsigned long ivhd_describe_f0_device(unsigned long current, uint16_t dev_id,
					     const char acpi_hid[8], uint8_t datasetting,
					     uint64_t uid)
{
	ivrs_ivhd_f0_entry_t *ivhd_f0 = (ivrs_ivhd_f0_entry_t *)current;
	memset(ivhd_f0, 0, sizeof(*ivhd_f0));

	ivhd_f0->type = IVHD_DEV_VARIABLE;
	ivhd_f0->dev_id = dev_id;
	ivhd_f0->dte_setting = datasetting;
	ivhd_f0->uuid_length = sizeof(uid);
	ivhd_f0->uuid_format = IVHD_UID_INT;
	memcpy(&ivhd_f0[1], &uid, sizeof(uid));
	memcpy(ivhd_f0->hardware_id, acpi_hid, sizeof(ivhd_f0->hardware_id));

	current += (sizeof(ivrs_ivhd_f0_entry_t) + ivhd_f0->uuid_length);
	return current;
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
	/* Fill in pm1_evt, pm1_cnt, pm_tmr, gpe0_blk from openSIL input structure */
	amd_opensil_fill_fadt_io_ports(fadt);

	fadt->pm1_evt_len = 4;	/* 32 bits */
	fadt->pm1_cnt_len = 2;	/* 16 bits */
	fadt->pm_tmr_len = 4;	/* 32 bits */
	fadt->gpe0_blk_len = 8;	/* 64 bits */

	fill_fadt_extended_pm_io(fadt);

	/* Below values as per doc #58088 */
	fadt->iapc_boot_arch = ACPI_FADT_LEGACY_FREE; /* legacy free default */
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

	fadt->FADT_MinorVersion = 5;

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

unsigned long soc_acpi_write_tables(const struct device *device, unsigned long current,
				    struct acpi_rsdp *rsdp)
{
	acpi_slit_t *slit;

	/* IVRS */
	printk(BIOS_DEBUG, "ACPI:   * IVRS\n");
	current = acpi_add_ivrs_table(current, rsdp);

	/* SLIT */
	current = ALIGN_UP(current, 8);
	printk(BIOS_DEBUG, "ACPI:   * SLIT at %lx\n", current);
	slit = (acpi_slit_t *)current;
	acpi_create_slit(slit, acpi_fill_slit);
	current += slit->header.length;
	acpi_add_table(rsdp, slit);

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
