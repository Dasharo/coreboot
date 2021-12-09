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
#include <fit.h>
#include <bootmem.h>

void stage_entry(uintptr_t stage_arg)
{
	if (!ENV_ROMSTAGE_OR_BEFORE)
		_cbmem_top_ptr = stage_arg;
	else
		timestamp_init(read_spr(SPR_TB));

	main();
}

bool fit_payload_arch(struct prog *payload, struct fit_config_node *config,
		      struct region *kernel,
		      struct region *fdt,
		      struct region *initrd)
{
	void *arg = NULL;

	/* Mark as reserved for future allocations. */
	bootmem_add_range(kernel->offset, kernel->size, BM_MEM_PAYLOAD);

	/* Place FDT */
	/* TODO: remove hardcoded size */
	fdt->offset = 0xf0000000;
	fdt->size = 318539;

	/* Mark as reserved for future allocations. */
	bootmem_add_range(fdt->offset, fdt->size, BM_MEM_PAYLOAD);

	/* Kernel expects FDT as argument */
	arg = (void *)fdt->offset;

	prog_set_entry(payload, (void *)0x10 /* kernel->offset + 0x10 ? */, arg);

	bootmem_dump_ranges();

	return true;
}
