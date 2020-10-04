/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/pci_ops.h>
#include <acpi/acpi.h>
#include <cpu/x86/smm.h>
#include <version.h>
#include <southbridge/intel/common/pmbase.h>
#include "i82801gx.h"
#include "chip.h"

void acpi_fill_fadt(acpi_fadt_t *fadt)
{
	struct device *dev = pcidev_on_root(0x1f, 0);
	struct southbridge_intel_i82801gx_config *chip = dev->chip_info;
	u16 pmbase = lpc_get_pmbase();

	fadt->sci_int = 0x9;

	if (permanent_smi_handler()) {
		fadt->smi_cmd = APM_CNT;
		fadt->acpi_enable = APM_CNT_ACPI_ENABLE;
		fadt->acpi_disable = APM_CNT_ACPI_DISABLE;
		fadt->pstate_cnt = APM_CNT_PST_CONTROL;
		fadt->cst_cnt = APM_CNT_CST_CONTROL;
	}

	fadt->pm1a_evt_blk = pmbase;
	fadt->pm1a_cnt_blk = pmbase + PM1_CNT;
	fadt->pm2_cnt_blk = pmbase + PM2_CNT;
	fadt->pm_tmr_blk = pmbase + PM1_TMR;
	fadt->gpe0_blk = pmbase + GPE0_STS;

	fadt->pm1_evt_len = 4;
	fadt->pm1_cnt_len = 2; /* Upper word is reserved and
				  Linux complains about 32 bit. */
	fadt->pm2_cnt_len = 1;
	fadt->pm_tmr_len = 4;
	fadt->gpe0_blk_len = 8;
	fadt->p_lvl2_lat = 1;
	fadt->p_lvl3_lat = chip->c3_latency;
	fadt->duty_offset = 1;
	if (chip->p_cnt_throttling_supported)
		fadt->duty_width = 3;
	else
		fadt->duty_width = 0;
	fadt->day_alrm = 0xd;
	fadt->mon_alrm = 0x00;
	fadt->century = 0x32;
	fadt->iapc_boot_arch = ACPI_FADT_8042 | ACPI_FADT_LEGACY_DEVICES;
	fadt->flags |= (ACPI_FADT_WBINVD | ACPI_FADT_C1_SUPPORTED
		       | ACPI_FADT_SLEEP_BUTTON | ACPI_FADT_S4_RTC_WAKE
		       | ACPI_FADT_PLATFORM_CLOCK | ACPI_FADT_C2_MP_SUPPORTED);
	if (chip->docking_supported)
		fadt->flags |= ACPI_FADT_DOCKING_SUPPORTED;

	fadt->x_pm1a_evt_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_pm1a_evt_blk.bit_width = 32;
	fadt->x_pm1a_evt_blk.bit_offset = 0;
	fadt->x_pm1a_evt_blk.access_size = ACPI_ACCESS_SIZE_WORD_ACCESS;
	fadt->x_pm1a_evt_blk.addrl = pmbase;
	fadt->x_pm1a_evt_blk.addrh = 0x0;

	fadt->x_pm1a_cnt_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_pm1a_cnt_blk.bit_width = 16; /* Upper word is reserved and
						Linux complains about 32 bit. */
	fadt->x_pm1a_cnt_blk.bit_offset = 0;
	fadt->x_pm1a_cnt_blk.access_size = ACPI_ACCESS_SIZE_WORD_ACCESS;
	fadt->x_pm1a_cnt_blk.addrl = pmbase + PM1_CNT;
	fadt->x_pm1a_cnt_blk.addrh = 0x0;

	fadt->x_pm2_cnt_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_pm2_cnt_blk.bit_width = 8;
	fadt->x_pm2_cnt_blk.bit_offset = 0;
	fadt->x_pm2_cnt_blk.access_size = ACPI_ACCESS_SIZE_BYTE_ACCESS;
	fadt->x_pm2_cnt_blk.addrl = pmbase + PM2_CNT;
	fadt->x_pm2_cnt_blk.addrh = 0x0;

	fadt->x_pm_tmr_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_pm_tmr_blk.bit_width = 32;
	fadt->x_pm_tmr_blk.bit_offset = 0;
	fadt->x_pm_tmr_blk.access_size = ACPI_ACCESS_SIZE_DWORD_ACCESS;
	fadt->x_pm_tmr_blk.addrl = pmbase + PM1_TMR;
	fadt->x_pm_tmr_blk.addrh = 0x0;

	fadt->x_gpe0_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_gpe0_blk.bit_width = 64;
	fadt->x_gpe0_blk.bit_offset = 0;
	fadt->x_gpe0_blk.access_size = ACPI_ACCESS_SIZE_BYTE_ACCESS;
	fadt->x_gpe0_blk.addrl = pmbase + GPE0_STS;
	fadt->x_gpe0_blk.addrh = 0x0;
}
