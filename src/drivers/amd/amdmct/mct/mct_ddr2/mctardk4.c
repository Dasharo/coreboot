/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>

static void Get_ChannelPS_Cfg0_D(u8 maa_dimms, u8 Speed, u8 MAAload,
				u8 DATAAload, u32 *addr_tmg_ctl, u32 *ODC_CTL,
				u8 *CMDmode);


void mct_get_ps_cfg_d(struct MCTStatStruc *p_mct_stat,
			 struct DCTStatStruc *p_dct_stat, u32 dct)
{
	print_tx("dct: ", dct);
	print_tx("Speed: ", p_dct_stat->speed);

	Get_ChannelPS_Cfg0_D(p_dct_stat->ma_dimms[dct], p_dct_stat->speed,
				p_dct_stat->ma_load[dct], p_dct_stat->data_load[dct],
				&(p_dct_stat->ch_addr_tmg[dct]), &(p_dct_stat->ch_odc_ctl[dct]),
				&p_dct_stat->_2t_mode);

	if (p_dct_stat->ma_dimms[dct] == 1)
		p_dct_stat->ch_odc_ctl[dct] |= 0x20000000;	/* 75ohms */
	else
		p_dct_stat->ch_odc_ctl[dct] |= 0x10000000;	/* 150ohms */


	/*
	 * Overrides and/or workarounds
	 */
	p_dct_stat->ch_odc_ctl[dct] = proc_odt_workaround(p_dct_stat, dct, p_dct_stat->ch_odc_ctl[dct]);

	print_tx("4 ch_odc_ctl: ", p_dct_stat->ch_odc_ctl[dct]);
	print_tx("4 ch_addr_tmg: ", p_dct_stat->ch_addr_tmg[dct]);
}

/*=============================================================================
 * Vendor is responsible for correct settings.
 * M2/Unbuffered 4 Slot - AMD Design Guideline.
 *=============================================================================
 * #1, BYTE, Speed (DCTStatstruc.speed)
 * #2, BYTE, number of Address bus loads on the Channel.
 *     These must be listed in ascending order.
 *     FFh (-1) has special meaning of 'any', and must be listed first for
 *     each speed grade.
 * #3, DWORD, Address Timing Control Register Value
 * #4, DWORD, Output Driver Compensation Control Register Value
 */

static const u8 Table_ATC_ODC_D_Bx[] = {
	1, 0xFF, 0x00, 0x2F, 0x2F, 0x0, 0x22, 0x13, 0x11, 0x0,
	2,   12, 0x00, 0x2F, 0x2F, 0x0, 0x22, 0x13, 0x11, 0x0,
	2,   16, 0x00, 0x2F, 0x00, 0x0, 0x22, 0x13, 0x11, 0x0,
	2,   20, 0x00, 0x2F, 0x38, 0x0, 0x22, 0x13, 0x11, 0x0,
	2,   24, 0x00, 0x2F, 0x37, 0x0, 0x22, 0x13, 0x11, 0x0,
	2,   32, 0x00, 0x2F, 0x34, 0x0, 0x22, 0x13, 0x11, 0x0,
	3,   12, 0x20, 0x22, 0x20, 0x0, 0x22, 0x13, 0x11, 0x0,
	3,   16, 0x20, 0x22, 0x30, 0x0, 0x22, 0x13, 0x11, 0x0,
	3,   20, 0x20, 0x22, 0x2C, 0x0, 0x22, 0x13, 0x11, 0x0,
	3,   24, 0x20, 0x22, 0x2A, 0x0, 0x22, 0x13, 0x11, 0x0,
	3,   32, 0x20, 0x22, 0x2B, 0x0, 0x22, 0x13, 0x11, 0x0,
	4, 0xFF, 0x20, 0x25, 0x20, 0x0, 0x22, 0x33, 0x11, 0x0,
	5, 0xFF, 0x20, 0x20, 0x2F, 0x0, 0x22, 0x32, 0x11, 0x0,
	0xFF
};


static void Get_ChannelPS_Cfg0_D(u8 maa_dimms, u8 Speed, u8 MAAload,
				u8 DATAAload, u32 *addr_tmg_ctl, u32 *ODC_CTL,
				u8 *CMDmode)
{
	u8 const *p;

	*addr_tmg_ctl = 0;
	*ODC_CTL = 0;
	*CMDmode = 1;

	// FIXME: add Ax support
	if (maa_dimms == 0) {
		*ODC_CTL = 0x00111222;
		if (Speed == 3)
			*addr_tmg_ctl = 0x00202220;
		else if (Speed == 2)
			*addr_tmg_ctl = 0x002F2F00;
		else if (Speed == 1)
			*addr_tmg_ctl = 0x002F2F00;
		else if (Speed == 4)
			*addr_tmg_ctl = 0x00202520;
		else if (Speed == 5)
			*addr_tmg_ctl = 0x002F2020;
		else
			*addr_tmg_ctl = 0x002F2F2F;
	} else if (maa_dimms == 1) {
		if (Speed == 4) {
			*CMDmode = 2;
			*addr_tmg_ctl = 0x00202520;
			*ODC_CTL = 0x00113222;
		} else if (Speed == 5) {
			*CMDmode = 2;
			*addr_tmg_ctl = 0x002F2020;
			*ODC_CTL = 0x00113222;
		} else {
			*CMDmode = 1;
			*ODC_CTL = 0x00111222;
			if (Speed == 3) {
				*addr_tmg_ctl = 0x00202220;
			} else if (Speed == 2) {
				if (MAAload == 4)
					*addr_tmg_ctl = 0x002B2F00;
				else if (MAAload == 16)
					*addr_tmg_ctl = 0x002B2F00;
				else if (MAAload == 8)
					*addr_tmg_ctl = 0x002F2F00;
				else
					*addr_tmg_ctl = 0x002F2F00;
			} else if (Speed == 1) {
				*addr_tmg_ctl = 0x002F2F00;
			} else {
				*addr_tmg_ctl = 0x002F2F2F;
			}
		}
	} else {
		*CMDmode = 2;
		p = Table_ATC_ODC_D_Bx;
	do {
		if (Speed == *p) {
			if (MAAload <= *(p + 1)) {
				*addr_tmg_ctl = stream_to_int(p + 2);
				*ODC_CTL = stream_to_int(p + 6);
				break;
			}
		}
		p+=10;
	} while (*p == 0xff);
	}
}
