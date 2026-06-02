/* SPDX-License-Identifier: GPL-2.0-only */

/* TODO: See what can be made common */

/* ACPI - create the Fixed ACPI Description Tables (FADT) */

#include <acpi/acpi.h>
#include <acpi/acpi_ivrs.h>
#include <acpi/acpigen.h>
#include <amdblocks/acpi.h>
#include <amdblocks/cppc.h>
#include <amdblocks/cpu.h>
#include <amdblocks/acpimmio.h>
#include <amdblocks/ioapic.h>
#include <amdblocks/psp.h>
#include <arch/ioapic.h>
#include <arch/smp/mpspec.h>
#include <cbmem.h>
#include <console/console.h>
#include <cpu/amd/cpuid.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <drivers/amd/opensil/opensil.h>
#include <soc/amd/common/block/psp/psp_def.h>
#include <soc/iomap.h>
#include <static.h>
#include <types.h>
#include "chip.h"

#if CONFIG(SOC_AMD_PHOENIX_OPENSIL)
#include <xPRF-api.h>
#endif

unsigned long soc_acpi_fill_ivrs40(unsigned long current, acpi_ivrs_ivhd40_t *ivhd,
				   struct device *nb_dev, struct device *iommu_dev)
{
	/* Describe UART devices */
	current = ivhd_describe_f0_device(current, PCI_DEVFN(0x14, 5),
				"AMDI0020", IVHD_DTE_LINT_0_PASS, 0);
	current = ivhd_describe_f0_device(current, PCI_DEVFN(0x14, 5),
				"AMDI0020", IVHD_DTE_LINT_0_PASS, 1);
	current = ivhd_describe_f0_device(current, PCI_DEVFN(0x14, 5),
				"AMDI0020", IVHD_DTE_LINT_0_PASS, 2);
	current = ivhd_describe_f0_device(current, PCI_DEVFN(0x14, 5),
				"AMDI0020", IVHD_DTE_LINT_0_PASS, 3);

	/* TODO: HSP if used
	 * current = ivhd_describe_f0_device(current, 0xfffe,
	 *			"MSFT0201", IVHD_DTE_LINT_0_PASS, 0);
	 */

	return current;
}

/*
 * Reference section 5.2.9 Fixed ACPI Description Table (FADT)
 * in the ACPI 3.0b specification.
 */
void acpi_fill_fadt(acpi_fadt_t *fadt)
{
	const struct soc_amd_phoenix_config *cfg = config_of_soc();

	if (CONFIG(PLATFORM_USES_FSP2_0)) {
		printk(BIOS_DEBUG, "pm_base: 0x%04x\n", ACPI_IO_BASE);

		fadt->pm1a_evt_blk = ACPI_PM_EVT_BLK;
		fadt->pm1a_cnt_blk = ACPI_PM1_CNT_BLK;
		fadt->pm_tmr_blk = ACPI_PM_TMR_BLK;
		fadt->gpe0_blk = ACPI_GPE0_BLK;
	} else {
		/* Fill in pm1_evt, pm1_cnt, pm_tmr, gpe0_blk from openSIL input structure */
		amd_opensil_fill_fadt_io_ports(fadt);
	}

	fadt->pm1_evt_len = 4;	/* 32 bits */
	fadt->pm1_cnt_len = 2;	/* 16 bits */
	fadt->pm_tmr_len = 4;	/* 32 bits */
	fadt->gpe0_blk_len = 8;	/* 64 bits */

	fill_fadt_extended_pm_io(fadt);

	fadt->iapc_boot_arch = cfg->common_config.fadt_boot_arch; /* legacy free default */
	fadt->flags |=	ACPI_FADT_WBINVD | /* See table 5-34 ACPI 6.3 spec */
			ACPI_FADT_C1_SUPPORTED |
			ACPI_FADT_S4_RTC_WAKE |
			ACPI_FADT_32BIT_TIMER |
			ACPI_FADT_PCI_EXPRESS_WAKE |
			ACPI_FADT_PLATFORM_CLOCK |
			ACPI_FADT_S4_RTC_VALID |
			ACPI_FADT_REMOTE_POWER_ON;
	if (cfg->s0ix_enable)
		fadt->flags |= ACPI_FADT_LOW_PWR_IDLE_S0;

	fadt->flags |= cfg->common_config.fadt_flags; /* additional board-specific flags */
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

	printk(BIOS_DEBUG, "PSP: Sending IVRS ACPI table ");

	cmd_status = send_psp_command(MBOX_BIOS_CMD_SEND_IVRS_ACPI_TABLE, &buffer);

	/* buffer's status shouldn't change but report it if it does */
	psp_print_cmd_status(cmd_status, &buffer.header);
}

unsigned long soc_acpi_write_tables(const struct device *device, unsigned long current,
				    acpi_rsdp_t *rsdp)
{
	/* IVRS */
	current = acpi_add_ivrs_table(current, rsdp);

	send_ivrs_to_psp(rsdp);

	if (CONFIG(PLATFORM_USES_FSP2_0))
		current = acpi_add_fsp_tables(current, rsdp);
	else
		current = acpi_add_opensil_tables(current, rsdp);

	return current;
}

const acpi_cstate_t cstate_cfg_table[] = {
	[0] = {
		.ctype = 1,
		.latency = 1,
		.power = 0,
	},
	[1] = {
		.ctype = 2,
		.latency = 0x12,
		.power = 0,
	},
	[2] = {
		.ctype = 3,
		.latency = 350,
		.power = 0,
	},
};

const acpi_cstate_t *get_cstate_config_data(size_t *size)
{
	*size = ARRAY_SIZE(cstate_cfg_table);
	return cstate_cfg_table;
}

#if CONFIG(SOC_AMD_PHOENIX_OPENSIL)
enum cb_err get_ccx_cppc_min_frequency(uint32_t *freq)
{
	SIL_CONTEXT SilContext = {
		.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS,
		.SilMemBaseAddress = (uintptr_t)cbmem_find(CBMEM_ID_AMD_OPENSIL)
	};

	if (SilContext.SilMemBaseAddress == 0)
		return CB_ERR;

	if (xPrfGetCppcMinFrequency(&SilContext, freq) != SilPass)
		return CB_ERR;

	return CB_SUCCESS;
}

enum cb_err get_ccx_cppc_nom_frequency(uint32_t *freq)
{
	SIL_CONTEXT SilContext = {
		.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS,
		.SilMemBaseAddress = (uintptr_t)cbmem_find(CBMEM_ID_AMD_OPENSIL)
	};

	if (SilContext.SilMemBaseAddress == 0)
		return CB_ERR;

	if (xPrfGetCppcNomFrequency(&SilContext, freq) != SilPass)
		return CB_ERR;

	return CB_SUCCESS;
}
#endif
