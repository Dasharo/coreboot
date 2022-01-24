/* SPDX-License-Identifier: GPL-2.0-only */

#include <drivers/amd/amdmct/wrappers/mcti.h>

void mct_dram_init_hw_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 reg;
	u32 dev = p_dct_stat->dev_dct;

	/*flag for selecting HW/SW DRAM Init HW DRAM Init */
	reg = 0x90 + 0x100 * dct; /*DRAM Configuration Low */
	val = get_nb32(dev, reg);
	val |= (1 << INIT_DRAM);
	set_nb32(dev, reg, val);
}
