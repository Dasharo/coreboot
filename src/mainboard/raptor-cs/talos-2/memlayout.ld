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

#if !ENV_RAMSTAGE
	STACK(0x8000, 32K)
#endif

	PRERAM_CBMEM_CONSOLE(0x10000, 128K)
	FMAP_CACHE(0x30000, 4K)
	CBFS_MCACHE(0x31000, 8K)
	TIMESTAMP(0x33000, 4K)
	CBFS_CACHE(0x34000, 512K)
	ROMSTAGE(0x100000, 1M)

#if CONFIG(BOOTBLOCK_IN_SEEPROM)
	BOOTBLOCK(0x203000, 20K)
#endif

#if !ENV_RAMSTAGE
	RAMSTAGE(0x300000, 5M)
#else
	STACK(0xf8008000, 32K)
	RAMSTAGE(0xf8300000, 5M)
#endif
}
