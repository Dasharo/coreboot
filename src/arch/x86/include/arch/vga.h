/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef ARCH_X86_VGA_H
#define ARCH_X86_VGA_H

/* VGA IO 0x3B0-0x3BB and 0x3C0-0x3DF */
#define VGA_IO_BASE	0x3b0
#define VGA_IO_SIZE	0x30
#define VGA_IO_LIMIT	(VGA_IO_BASE + VGA_IO_SIZE - 1)

/* VGA MMIO and SMM ASEG share the same address range */
#define VGA_MMIO_BASE	0xa0000
#define VGA_MMIO_SIZE	0x20000
#define VGA_MMIO_LIMIT	(VGA_MMIO_BASE + VGA_MMIO_SIZE - 1)

#endif /* ARCH_X86_VGA_H */
