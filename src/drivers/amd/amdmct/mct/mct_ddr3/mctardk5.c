/* SPDX-License-Identifier: GPL-2.0-only */

/* AM3/ASB2/C32/G34 DDR3 */

#include <arch/cpu.h>
#include <stdint.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static void Get_ChannelPS_Cfg0_D(u8 MAAdimms, u8 Speed, u8 MAAload,
				u32 *ODC_CTL,
				u8 *CMDmode);

void mctGet_PS_Cfg_D(struct MCTStatStruc *pMCTstat,
			 struct DCTStatStruc *pDCTstat, u32 dct)
{
	if (is_fam15h()) {
		pDCTstat->ch_addr_tmg[dct] = fam15h_address_timing_compensation_code(pDCTstat, dct);
		pDCTstat->ch_odc_ctl[dct] = fam15h_output_driver_compensation_code(pDCTstat, dct);
		pDCTstat->_2t_mode = fam15h_slow_access_mode(pDCTstat, dct);
	} else {
		Get_ChannelPS_Cfg0_D(pDCTstat->ma_dimms[dct], pDCTstat->speed,
					pDCTstat->ma_load[dct],
					&(pDCTstat->ch_odc_ctl[dct]),
					&pDCTstat->_2t_mode);

		if (pDCTstat->status & (1 << SB_REGISTERED)) {
			pDCTstat->_2t_mode = 1;	/* Disable slow access mode */
		}
		pDCTstat->ch_addr_tmg[dct] = fam10h_address_timing_compensation_code(pDCTstat, dct);

		pDCTstat->ch_odc_ctl[dct] |= 0x20000000;	/* 60ohms */
	}

	pDCTstat->ch_ecc_dqs_like[0]  = 0x0403;
	pDCTstat->ch_ecc_dqs_scale[0] = 0x70;
	pDCTstat->ch_ecc_dqs_like[1]  = 0x0403;
	pDCTstat->ch_ecc_dqs_scale[1] = 0x70;
}

/*
 *  In: MAAdimms   - number of DIMMs on the channel
 *    : Speed      - Speed (see DCTStatstruc.speed for definition)
 *    : MAAload    - number of address bus loads on the channel
 * Out: AddrTmgCTL - Address Timing Control Register Value
 *    : ODC_CTL    - Output Driver Compensation Control Register Value
 *    : CMDmode    - CMD mode
 */
static void Get_ChannelPS_Cfg0_D(u8 MAAdimms, u8 Speed, u8 MAAload,
				u32 *ODC_CTL,
				u8 *CMDmode)
{
	*ODC_CTL = 0;
	*CMDmode = 1;

	if (MAAdimms == 1) {
		*ODC_CTL = 0x00113222;
		*CMDmode = 1;
	} else /* if (MAAdimms == 0) */ {
		if (Speed == 4) {
			*CMDmode = 1;
		} else if (Speed == 5) {
			*CMDmode = 1;
		} else if (Speed == 6) {
			*CMDmode = 2;
		} else {
			*CMDmode = 2;
		}
		*ODC_CTL = 0x00223323;
	}
}
