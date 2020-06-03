/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

gdtptr16_offset = gdtptr16 & 0xffff;
nullidt_offset = nullidt & 0xffff;

SECTIONS {
	/* The TXT AP entry is fixed to be on 0xfffef000 for now */
	. = 0xfffef000;
	.txt_ap_sipi (.) : {
		KEEP(*(.txt_ap_sipi));
	}
}

/*
 * Symbol _start16bit_txt_ap must be aligned to 4kB to start AP CPUs with
 * Startup IPI message without RAM.
 */
_bogus = ASSERT((_start16bit_txt_ap & 0xfff) == 0, "Symbol _start16bit_txt_ap is not at 4 KiB boundary");


/*
 * Symbols gdtptr and nullidt must be addressable with 16b offset when
 * %cs points to _start16bit_txt_ap.
 */
_bogus = ASSERT((gdtptr - _start16bit_txt_ap) < 0x10000 - 6, "Symbol gdtptr is not in range of TXT AP SIPI vector");
_bogus = ASSERT((nullidt - _start16bit_txt_ap) < 0x10000 - 6, "Symbol nullidt is not in range of TXT AP SIPI vector");
