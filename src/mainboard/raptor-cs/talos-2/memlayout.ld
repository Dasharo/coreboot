/* SPDX-License-Identifier: GPL-2.0-only */

#include <memlayout.h>

#include <arch/header.ld>

SECTIONS
{
	// On POWER, 0 is wherever HRMOR points to rather than physical DRAM start
	DRAM_START(0x0)

#if !CONFIG(BOOTBLOCK_IN_SEEPROM)
	BOOTBLOCK(0, 32K)
#endif

	STACK(0x8000, 32K)
	PRERAM_CBMEM_CONSOLE(0x10000, 8K)
	FMAP_CACHE(0x12000, 2K)
	CBFS_MCACHE(0x13000, 8K)
	ROMSTAGE(0x20000, 1M)

#if CONFIG(BOOTBLOCK_IN_SEEPROM)
	BOOTBLOCK(0x203000, 20K)
#endif

	// TODO: maybe we should change HRMOR again before jumping into ramstage?
	// In that case the address may be overlapping previous stages because it
	// would be in different address space, but current macros forbid it.
	// On the other hand, the value specified below will work regardless of
	// whether we change HRMOR or not. There may still be issues with mapping
	// other components that should be accessible both by ramstage and previous
	// stages (pre-RAM console, FMAP and CBFS cache), but these could be copied
	// by postcar.
	//
	// Pros of changing HRMOR:
	// - can access more than 128MB of RAM without setting "ignore HRMOR" bit
	// - real address is equal to physical address
	//   - no need for calculations
	//   - pointers can be passed as they are to the payload
	//
	// Cons:
	// - a bit more work in postcar is required
	// - need to be compatible with payload's expectations

	// It doesn't work if anything is above 4M, HBBL doesn't 'dcbz' it.
	CBFS_CACHE(0x280000, 512K)
	RAMSTAGE(0x300000, 5M)
}
