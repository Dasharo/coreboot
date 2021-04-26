/* SPDX-License-Identifier: GPL-2.0-only */

#include <memlayout.h>

#include <arch/header.ld>

SECTIONS
{
	// On POWER, 0 is wherever HRMOR points to rather than physical DRAM start.
	// HRMOR is ORed with address, not added to it.
	DRAM_START(0x0)

#if !CONFIG(BOOTBLOCK_IN_SEEPROM)
	BOOTBLOCK(0, 32K)
#endif

	// TODO: ramstage should have stack in the last 128MB of first 4GB of RAM,
	// in case payload needs to be loaded here
	STACK(0x8000, 32K)
	PRERAM_CBMEM_CONSOLE(0x10000, 128K)
	FMAP_CACHE(0x30000, 4K)
	CBFS_MCACHE(0x31000, 8K)
	TIMESTAMP(0x33000, 4K)
	CBFS_CACHE(0x34000, 512K)
	ROMSTAGE(0x100000, 1M)

#if CONFIG(BOOTBLOCK_IN_SEEPROM)
	BOOTBLOCK(0x203000, 20K)
#endif

	RAMSTAGE(0xf8300000, 5M)
}
