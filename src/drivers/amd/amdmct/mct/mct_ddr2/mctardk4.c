/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>

static void get_channel_ps_cfg_0_d(u8 maa_dimms, u8 speed, u8 ma_a_load,
				u8 data_a_load, u32 *addr_tmg_ctl, u32 *odc_ctl,
				u8 *cmd_mode);


void mct_get_ps_cfg_d(struct MCTStatStruc *p_mct_stat,
			 struct DCTStatStruc *p_dct_stat, u32 dct)
{
	print_tx("dct: ", dct);
	print_tx("speed: ", p_dct_stat->speed);

	get_channel_ps_cfg_0_d(p_dct_stat->ma_dimms[dct], p_dct_stat->speed,
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
 * #1, BYTE, speed (DCTStatstruc.speed)
 * #2, BYTE, number of Address bus loads on the Channel.
 *     These must be listed in ascending order.
 *     FFh (-1) has special meaning of 'any', and must be listed first for
 *     each speed grade.
 * #3, DWORD, Address Timing Control Register Value
 * #4, DWORD, Output Driver Compensation Control Register Value
 */

static const u8 table_atc_odc_d_bx[] = {
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


static void get_channel_ps_cfg_0_d(u8 maa_dimms, u8 speed, u8 ma_a_load,
				u8 data_a_load, u32 *addr_tmg_ctl, u32 *odc_ctl,
				u8 *cmd_mode)
{
	u8 const *p;

	*addr_tmg_ctl = 0;
	*odc_ctl = 0;
	*cmd_mode = 1;

	// FIXME: add Ax support
	if (maa_dimms == 0) {
		*odc_ctl = 0x00111222;
		if (speed == 3)
			*addr_tmg_ctl = 0x00202220;
		else if (speed == 2)
			*addr_tmg_ctl = 0x002F2F00;
		else if (speed == 1)
			*addr_tmg_ctl = 0x002F2F00;
		else if (speed == 4)
			*addr_tmg_ctl = 0x00202520;
		else if (speed == 5)
			*addr_tmg_ctl = 0x002F2020;
		else
			*addr_tmg_ctl = 0x002F2F2F;
	} else if (maa_dimms == 1) {
		if (speed == 4) {
			*cmd_mode = 2;
			*addr_tmg_ctl = 0x00202520;
			*odc_ctl = 0x00113222;
		} else if (speed == 5) {
			*cmd_mode = 2;
			*addr_tmg_ctl = 0x002F2020;
			*odc_ctl = 0x00113222;
		} else {
			*cmd_mode = 1;
			*odc_ctl = 0x00111222;
			if (speed == 3) {
				*addr_tmg_ctl = 0x00202220;
			} else if (speed == 2) {
				if (ma_a_load == 4)
					*addr_tmg_ctl = 0x002B2F00;
				else if (ma_a_load == 16)
					*addr_tmg_ctl = 0x002B2F00;
				else if (ma_a_load == 8)
					*addr_tmg_ctl = 0x002F2F00;
				else
					*addr_tmg_ctl = 0x002F2F00;
			} else if (speed == 1) {
				*addr_tmg_ctl = 0x002F2F00;
			} else {
				*addr_tmg_ctl = 0x002F2F2F;
			}
		}
	} else {
		*cmd_mode = 2;
		p = table_atc_odc_d_bx;
	do {
		if (speed == *p) {
			if (ma_a_load <= *(p + 1)) {
				*addr_tmg_ctl = stream_to_int(p + 2);
				*odc_ctl = stream_to_int(p + 6);
				break;
			}
		}
		p+=10;
	} while (*p == 0xff);
	}
}
