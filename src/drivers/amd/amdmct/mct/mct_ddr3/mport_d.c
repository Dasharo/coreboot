/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"
#include "mwlc_d.h"

void amd_mem_pci_read(SBDFO loc, u32 *value)
{
	/* Convert SBDFO into a CF8 Address */
	loc = (loc >> 4 & 0xFFFFFF00) | (loc & 0xFF) | ((loc & 0xF00) << 16);
	loc |= 0x80000000;

	outl(loc, 0xCF8);

	*value = inl(0xCFC);
}

void amd_mem_cpi_write(SBDFO loc, u32 *value)
{
	/* Convert SBDFO into a CF8 Address */
	loc = (loc >> 4 & 0xFFFFFF00) | (loc & 0xFF) | ((loc & 0xF00) << 16);
	loc |= 0x80000000;

	outl(loc, 0xCF8);
	outl(*value, 0xCFC);
}
