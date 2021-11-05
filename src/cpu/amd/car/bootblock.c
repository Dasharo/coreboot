/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <arch/bootblock.h>
#include <cpu/x86/bist.h>
#include <cpu/x86/lapic.h>
#include <cpu/amd/msr.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>

static uint32_t saved_bist;

static void enable_pci_mmconf(void)
{
	msr_t mmconf;

	mmconf.hi = 0;
	mmconf.lo = CONFIG_MMCONF_BASE_ADDRESS | MMIO_RANGE_EN
			| fms(CONFIG_MMCONF_BUS_NUMBER) << MMIO_BUS_RANGE_SHIFT;
	wrmsr(MMIO_CONF_BASE, mmconf);
}

asmlinkage void bootblock_c_entry_bist(uint64_t base_timestamp, uint32_t bist)
{
	saved_bist = bist;

	enable_pci_mmconf();

	if (CONFIG(UDELAY_LAPIC))
		enable_lapic();

	/* Call lib/bootblock.c main */
	bootblock_main_with_basetime(base_timestamp);
}

void __weak bootblock_early_northbridge_init(void) { }
void __weak bootblock_early_southbridge_init(void) { }
void __weak bootblock_early_cpu_init(void) { }

void bootblock_soc_early_init(void)
{
	bootblock_early_northbridge_init();
	bootblock_early_southbridge_init();
	bootblock_early_cpu_init();
}

void bootblock_soc_init(void)
{
	/* Halt if there was a built in self test failure */
	report_bist_failure(saved_bist);
}
