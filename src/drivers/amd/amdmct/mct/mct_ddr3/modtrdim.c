/* SPDX-License-Identifier: GPL-2.0-only */

/* This file contains functions for odt setting on registered DDR3 dimms */

#include <stdint.h>
#include <console/console.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

/**
 *
 *
 *   This function set Rtt_Nom for registered DDR3 dimms on targeted dimm.
 *
 *     @param      *p_mct_data
 *     @param[in]  *p_dct_data - Pointer to buffer with information about each DCT
 *     @param      dimm - targeted dimm
 *     @param      wl - current mode, either write levelization mode or normal mode
 *     @param      mem_clk_freq - current frequency
 *     @param      rank
 *
 *     @return     temp_w1 - Rtt_Nom
 */
u32 rtt_nom_target_reg_dimm (sMCTStruct *p_mct_data, sDCTStruct *p_dct_data, u8 dimm, bool wl, u8 mem_clk_freq, u8 rank)
{
	u32 temp_w1;
	temp_w1 = 0;
	if (wl) {
		switch (mct_get_nv_bits(NV_MAX_DIMMS_PER_CH)) {
		case 2:
			/* 2 dimms per channel */
			if (p_dct_data->MaxDimmsInstalled == 1) {
				if ((p_dct_data->dimm_ranks[dimm] == 2) && (rank == 0)) {
					temp_w1 = 0x00;	/* Rtt_Nom = OFF */
				} else if (p_dct_data->dimm_ranks[dimm] == 4) {
					if (rank == 1) {
						temp_w1 = 0x00;	/* Rtt_Nom = OFF on second and forth rank of QR dimm */
					} else {
						if (mem_clk_freq == 6) {
							temp_w1 = 0x04;	/* Rtt_Nom = 60 ohms */
						} else {
							temp_w1 = 0x40;	/* Rtt_Nom = 120 ohms */
						}
					}
				} else {
					temp_w1 = 0x04;	/* Rtt_Nom = 60 ohms */
				}
			} else if (p_dct_data->MaxDimmsInstalled == 2) {
				if (((p_dct_data->dimm_ranks[dimm] == 2) || (p_dct_data->dimm_ranks[dimm] == 4)) && (rank == 1)) {
					temp_w1 = 0x00;	/* Rtt_Nom = OFF */
				} else if ((p_dct_data->dimm_ranks[dimm] == 4) || (p_dct_data->DctCSPresent & 0xF0)) {
					if (mem_clk_freq == 3) {
						temp_w1 = 0x40;	/* Rtt_Nom = 120 ohms */
					} else {
						temp_w1 = 0x04;	/* Rtt_Nom = 60 ohms */
					}
				} else {
					if (mem_clk_freq == 6) {
						temp_w1 = 0x04;	/* Rtt_Nom = 60 ohms */
					} else {
						temp_w1 = 0x40;	/* Rtt_Nom = 120 ohms */
					}
				}
			}
			break;
		case 3:
			/* 3 dimms per channel */
			/* QR not supported in this case on L1 package. */
			if (p_dct_data->MaxDimmsInstalled == 1) {
				if ((p_dct_data->dimm_ranks[dimm] == 2) && (rank == 1)) {
					temp_w1 = 0x00;	/* Rtt_Nom = OFF */
				} else {
					temp_w1 = 0x04;	/* Rtt_Nom = 60 ohms */
				}
			} else {
				temp_w1 = 0x40;	/* Rtt_Nom = 120 ohms */
			}
			break;
		default:
			die("modtrdim.c: Wrong maximum value of DIMM's per channel.");
		}
	} else {
		switch (mct_get_nv_bits(NV_MAX_DIMMS_PER_CH)) {
		case 2:
			/* 2 dimms per channel */
			if ((p_dct_data->dimm_ranks[dimm] == 4) && (rank == 1)) {
				temp_w1 = 0x00;	/* Rtt_Nom = OFF */
			} else if ((p_dct_data->MaxDimmsInstalled == 1) || (p_dct_data->dimm_ranks[dimm] == 4)) {
				temp_w1 = 0x04;	/* Rtt_Nom = 60 ohms */
			} else {
				if (p_dct_data->DctCSPresent & 0xF0) {
					temp_w1 = 0x0204;	/* Rtt_Nom = 30 ohms */
				} else {
					if (mem_clk_freq < 5) {
						temp_w1 = 0x44;	/* Rtt_Nom = 40 ohms */
					} else {
						temp_w1 = 0x0204;	/* Rtt_Nom = 30 ohms */
					}
				}
			}
			break;
		case 3:
			/* 3 dimms per channel */
			/* L1 package does not support QR dimms this case. */
			if (rank == 1) {
				temp_w1 = 0x00;	/* Rtt_Nom = OFF */
			} else if (p_dct_data->MaxDimmsInstalled == 1) {
				temp_w1 = 0x04;	/* Rtt_Nom = 60 ohms */
			} else if ((mem_clk_freq < 5) || (p_dct_data->MaxDimmsInstalled == 3)) {
				temp_w1 = 0x44;	/* Rtt_Nom = 40 ohms */
			} else {
				temp_w1 = 0x0204;	/* Rtt_Nom = 30 ohms */
			}
			break;
		default:
			die("modtrdim.c: Wrong maximum value of DIMM's per channel.");
		}
	}
	return temp_w1;
}

