/* SPDX-License-Identifier: GPL-2.0-only */

/* The socket type Fr2, G (1207) are not tested.
 */

static void Get_ChannelPS_Cfg0_D(u8 maa_dimms, u8 speed, u8 maa_load,
				u8 data_a_load, u32 *addr_tmg_ctl, u32 *odc_ctl,
				u8 *cmd_mode);


void mctGet_PS_Cfg_D(struct MCTStatStruc *p_mct_stat,
			 struct DCTStatStruc *p_dct_stat, u32 dct)
{
	Get_ChannelPS_Cfg0_D(p_dct_stat->ma_dimms[dct], p_dct_stat->speed,
				p_dct_stat->ma_load[dct], p_dct_stat->data_load[dct],
				&(p_dct_stat->ch_addr_tmg[dct]), &(p_dct_stat->ch_odc_ctl[dct]),
				&p_dct_stat->_2t_mode);

	if (p_dct_stat->ganged_mode == 1 && dct == 0)
		Get_ChannelPS_Cfg0_D(p_dct_stat->ma_dimms[1], p_dct_stat->speed,
				     p_dct_stat->ma_load[1], p_dct_stat->data_load[1],
				     &(p_dct_stat->ch_addr_tmg[1]), &(p_dct_stat->ch_odc_ctl[1]),
				     &p_dct_stat->_2t_mode);

	p_dct_stat->ch_ecc_dqs_like[0]  = 0x0302;
	p_dct_stat->ch_ecc_dqs_like[1]  = 0x0302;

}

/*
 *  In: maa_dimms   - number of DIMMs on the channel
 *    : speed      - speed (see DCTStatstruc.speed for definition)
 *    : maa_load    - number of address bus loads on the channel
 *    : data_a_load  - number of ranks on the channel
 * Out: addr_tmg_ctl - Address Timing Control Register Value
 *    : odc_ctl    - Output Driver Compensation Control Register Value
 *    : cmd_mode    - CMD mode
 */
static void Get_ChannelPS_Cfg0_D(u8 maa_dimms, u8 speed, u8 maa_load,
				u8 data_a_load, u32 *addr_tmg_ctl, u32 *odc_ctl,
				u8 *cmd_mode)
{
	*addr_tmg_ctl = 0;
	*odc_ctl = 0;
	*cmd_mode = 1;

	if (mct_get_nv_bits(NV_MAX_DIMMS) == 4) {
		if (speed == 4) {
			*addr_tmg_ctl = 0x00000000;
		} else if (speed == 5) {
			*addr_tmg_ctl = 0x003C3C3C;
			if (maa_dimms > 1)
				*addr_tmg_ctl = 0x003A3C3A;
		} else if (speed == 6) {
			if (maa_dimms == 1)
				*addr_tmg_ctl = 0x003A3A3A;
			else
				*addr_tmg_ctl = 0x00383A38;
		} else {
			if (maa_dimms == 1)
				*addr_tmg_ctl = 0x00373937;
			else
				*addr_tmg_ctl = 0x00353935;
		}
	} else {
		if (speed == 4) {
			*addr_tmg_ctl = 0x00000000;
			if (maa_dimms == 3)
				*addr_tmg_ctl = 0x00380038;
		} else if (speed == 5) {
			if (maa_dimms == 1)
				*addr_tmg_ctl = 0x003C3C3C;
			else if (maa_dimms == 2)
				*addr_tmg_ctl = 0x003A3C3A;
			else
				*addr_tmg_ctl = 0x00373C37;
		} else if (speed == 6) {
			if (maa_dimms == 1)
				*addr_tmg_ctl = 0x003A3A3A;
			else if (maa_dimms == 2)
				*addr_tmg_ctl = 0x00383A38;
			else
				*addr_tmg_ctl = 0x00343A34;
		} else {
			if (maa_dimms == 1)
				*addr_tmg_ctl = 0x00393939;
			else if (maa_dimms == 2)
				*addr_tmg_ctl = 0x00363936;
			else
				*addr_tmg_ctl = 0x00303930;
		}
	}

	if ((maa_dimms == 1) && (maa_load < 4))
		*odc_ctl = 0x20113222;
	else
		*odc_ctl = 0x20223222;

	*cmd_mode = 1;
}
