/* SPDX-License-Identifier: GPL-2.0-only */

u8 mct_checkNumberOfDqsRcvEn_Pass(u8 pass)
{
	return 1;
}


u32 SetupDqsPattern_PassA(u8 Pass)
{
	u32 ret;
	if (Pass == FIRST_PASS)
		ret = (u32) test_pattern_1_d;
	else
		ret = (u32) test_pattern_2_d;

	return ret;
}


u32 SetupDqsPattern_PassB(u8 Pass)
{
	u32 ret;
	if (Pass == FIRST_PASS)
		ret = (u32) test_pattern_0_d;
	else
		ret = (u32) test_pattern_2_d;

	return ret;
}


u8 mct_Get_Start_RcvrEnDly_Pass(struct DCTStatStruc *p_dct_stat,
					u8 Channel, u8 Receiver,
					u8 Pass)
{
	u8 RcvrEnDly;

	if (Pass == FIRST_PASS)
		RcvrEnDly = 0;
	else {
		u8 max = 0;
		u8 val;
		u8 i;
		u8 *p = p_dct_stat->persistentData.ch_d_b_rcvr_dly[Channel][Receiver >> 1];
		u8 bn;
		bn = 8;

		for (i = 0; i < bn; i++) {
			val  = p[i];
			if (val > max) {
				max = val;
			}
		}
		RcvrEnDly = max;
//		while (1) {; }
//		RcvrEnDly += SEC_PASS_OFFSET; //FIXME Why
	}

	return RcvrEnDly;
}



u8 mct_average_rcvr_en_dly_pass(struct DCTStatStruc *p_dct_stat,
				u8 RcvrEnDly, u8 RcvrEnDlyLimit,
				u8 Channel, u8 Receiver, u8 Pass)
{
	u8 i;
	u8 *p;
	u8 *p_1;
	u8 val;
	u8 val_1;
	u8 valid = 1;
	u8 bn;

	bn = 8;

	p = p_dct_stat->persistentData.ch_d_b_rcvr_dly[Channel][Receiver >> 1];

	if (Pass == SECOND_PASS) { /* second pass must average values */
		//FIXME: which byte?
		p_1 = p_dct_stat->B_RCVRDLY_1;
//		p_1 = p_dct_stat->persistentData.CH_D_B_RCVRDLY_1[Channel][Receiver>>1];
		for (i = 0; i < bn; i++) {
			val = p[i];
			/* left edge */
			if (val != (RcvrEnDlyLimit - 1)) {
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
			p_dct_stat->DimmTrainFail &= ~(1 << (Receiver + Channel));
		}
	} else {
		for (i = 0; i < bn; i++) {
			val = p[i];
			/* Add 1/2 Memlock delay */
			val += 0x5; // NOTE: middle value with DQSRCVEN_SAVED_GOOD_TIMES
			p[i] = val;
			p_dct_stat->DimmTrainFail &= ~(1 << (Receiver + Channel));
		}
	}

	return RcvrEnDly;
}
