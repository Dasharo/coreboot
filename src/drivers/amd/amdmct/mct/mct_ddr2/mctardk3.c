/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>

static void Get_ChannelPS_Cfg0_D(u8 maa_dimms, u8 Speed, u8 MAAload,
				u8 DATAAload, u32 *addr_tmg_ctl, u32 *ODC_CTL);


void mctGet_PS_Cfg_D(struct MCTStatStruc *p_mct_stat,
			 struct DCTStatStruc *p_dct_stat, u32 dct)
{
	u16 val, valx;

	print_tx("dct: ", dct);
	print_tx("Speed: ", p_dct_stat->speed);

	Get_ChannelPS_Cfg0_D(p_dct_stat->ma_dimms[dct], p_dct_stat->speed,
				p_dct_stat->ma_load[dct], p_dct_stat->data_load[dct],
				&(p_dct_stat->ch_addr_tmg[dct]), &(p_dct_stat->ch_odc_ctl[dct]));


	if (p_dct_stat->ma_dimms[dct] == 1)
		p_dct_stat->ch_odc_ctl[dct] |= 0x20000000;	/* 75ohms */
	else
		p_dct_stat->ch_odc_ctl[dct] |= 0x10000000;	/* 150ohms */

	p_dct_stat->_2t_mode = 1;

	/* use byte lane 4 delay for ECC lane */
	p_dct_stat->ch_ecc_dqs_like[0] = 0x0504;
	p_dct_stat->ch_ecc_dqs_scale[0] = 0;	/* 100% byte lane 4 */
	p_dct_stat->ch_ecc_dqs_like[1] = 0x0504;
	p_dct_stat->ch_ecc_dqs_scale[1] = 0;	/* 100% byte lane 4 */


	/*
	 Overrides and/or exceptions
	*/

	/* 1) QRx4 needs to adjust CS/ODT setup time */
	// FIXME: Add Ax support?
	if (mct_get_nv_bits(NV_MAX_DIMMS) == 4) {
		if (p_dct_stat->dimm_qr_present != 0) {
			p_dct_stat->ch_addr_tmg[dct] &= 0xFF00FFFF;
			p_dct_stat->ch_addr_tmg[dct] |= 0x00000000;
			if (p_dct_stat->ma_dimms[dct] == 4) {
				p_dct_stat->ch_addr_tmg[dct] &= 0xFF00FFFF;
				p_dct_stat->ch_addr_tmg[dct] |= 0x002F0000;
				if (p_dct_stat->speed == 3 || p_dct_stat->speed == 4) {
					p_dct_stat->ch_addr_tmg[dct] &= 0xFF00FFFF;
					p_dct_stat->ch_addr_tmg[dct] |= 0x00002F00;
					if (p_dct_stat->ma_dimms[dct] == 4)
						p_dct_stat->ch_odc_ctl[dct] = 0x00331222;
				}
			}
		}
	}


	/* 2) DRx4 (R/C-J) @ DDR667 needs to adjust CS/ODT setup time */
	if (p_dct_stat->speed == 3 || p_dct_stat->speed == 4) {
		val = p_dct_stat->dimm_x4_present;
		if (dct == 0) {
			val &= 0x55;
		} else {
			val &= 0xAA;
			val >>= 1;
		}
		val &= p_dct_stat->dimm_valid;
		if (val) {
			//FIXME: skip for Ax
			valx = p_dct_stat->dimm_dr_present;
			if (dct == 0) {
				valx &= 0x55;
			} else {
				valx &= 0xAA;
				valx >>= 1;
			}
			val &= valx;
			if (val != 0) {
				if (mct_get_nv_bits(NV_MAX_DIMMS) == 8 ||
						p_dct_stat->speed == 3) {
					p_dct_stat->ch_addr_tmg[dct] &= 0xFFFF00FF;
					p_dct_stat->ch_addr_tmg[dct] |= 0x00002F00;
				}
			}
		}
	}


