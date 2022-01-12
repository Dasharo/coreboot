/* SPDX-License-Identifier: GPL-2.0-only */

#include <drivers/amd/amdmct/wrappers/mcti.h>

void mct_DramInit_Hw_D(struct MCTStatStruc *pMCTstat,
			struct DCTStatStruc *pDCTstat, u8 dct)
{
	u32 val;
	u32 reg;
	u32 dev = pDCTstat->dev_dct;

	/*flag for selecting HW/SW DRAM Init HW DRAM Init */
	reg = 0x90 + 0x100 * dct; /*DRAM Configuration Low */
	val = Get_NB32(dev, reg);
	val |= (1 << InitDram);
	Set_NB32(dev, reg, val);
}
