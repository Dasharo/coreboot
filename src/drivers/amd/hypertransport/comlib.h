/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef COMLIB_H
#define COMLIB_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "porting.h"

#ifdef AMD_DEBUG_ERROR_STOP
	/* Macro to aid debugging, causes program to halt and display the line number of the
	 * halt
	 */
	#define STOP_HERE ASSERT(0)
#else
	#define STOP_HERE
#endif

void CALLCONV amd_pci_read_bits(SBDFO loc, u8 highbit, u8 lowbit, u32 *value);
void CALLCONV amd_pci_write_bits(SBDFO loc, u8 highbit, u8 lowbit, u32 *value);
void CALLCONV amd_pci_find_next_cap(SBDFO *current);

void CALLCONV amd_memcpy(void *dst, const void *src, u32 length);
void CALLCONV amd_memset(void *buf, u8 val, u32 length);

u8 CALLCONV amd_bit_scan_reverse(u32 value);
u32 CALLCONV amd_rotate_right(u32 value, u8 size, u32 count);
u32 CALLCONV amd_rotate_left(u32 value, u8 size, u32 count);

#endif
