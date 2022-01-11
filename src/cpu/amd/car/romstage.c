/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/romstage.h>
#include <console/console.h>
#include <timestamp.h>

void __weak mainboard_romstage_entry(void) { }

asmlinkage void car_stage_entry(void)
{
	u32 stack_pointer = 0;
	timestamp_add_now(TS_START_ROMSTAGE);

	/* Assumes the hardware was set up during the bootblock */
	console_init();

	asm volatile (
		"movl %%esp, %0"
		: "=r" (stack_pointer)
		);

	printk(BIOS_DEBUG, "Initial stack pointer: %08x\n", stack_pointer);

	mainboard_romstage_entry();
	/* We do not return here. */
}
