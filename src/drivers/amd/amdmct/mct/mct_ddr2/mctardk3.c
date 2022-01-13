/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>

static void Get_ChannelPS_Cfg0_D(u8 MAAdimms, u8 Speed, u8 MAAload,
				u8 DATAAload, u32 *AddrTmgCTL, u32 *ODC_CTL);


void mctGet_PS_Cfg_D(struct MCTStatStruc *pMCTstat,
			 struct DCTStatStruc *pDCTstat, u32 dct)
{
	u16 val, valx;

	print_tx("dct: ", dct);
	print_tx("Speed: ", pDCTstat->speed);

	Get_ChannelPS_Cfg0_D(pDCTstat->ma_dimms[dct], pDCTstat->speed,
				pDCTstat->ma_load[dct], pDCTstat->data_load[dct],
				&(pDCTstat->ch_addr_tmg[dct]), &(pDCTstat->ch_odc_ctl[dct]));


	if (pDCTstat->ma_dimms[dct] == 1)
		pDCTstat->ch_odc_ctl[dct] |= 0x20000000;	/* 75ohms */
	else
		pDCTstat->ch_odc_ctl[dct] |= 0x10000000;	/* 150ohms */

	pDCTstat->_2t_mode = 1;

	/* use byte lane 4 delay for ECC lane */
	pDCTstat->ch_ecc_dqs_like[0] = 0x0504;
	pDCTstat->ch_ecc_dqs_scale[0] = 0;	/* 100% byte lane 4 */
	pDCTstat->ch_ecc_dqs_like[1] = 0x0504;
	pDCTstat->ch_ecc_dqs_scale[1] = 0;	/* 100% byte lane 4 */


	/*
	 Overrides and/or exceptions
	*/

	/* 1) QRx4 needs to adjust CS/ODT setup time */
	// FIXME: Add Ax support?
	if (mctGet_NVbits(NV_MAX_DIMMS) == 4) {
		if (pDCTstat->dimm_qr_present != 0) {
			pDCTstat->ch_addr_tmg[dct] &= 0xFF00FFFF;
			pDCTstat->ch_addr_tmg[dct] |= 0x00000000;
			if (pDCTstat->ma_dimms[dct] == 4) {
				pDCTstat->ch_addr_tmg[dct] &= 0xFF00FFFF;
				pDCTstat->ch_addr_tmg[dct] |= 0x002F0000;
				if (pDCTstat->speed == 3 || pDCTstat->speed == 4) {
					pDCTstat->ch_addr_tmg[dct] &= 0xFF00FFFF;
					pDCTstat->ch_addr_tmg[dct] |= 0x00002F00;
					if (pDCTstat->ma_dimms[dct] == 4)
						pDCTstat->ch_odc_ctl[dct] = 0x00331222;
				}
			}
		}
	}


	/* 2) DRx4 (R/C-J) @ DDR667 needs to adjust CS/ODT setup time */
	if (pDCTstat->speed == 3 || pDCTstat->speed == 4) {
		val = pDCTstat->dimm_x4_present;
		if (dct == 0) {
			val &= 0x55;
		} else {
			val &= 0xAA;
			val >>= 1;
		}
		val &= pDCTstat->dimm_valid;
		if (val) {
			//FIXME: skip for Ax
			valx = pDCTstat->dimm_dr_present;
			if (dct == 0) {
				valx &= 0x55;
			} else {
				valx &= 0xAA;
				valx >>= 1;
			}
			val &= valx;
			if (val != 0) {
				if (mctGet_NVbits(NV_MAX_DIMMS) == 8 ||
						pDCTstat->speed == 3) {
					pDCTstat->ch_addr_tmg[dct] &= 0xFFFF00FF;
					pDCTstat->ch_addr_tmg[dct] |= 0x00002F00;
				}
			}
		}
	}


	pDCTstat->ch_odc_ctl[dct] = procOdtWorkaround(pDCTstat, dct, pDCTstat->ch_odc_ctl[dct]);

	print_tx("CH_ODC_CTL: ", pDCTstat->ch_odc_ctl[dct]);
	print_tx("CH_ADDR_TMG: ", pDCTstat->ch_addr_tmg[dct]);


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


static void Get_ChannelPS_Cfg0_D(u8 MAAdimms, u8 Speed, u8 MAAload,
				u8 DATAAload, u32 *AddrTmgCTL, u32 *ODC_CTL)
{
	const u8 *p;

	*AddrTmgCTL = 0;
	*ODC_CTL = 0;

	if (mctGet_NVbits(NV_MAX_DIMMS) == 8) {
		/* 8 DIMM Table */
		p = Table_ATC_ODC_8D_D;
		//FIXME Add Ax support
	} else {
		/* 4 DIMM Table*/
		p = Table_ATC_ODC_4D_D;
		//FIXME Add Ax support
	}

	while (*p != 0xFF) {
		if ((MAAdimms == *(p + 10)) || (*(p + 10) == 0xFE)) {
			if ((*p == Speed) || (*p == 0xFE)) {
				if (MAAload <= *(p + 1)) {
					*AddrTmgCTL = stream_to_int((u8*)(p + 2));
					*ODC_CTL = stream_to_int((u8*)(p + 6));
					break;
				}
			}
		}
	p+=11;
	}
}
