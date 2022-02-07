/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

u32 mct_mr_1Odt_r_dimm(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_chip_sel)
{
	u8 speed = p_dct_stat->speed;
	u32 ret;
	u8 dimms_installed, dimm_num, chip_select;

	chip_select = (mrs_chip_sel >> 20) & 0xF;
	dimm_num = chip_select & 0xFE;
	dimms_installed = p_dct_stat->ma_dimms[dct];
	if (dct == 1)
		dimm_num ++;
	ret = 0;

	if (mct_get_nv_bits(NV_MAX_DIMMS) == 4) {
		if (dimms_installed == 1)
			ret |= 1 << 2;
		else {
			if (p_dct_stat->cs_present & 0xF0) {
				if (p_dct_stat->dimm_qr_present & (1 << dimm_num)) {
					if (!(chip_select & 1))
						ret |= 1 << 2;
				} else
					ret |= 0x204;
			} else {
				if (speed < 6)
					ret |= 0x44;
				else
					ret |= 0x204;
			}
		}
	} else if (dimms_installed == 1)
		ret |= 1 << 2;
	else if (speed < 6)
		ret |= 0x44;
	else
		ret |= 0x204;

	//ret = 0;
	return ret;
}

u32 mct_dram_term_dyn_r_dimm(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dimm)
{
	u8 dimms_installed = dimm;
	u32 dram_term_dyn = 0;
	u8 speed = p_dct_stat->speed;

	if (mct_get_nv_bits(NV_MAX_DIMMS) == 4) {
		if (p_dct_stat->cs_present & 0xF0) {
			if (dimms_installed == 1)
				if (speed == 7)
					dram_term_dyn |= 1 << 10;
				else
					dram_term_dyn |= 1 << 11;
			else
				if (speed == 4)
					dram_term_dyn |= 1 << 11;
				else
					dram_term_dyn |= 1 << 10;
		} else {
			if (dimms_installed != 1) {
				if (speed == 7)
					dram_term_dyn |= 1 << 10;
				else
					dram_term_dyn |= 1 << 11;
			}
		}
	} else {
		if (dimms_installed != 1)
			dram_term_dyn |= 1 << 11;
	}
	return dram_term_dyn;
}
