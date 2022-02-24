/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * This file contains entry/exit functions for each stage during coreboot
 * execution (bootblock entry and ramstage exit will depend on external
 * loading).
 *
 * Entry points must be placed at the location the previous stage jumps
 * to (the lowest address in the stage image). This is done by giving
 * stage_entry() its own section in .text and placing it first in the
 * linker script.
 */

#include <cbmem.h>
#include <arch/stages.h>
#include <cpu/power/spr.h>
#include <timestamp.h>

void stage_entry(uintptr_t stage_arg)
{
	if (!ENV_ROMSTAGE_OR_BEFORE) {
		/*
		 * This works if:
		 * - this stage haven't use stack so far
		 * - this function (stage_entry) doesn't use a stack - preparing a stack
		 *   frame for main() doesn't count
		 * - main() doesn't take arguments on stack
		 * - main() doesn't return
		 * - stage is linked at proper address that includes HRMOR so we don't
		 *   have to play with "ignore HRMOR" bit
		 *
		 * Also, 'sync' is for changing HRMOR, not stack.
		 */
		register uint64_t hrmor = read_spr(SPR_HRMOR);
		asm volatile("sync; isync" ::: "memory");
		write_spr(SPR_HRMOR, 0);
		asm volatile("or 1,1,%0; slbia 7; sync; isync" :: "r"(hrmor) : "memory");

		_cbmem_top_ptr = stage_arg;
	}
	else {
		timestamp_init(read_spr(SPR_TB));
	}

	main();
}
