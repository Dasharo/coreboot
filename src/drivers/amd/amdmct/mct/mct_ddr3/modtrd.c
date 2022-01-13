/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

u32 mct_MR1Odt_RDimm(struct MCTStatStruc *pMCTstat,
				struct DCTStatStruc *pDCTstat, u8 dct, u32 MrsChipSel)
{
	u8 Speed = pDCTstat->speed;
	u32 ret;
	u8 DimmsInstalled, DimmNum, ChipSelect;

	ChipSelect = (MrsChipSel >> 20) & 0xF;
	DimmNum = ChipSelect & 0xFE;
	DimmsInstalled = pDCTstat->ma_dimms[dct];
	if (dct == 1)
		DimmNum ++;
	ret = 0;

	if (mctGet_NVbits(NV_MAX_DIMMS) == 4) {
		if (DimmsInstalled == 1)
			ret |= 1 << 2;
		else {
			if (pDCTstat->cs_present & 0xF0) {
				if (pDCTstat->dimm_qr_present & (1 << DimmNum)) {
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

u32 mct_DramTermDyn_RDimm(struct MCTStatStruc *pMCTstat,
				struct DCTStatStruc *pDCTstat, u8 dimm)
{
	u8 DimmsInstalled = dimm;
	u32 DramTermDyn = 0;
	u8 Speed = pDCTstat->speed;

	if (mctGet_NVbits(NV_MAX_DIMMS) == 4) {
		if (pDCTstat->cs_present & 0xF0) {
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