	p_dct_stat->ch_odc_ctl[dct] = procOdtWorkaround(p_dct_stat, dct, p_dct_stat->ch_odc_ctl[dct]);

	print_tx("ch_odc_ctl: ", p_dct_stat->ch_odc_ctl[dct]);
	print_tx("ch_addr_tmg: ", p_dct_stat->ch_addr_tmg[dct]);


}


/*===============================================================================
 * Vendor is responsible for correct settings.
 * M2/Unbuffered 4 Slot - AMD Design Guideline.
 *===============================================================================
 * #1, BYTE, Speed (DCTStatstruc.speed) (Secondary Key)
 * #2, BYTE, number of Address bus loads on the Channel. (Tershery Key)
 *           These must be listed in ascending order.
 *           FFh (0xFE) has special meaning of 'any', and must be listed first for each speed grade.
 * #3, DWORD, Address Timing Control Register Value
 * #4, DWORD, Output Driver Compensation Control Register Value
 * #5, BYTE, Number of DIMMs (Primary Key)
 */
static const u8 Table_ATC_ODC_8D_D[] = {
	0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x22, 0x12, 0x11, 0x00, 1,
	0xFE, 0xFF, 0x00, 0x00, 0x37, 0x00, 0x22, 0x12, 0x11, 0x00, 2,
	   1, 0xFF, 0x00, 0x00, 0x2F, 0x00, 0x22, 0x12, 0x11, 0x00, 3,
	   2, 0xFF, 0x00, 0x00, 0x2F, 0x00, 0x22, 0x12, 0x11, 0x00, 3,
	   3, 0xFF, 0x2F, 0x00, 0x2F, 0x00, 0x22, 0x12, 0x11, 0x00, 3,
	   4, 0xFF, 0x2F, 0x00, 0x2F, 0x00, 0x22, 0x12, 0x33, 0x00, 3,
	   1, 0xFF, 0x00, 0x00, 0x2F, 0x00, 0x22, 0x12, 0x11, 0x00, 4,
	   2, 0xFF, 0x00, 0x00, 0x2F, 0x00, 0x22, 0x12, 0x11, 0x00, 4,
	   3, 0xFF, 0x2F, 0x00, 0x2F, 0x00, 0x22, 0x12, 0x33, 0x00, 4,
	   4, 0xFF, 0x2F, 0x00, 0x2F, 0x00, 0x22, 0x12, 0x33, 0x00, 4,
	0xFF
};

static const u8 Table_ATC_ODC_4D_D[] = {
	0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x22, 0x12, 0x11, 0x00, 1,
	0xFE, 0xFF, 0x00, 0x00, 0x37, 0x00, 0x22, 0x12, 0x11, 0x00, 2,
	0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x22, 0x12, 0x11, 0x00, 3,
	0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x22, 0x12, 0x11, 0x00, 4,
	0xFF
};


static void Get_ChannelPS_Cfg0_D(u8 maa_dimms, u8 Speed, u8 MAAload,
				u8 DATAAload, u32 *addr_tmg_ctl, u32 *ODC_CTL)
{
	const u8 *p;

	*addr_tmg_ctl = 0;
	*ODC_CTL = 0;

	if (mct_get_nv_bits(NV_MAX_DIMMS) == 8) {
		/* 8 DIMM Table */
		p = Table_ATC_ODC_8D_D;
		//FIXME Add Ax support
	} else {
		/* 4 DIMM Table*/
		p = Table_ATC_ODC_4D_D;
		//FIXME Add Ax support
	}

	while (*p != 0xFF) {
		if ((maa_dimms == *(p + 10)) || (*(p + 10) == 0xFE)) {
			if ((*p == Speed) || (*p == 0xFE)) {
				if (MAAload <= *(p + 1)) {
					*addr_tmg_ctl = stream_to_int((u8*)(p + 2));
					*ODC_CTL = stream_to_int((u8*)(p + 6));
					break;
				}
			}
		}
	p+=11;
	}
}
