/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <arch/bootblock.h>
#include <console/console.h>
#include <cpu/x86/bist.h>
#include <cpu/x86/lapic.h>
#include <cpu/amd/msr.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <program_loading.h>
#include <southbridge/amd/sb700/sb700.h>
#include <smp/node.h>

static u32 saved_bist;

static void enable_pci_mmconf(void)
{
	msr_t mmconf;

	mmconf.hi = 0;
	mmconf.lo = CONFIG_MMCONF_BASE_ADDRESS | MMIO_RANGE_EN
			| fms(CONFIG_MMCONF_BUS_NUMBER) << MMIO_BUS_RANGE_SHIFT;
	wrmsr(MMIO_CONF_BASE, mmconf);
}

static void *get_ap_entry_ptr(void)
{
	u32 entry;
	load_bios_ram_data(&entry, 4, BIOSRAM_AP_ENTRY);
	return (void *)entry;
}

asmlinkage void bootblock_c_entry_bist(u64 base_timestamp, u32 bist)
{
	saved_bist = bist;

	enable_pci_mmconf();

	if (CONFIG(UDELAY_LAPIC))
		enable_lapic();

	if (!boot_cpu()) {
		void (*ap_romstage_entry)(void) = get_ap_entry_ptr();
		ap_romstage_entry(); /* execution does not return */
		halt();
	}

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
