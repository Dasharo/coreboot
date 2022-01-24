/* SPDX-License-Identifier: GPL-2.0-only */

u8 mct_check_number_of_dqs_rcv_en_pass(u8 pass)
{
	return 1;
}

u32 setup_dqs_pattern_pass_a(u8 pass)
{
	u32 ret;
	if (pass == FIRST_PASS)
		ret = (u32) test_pattern_1_d;
	else
		ret = (u32) test_pattern_2_d;

	return ret;
}

u32 setup_dqs_pattern_pass_b(u8 pass)
{
	u32 ret;
	if (pass == FIRST_PASS)
		ret = (u32) test_pattern_0_d;
	else
		ret = (u32) test_pattern_2_d;

	return ret;
}

u8 mct_get_start_rcvr_en_dly_pass(struct DCTStatStruc *p_dct_stat,
					u8 channel, u8 receiver,
					u8 pass)
{
	u8 rcvr_en_dly;

	if (pass == FIRST_PASS)
		rcvr_en_dly = 0;
	else {
		u8 max = 0;
		u8 val;
		u8 i;
		u8 *p = p_dct_stat->persistent_data.ch_d_b_rcvr_dly[channel][receiver >> 1];
		u8 bn;
		bn = 8;

		for (i = 0; i < bn; i++) {
			val  = p[i];

			if (val > max) {
				max = val;
			}
		}
		rcvr_en_dly = max;
	}

	return rcvr_en_dly;
}

u16 mct_average_rcvr_en_dly_pass(struct DCTStatStruc *p_dct_stat,
				u16 rcvr_en_dly, u16 rcvr_en_dly_limit,
				u8 channel, u8 receiver, u8 pass)
{
	u8 i;
	u16 *p;
	u16 *p_1;
	u16 val;
	u16 val_1;
	u8 valid = 1;
	u8 bn;

	bn = 8;

	p = p_dct_stat->persistent_data.ch_d_b_rcvr_dly[channel][receiver >> 1];

	if (pass == SECOND_PASS) { /* second pass must average values */
		/* FIXME: which byte? */
		p_1 = p_dct_stat->B_RCVRDLY_1;
		/* p_1 = p_dct_stat->persistent_data.CH_D_B_RCVRDLY_1[channel][receiver>>1]; */
		for (i = 0; i < bn; i++) {
			val = p[i];
			/* left edge */
			if (val != (rcvr_en_dly_limit - 1)) {
				val -= PASS_1_MEM_CLK_DLY;
				val_1 = p_1[i];
				val += val_1;
				val >>= 1;
				p[i] = val;
			} else {
				valid = 0;
				break;
			}
		}
		if (!valid) {
			p_dct_stat->err_status |= 1 << SB_NO_RCVR_EN;
		} else {
			p_dct_stat->DimmTrainFail &= ~(1 << (receiver + channel));
		}
	} else {
		for (i = 0; i < bn; i++) {
			val = p[i];
			/* Add 1/2 Memlock delay */
			/* val += PASS_1_MEM_CLK_DLY; */
			val += 0x5; /* NOTE: middle value with DQSRCVEN_SAVED_GOOD_TIMES */
			/* val += 0x02; */
			p[i] = val;
			p_dct_stat->DimmTrainFail &= ~(1 << (receiver + channel));
		}
	}

	return rcvr_en_dly;
}