/* -----------------------------------------------------------------------------*/
/**
 *
 *
 *   This function set Rtt_Nom for registered DDR3 dimms on non-targeted dimm.
 *
 *     @param      *p_mct_data
 *     @param[in]  *p_dct_data - Pointer to buffer with information about each DCT
 *     @param      dimm - non-targeted dimm
 *     @param      wl - current mode, either write levelization mode or normal mode
 *     @param      mem_clk_freq - current frequency
 *     @param      rank
 *
 *      @return    temp_w1 - Rtt_Nom
 */
u32 rtt_nom_non_target_reg_dimm (sMCTStruct *p_mct_data, sDCTStruct *p_dct_data, u8 dimm, bool wl, u8 mem_clk_freq, u8 rank)
{
	if ((wl) && (mct_get_nv_bits(NV_MAX_DIMMS_PER_CH) == 2) && (p_dct_data->dimm_ranks[dimm] == 2) && (rank == 1)) {
		return 0x00;	/* for non-target dimm during WL, the second rank of a DR dimm need to have Rtt_Nom = OFF */
	} else {
		return rtt_nom_target_reg_dimm (p_mct_data, p_dct_data, dimm, FALSE, mem_clk_freq, rank);	/* otherwise, the same as target dimm in normal mode. */
	}
}

/* -----------------------------------------------------------------------------*/
/**
 *
 *
 *   This function set Rtt_Wr for registered DDR3 dimms.
 *
 *     @param      p_mct_data
 *     @param[in]  *p_dct_data - Pointer to buffer with information about each DCT
 *     @param      dimm - targeted dimm
 *     @param      wl - current mode, either write levelization mode or normal mode
 *     @param      mem_clk_freq - current frequency
 *     @param      rank
 *
 *      @return    temp_w1 - Rtt_Wr
 */

u32 rtt_wr_reg_dimm (sMCTStruct *p_mct_data, sDCTStruct *p_dct_data, u8 dimm, bool wl, u8 mem_clk_freq, u8 rank)
{
	u32 temp_w1;
	temp_w1 = 0;
	if (wl) {
		temp_w1 = 0x00;	/* Rtt_WR = OFF */
	} else {
		switch (mct_get_nv_bits(NV_MAX_DIMMS_PER_CH)) {
		case 2:
			if (p_dct_data->MaxDimmsInstalled == 1) {
				if (p_dct_data->dimm_ranks[dimm] != 4) {
					temp_w1 = 0x00;
				} else {
					if (mem_clk_freq == 6) {
						temp_w1 = 0x200;	/* Rtt_WR = 60 ohms */
					} else {
						temp_w1 = 0x400;	/* Rtt_WR = 120 ohms */
					}
				}
			} else {
				if ((p_dct_data->dimm_ranks[dimm] == 4) || (p_dct_data->DctCSPresent & 0xF0)) {
					if (mem_clk_freq == 3) {
						temp_w1 = 0x400;	/* Rtt_WR = 120 ohms */
					} else {
						temp_w1 = 0x200;	/* Rtt_WR = 60 ohms */
					}
				} else {
					if (mem_clk_freq == 6) {
						temp_w1 = 0x200;	/* Rtt_WR = 60 ohms */
					} else {
						temp_w1 = 0x400;	/* Rtt_Nom = 120 ohms */
					}
				}
			}
			break;
		case 3:
			if (p_dct_data->MaxDimmsInstalled == 1) {
				temp_w1 = 0x00;	/* Rtt_WR = OFF */
			} else {
				temp_w1 = 0x400;	/* Rtt_Nom = 120 ohms */
			}
			break;
		default:
			die("modtrdim.c: Wrong maximum value of DIMM's per channel.");
		}
	}
	return temp_w1;
}

/* -----------------------------------------------------------------------------*/
/**
 *
 *
 *   This function set WrLvOdt for registered DDR3 dimms.
 *
 *     @param      *p_mct_data
 *     @param[in]  *p_dct_data - Pointer to buffer with information about each DCT
 *     @param      dimm - targeted dimm
 *
 *      @return    WrLvOdt
 */
u8 wr_lv_odt_reg_dimm (sMCTStruct *p_mct_data, sDCTStruct *p_dct_data, u8 dimm)
{
	u8 wr_lv_odt_1, i;
	wr_lv_odt_1 = 0;
	i = 0;
	while (i < 8) {
		if (p_dct_data->DctCSPresent & (1 << i)) {
			wr_lv_odt_1 = (u8)bit_test_set(wr_lv_odt_1, i/2);
		}
		i += 2;
	}
	if (mct_get_nv_bits(NV_MAX_DIMMS_PER_CH) == 2) {
		if ((p_dct_data->dimm_ranks[dimm] == 4) && (p_dct_data->MaxDimmsInstalled != 1)) {
			if (dimm >= 2) {
				wr_lv_odt_1 = (u8)bit_test_reset (wr_lv_odt_1, (dimm - 2));
			} else {
				wr_lv_odt_1 = (u8)bit_test_reset (wr_lv_odt_1, (dimm + 2));
			}
		} else if ((p_dct_data->dimm_ranks[dimm] == 2) && (p_dct_data->MaxDimmsInstalled == 1)) {
			/* the case for one DR on a 2 dimms per channel is special */
			wr_lv_odt_1 = 0x8;
		}
	}
	return wr_lv_odt_1;
}
