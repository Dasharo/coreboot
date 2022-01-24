/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

u32 mct_mr_1Odt_r_dimm(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct, u32 MrsChipSel)
{
	u8 Speed = p_dct_stat->speed;
	u32 ret;
	u8 DimmsInstalled, DimmNum, ChipSelect;

	ChipSelect = (MrsChipSel >> 20) & 0xF;
	DimmNum = ChipSelect & 0xFE;
	DimmsInstalled = p_dct_stat->ma_dimms[dct];
	if (dct == 1)
		DimmNum ++;
	ret = 0;

	if (mct_get_nv_bits(NV_MAX_DIMMS) == 4) {
		if (DimmsInstalled == 1)
			ret |= 1 << 2;
		else {
			if (p_dct_stat->cs_present & 0xF0) {
				if (p_dct_stat->dimm_qr_present & (1 << DimmNum)) {
					if (!(ChipSelect & 1))
						ret |= 1 << 2;
				} else
					ret |= 0x204;
			} else {
				if (Speed < 6)
					ret |= 0x44;
				else
					ret |= 0x204;
			}
		}
	} else if (DimmsInstalled == 1)
		ret |= 1 << 2;
	else if (Speed < 6)
		ret |= 0x44;
	else
		ret |= 0x204;

	//ret = 0;
	return ret;
}

u32 mct_dram_term_dyn_r_dimm(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dimm)
{
	u8 DimmsInstalled = dimm;
	u32 DramTermDyn = 0;
	u8 Speed = p_dct_stat->speed;

	if (mct_get_nv_bits(NV_MAX_DIMMS) == 4) {
		if (p_dct_stat->cs_present & 0xF0) {
			if (DimmsInstalled == 1)
				if (Speed == 7)
					DramTermDyn |= 1 << 10;
				else
					DramTermDyn |= 1 << 11;
			else
				if (Speed == 4)
					DramTermDyn |= 1 << 11;
				else
					DramTermDyn |= 1 << 10;
		} else {
			if (DimmsInstalled != 1) {
				if (Speed == 7)
					DramTermDyn |= 1 << 10;
				else
					DramTermDyn |= 1 << 11;
			}
		}
	} else {
		if (DimmsInstalled != 1)
			DramTermDyn |= 1 << 11;
	}
	return DramTermDyn;
}
