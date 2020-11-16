/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <halt.h>
#include <string.h>
#include <acpi/acpi.h>
#include <amdblocks/amd_pci_mmconf.h>
#include <arch/bootblock.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/lapic.h>
#include <northbridge/amd/agesa/agesa_helper.h>
#include <northbridge/amd/agesa/state_machine.h>

#define EARLY_VMTRR_FLASH 6

void __weak board_BeforeAgesa(struct sysinfo *cb) { }

static void set_early_mtrrs(void)
{
	/* Cache the ROM to speed up booting */
	set_var_mtrr(EARLY_VMTRR_FLASH, OPTIMAL_CACHE_ROM_BASE,
		     OPTIMAL_CACHE_ROM_SIZE, MTRR_TYPE_WRPROT);
}

void bootblock_soc_early_init(void)
{
	bootblock_early_southbridge_init();
}

static void fill_sysinfo(struct sysinfo *cb)
{
	memset(cb, 0, sizeof(*cb));
	cb->s3resume = acpi_is_wakeup_s3();

	agesa_set_interface(cb);
}

void bootblock_soc_init(void)
{
	struct sysinfo bootblock_state;
	struct sysinfo *cb = &bootblock_state;
	u8 initial_apic_id = (u8) (cpuid_ebx(1) >> 24);

	fill_sysinfo(cb);

	if (initial_apic_id == 0) {
		board_BeforeAgesa(cb);
		console_init();
	}

	printk(BIOS_DEBUG, "APIC %02d: CPU Family_Model = %08x\n",
		initial_apic_id, cpuid_eax(1));

	agesa_execute_state(cb, AMD_INIT_RESET);

	agesa_execute_state(cb, AMD_INIT_EARLY);

	/* APs shall not get here */
	if (initial_apic_id != 0)
		halt();
}

asmlinkage void bootblock_c_entry(uint64_t base_timestamp)
{
	enable_pci_mmconf();
	set_early_mtrrs();

	if (CONFIG(UDELAY_LAPIC))
		enable_lapic();

	bootblock_main_with_basetime(base_timestamp);
}

asmlinkage void ap_bootblock_c_entry(void)
{
	enable_pci_mmconf();
	set_early_mtrrs();

	if (CONFIG(UDELAY_LAPIC))
		enable_lapic();

	bootblock_soc_init();
}
