/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef COMLIB_H
#define COMLIB_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "porting.h"

#ifdef AMD_DEBUG_ERROR_STOP
	/* Macro to aid debugging, causes program to halt and display the line number of the halt */
	#define STOP_HERE ASSERT(0)
#else
	#define STOP_HERE
#endif

void CALLCONV amd_pci_read_bits(SBDFO loc, uint8 highbit, uint8 lowbit, uint32 *value);
void CALLCONV amd_pci_write_bits(SBDFO loc, uint8 highbit, uint8 lowbit, uint32 *value);
void CALLCONV amd_pci_find_next_cap(SBDFO *current);

void CALLCONV amd_memcpy(void *dst, const void *src, uint32 length);
void CALLCONV amd_memset(void *buf, uint8 val, uint32 length);

uint8 CALLCONV amd_bit_scan_reverse(uint32 value);
uint32 CALLCONV amd_rotate_right(uint32 value, uint8 size, uint32 count);
uint32 CALLCONV amd_rotate_left(uint32 value, uint8 size, uint32 count);

#endif
