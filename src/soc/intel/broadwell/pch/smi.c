/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <device/pci.h>
#include <console/console.h>
#include <arch/io.h>
#include <cpu/x86/smm.h>
#include <cpu/intel/smm_reloc.h>
#include <soc/iomap.h>
#include <soc/pch.h>
#include <soc/pm.h>

void smm_southbridge_clear_state(void)
{
	u32 smi_en;

	printk(BIOS_DEBUG, "Initializing Southbridge SMI...");
	printk(BIOS_SPEW, " ... pmbase = 0x%04x\n", ACPI_BASE_ADDRESS);

	smi_en = inl(ACPI_BASE_ADDRESS + SMI_EN);
	if (smi_en & APMC_EN) {
		printk(BIOS_INFO, "SMI# handler already enabled?\n");
		return;
	}

	printk(BIOS_DEBUG, "\n");

	/* Dump and clear status registers */
	clear_smi_status();
	clear_pm1_status();
	clear_tco_status();
	clear_gpe_status();
}

static void smm_southbridge_enable(uint16_t pm1_events)
{
	printk(BIOS_DEBUG, "Enabling SMIs.\n");
	/* Configure events */
	enable_pm1(pm1_events);
	disable_gpe(PME_B0_EN);

	/* Enable SMI generation:
	 *  - on APMC writes (io 0xb2)
	 *  - on writes to SLP_EN (sleep states)
	 *  - on writes to GBL_RLS (bios commands)
	 * No SMIs:
	 *  - on microcontroller writes (io 0x62/0x66)
	 *  - on TCO events
	 */
	enable_smi(APMC_EN | SLP_SMI_EN | GBL_SMI_EN | EOS);
}

void global_smi_enable(void)
{
	smm_southbridge_enable(PWRBTN_EN | GBL_EN);
}
