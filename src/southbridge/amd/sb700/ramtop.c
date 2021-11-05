/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <arch/io.h>
#include <amdblocks/biosram.h>

#include "sb700.h"

void backup_top_of_low_cacheable(uintptr_t ramtop)
{
	u32 dword = (u32) ramtop;
	int nvram_pos = 0xfc, i;
	for (i = 0; i < 4; i++) {
		outb(nvram_pos, BIOSRAM_INDEX);
		outb((dword >> (8 * i)) & 0xff, BIOSRAM_DATA);
		nvram_pos++;
	}
}

uintptr_t restore_top_of_low_cacheable(void)
{
	uint32_t xdata = 0;
	int xnvram_pos = 0xfc, xi;
	for (xi = 0; xi < 4; xi++) {
		outb(xnvram_pos, BIOSRAM_INDEX);
		xdata &= ~(0xff << (xi * 8));
		xdata |= inb(BIOSRAM_DATA) << (xi *8);
		xnvram_pos++;
	}
	return xdata;
}
