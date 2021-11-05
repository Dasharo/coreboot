/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"
#include "mwlc_d.h"

void AmdMemPCIRead(SBDFO loc, u32 *Value)
{
	/* Convert SBDFO into a CF8 Address */
	loc = (loc >> 4 & 0xFFFFFF00) | (loc & 0xFF) | ((loc & 0xF00) << 16);
	loc |= 0x80000000;

	outl(loc, 0xCF8);

	*Value = inl(0xCFC);
}

void AmdMemPCIWrite(SBDFO loc, u32 *Value)
{
	/* Convert SBDFO into a CF8 Address */
	loc = (loc >> 4 & 0xFFFFFF00) | (loc & 0xFF) | ((loc & 0xF00) << 16);
	loc |= 0x80000000;

	outl(loc, 0xCF8);
	outl(*Value, 0xCFC);
}
