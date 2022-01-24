/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

void mct_dram_init_hw_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 reg;
	u32 dev = p_dct_stat->dev_dct;

	/*flag for selecting HW/SW DRAM Init HW DRAM Init */
	reg = 0x90; /*DRAM Configuration Low */
	val = Get_NB32_DCT(dev, dct, reg);
	val |= (1 << INIT_DRAM);
	Set_NB32_DCT(dev, dct, reg, val);
}
