/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

gdtptr16_offset = gdtptr16 & 0xffff;
nullidt_offset = nullidt & 0xffff;

/*
 * Symbol _start16bit must be aligned to 4kB to start AP CPUs with
 * Startup IPI message without RAM.
 */
_bogus = ASSERT((_start16bit & 0xfff) == 0, "Symbol _start16bit is not at 4 KiB boundary");

SECTIONS {
	/* The TXT AP entry is fixed to be on 0xfffe0000 for now */
	. = 0xfffe0000;
	.reset . : {
		*(.reset);
		. = 15;
		BYTE(0x00);
	}
}
