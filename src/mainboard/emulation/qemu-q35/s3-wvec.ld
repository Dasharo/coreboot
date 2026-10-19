/* SPDX-License-Identifier: GPL-2.0-only */
OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)
SECTIONS {
	. = 0x00100000;
	.text : { *(.text*) *(.rodata*) }
	/DISCARD/ : { *(.eh_frame*) *(.note*) *(.comment*) }
}
