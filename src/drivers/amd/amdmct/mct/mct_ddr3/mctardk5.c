/* SPDX-License-Identifier: GPL-2.0-only */

/* AM3/ASB2/C32/G34 DDR3 */

#include <arch/cpu.h>
#include <stdint.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static void get_channel_ps_cfg_0_d(u8 maa_dimms, u8 speed, u8 maa_load,
				u32 *odc_ctl,
				u8 *cmd_mode);

void mct_get_ps_cfg_d(struct MCTStatStruc *p_mct_stat,
			 struct DCTStatStruc *p_dct_stat, u32 dct)
{
	if (is_fam15h()) {
		p_dct_stat->ch_addr_tmg[dct] = fam15h_address_timing_compensation_code(p_dct_stat, dct);
		p_dct_stat->ch_odc_ctl[dct] = fam15h_output_driver_compensation_code(p_dct_stat, dct);
		p_dct_stat->_2t_mode = fam15h_slow_access_mode(p_dct_stat, dct);
	} else {
		get_channel_ps_cfg_0_d(p_dct_stat->ma_dimms[dct], p_dct_stat->speed,
					p_dct_stat->ma_load[dct],
					&(p_dct_stat->ch_odc_ctl[dct]),
					&p_dct_stat->_2t_mode);

		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			p_dct_stat->_2t_mode = 1;	/* Disable slow access mode */
		}
		p_dct_stat->ch_addr_tmg[dct] = fam10h_address_timing_compensation_code(p_dct_stat, dct);

		p_dct_stat->ch_odc_ctl[dct] |= 0x20000000;	/* 60ohms */
	}

	p_dct_stat->ch_ecc_dqs_like[0]  = 0x0403;
	p_dct_stat->ch_ecc_dqs_scale[0] = 0x70;
	p_dct_stat->ch_ecc_dqs_like[1]  = 0x0403;
	p_dct_stat->ch_ecc_dqs_scale[1] = 0x70;
}

/*
 *  In: maa_dimms   - number of DIMMs on the channel
 *    : speed      - speed (see DCTStatstruc.speed for definition)
 *    : maa_load    - number of address bus loads on the channel
 * Out: addr_tmg_ctl - Address Timing Control Register Value
 *    : odc_ctl    - Output Driver Compensation Control Register Value
 *    : cmd_mode    - CMD mode
 */
static void get_channel_ps_cfg_0_d(u8 maa_dimms, u8 speed, u8 maa_load,
				u32 *odc_ctl,
				u8 *cmd_mode)
{
	*odc_ctl = 0;
	*cmd_mode = 1;

	if (maa_dimms == 1) {
		*odc_ctl = 0x00113222;
		*cmd_mode = 1;
	} else /* if (maa_dimms == 0) */ {
		if (speed == 4) {
			*cmd_mode = 1;
		} else if (speed == 5) {
			*cmd_mode = 1;
		} else if (speed == 6) {
			*cmd_mode = 2;
		} else {
			*cmd_mode = 2;
		}
		*odc_ctl = 0x00223323;
	}
}
