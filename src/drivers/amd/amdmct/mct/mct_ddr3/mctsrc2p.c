/* SPDX-License-Identifier: GPL-2.0-only */

u8 mct_checkNumberOfDqsRcvEn_Pass(u8 pass)
{
	return 1;
}

u32 SetupDqsPattern_PassA(u8 Pass)
{
	u32 ret;
	if (Pass == FirstPass)
		ret = (u32) TestPattern1_D;
	else
		ret = (u32) TestPattern2_D;

	return ret;
}

u32 SetupDqsPattern_PassB(u8 Pass)
{
	u32 ret;
	if (Pass == FirstPass)
		ret = (u32) TestPattern0_D;
	else
		ret = (u32) TestPattern2_D;

	return ret;
}

u8 mct_Get_Start_RcvrEnDly_Pass(struct DCTStatStruc *pDCTstat,
					u8 Channel, u8 Receiver,
					u8 Pass)
{
	u8 RcvrEnDly;

	if (Pass == FirstPass)
		RcvrEnDly = 0;
	else {
		u8 max = 0;
		u8 val;
		u8 i;
		u8 *p = pDCTstat->persistentData.CH_D_B_RCVRDLY[Channel][Receiver >> 1];
		u8 bn;
		bn = 8;

		for (i = 0; i < bn; i++) {
			val  = p[i];

			if (val > max) {
				max = val;
			}
		}
		RcvrEnDly = max;
	}

	return RcvrEnDly;
}

u16 mct_Average_RcvrEnDly_Pass(struct DCTStatStruc *pDCTstat,
				u16 RcvrEnDly, u16 RcvrEnDlyLimit,
				u8 Channel, u8 Receiver, u8 Pass)
{
	u8 i;
	u16 *p;
	u16 *p_1;
	u16 val;
	u16 val_1;
	u8 valid = 1;
	u8 bn;

	bn = 8;

	p = pDCTstat->persistentData.CH_D_B_RCVRDLY[Channel][Receiver >> 1];

	if (Pass == SecondPass) { /* second pass must average values */
		/* FIXME: which byte? */
		p_1 = pDCTstat->B_RCVRDLY_1;
		/* p_1 = pDCTstat->persistentData.CH_D_B_RCVRDLY_1[Channel][Receiver>>1]; */
		for (i = 0; i < bn; i++) {
			val = p[i];
			/* left edge */
			if (val != (RcvrEnDlyLimit - 1)) {
				val -= Pass1MemClkDly;
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
			pDCTstat->ErrStatus |= 1 << SB_NORCVREN;
		} else {
			pDCTstat->DimmTrainFail &= ~(1 << (Receiver + Channel));
		}
	} else {
		for (i = 0; i < bn; i++) {
			val = p[i];
			/* Add 1/2 Memlock delay */
			/* val += Pass1MemClkDly; */
			val += 0x5; /* NOTE: middle value with DQSRCVEN_SAVED_GOOD_TIMES */
			/* val += 0x02; */
			p[i] = val;
			pDCTstat->DimmTrainFail &= ~(1 << (Receiver + Channel));
		}
	}

	return RcvrEnDly;
}
