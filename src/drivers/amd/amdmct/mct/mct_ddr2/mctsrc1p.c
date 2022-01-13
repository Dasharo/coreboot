/* SPDX-License-Identifier: GPL-2.0-only */

#include <drivers/amd/amdmct/wrappers/mcti.h>

u8 mct_checkNumberOfDqsRcvEn_1Pass(u8 pass)
{
	u8 ret = 1;
	if (pass == SECOND_PASS)
		ret = 0;

	return ret;
}


u32 SetupDqsPattern_1PassA(u8 pass)
{
	return (u32) TestPattern1_D;
}


u32 SetupDqsPattern_1PassB(u8 pass)
{
	return (u32) TestPattern0_D;
}

u8  mct_Get_Start_RcvrEnDly_1Pass(u8 pass)
{
	return 0;
}

static u8 mct_Average_RcvrEnDly_1Pass(struct DCTStatStruc *pDCTstat, u8 Channel, u8 Receiver,
					u8 Pass)
{
	u8 i, MaxValue;
	u8 *p;
	u8 val;

	MaxValue = 0;
	p = pDCTstat->persistentData.CH_D_B_RCVRDLY[Channel][Receiver >> 1];

	for (i = 0; i < 8; i++) {
		/* get left value from DCTStatStruc.CHA_D0_B0_RCVRDLY*/
		val = p[i];
		/* get right value from DCTStatStruc.CHA_D0_B0_RCVRDLY_1*/
		val += PASS_1_MEM_CLK_DLY;
		/* write back the value to stack */
		if (val > MaxValue)
			MaxValue = val;

		p[i] = val;
	}

	return MaxValue;
}

#ifdef UNUSED_CODE
static u8 mct_AdjustFinalDQSRcvValue_1Pass(u8 val_1p, u8 val_2p)
{
	return (val_1p & 0xff) + ((val_2p & 0xff) << 8);
}
#endif

u8 mct_SaveRcvEnDly_D_1Pass(struct DCTStatStruc *pDCTstat, u8 pass)
{
	u8 ret;
	ret = 0;
	if ((pDCTstat->DqsRcvEn_Pass == 0xff) && (pass == FIRST_PASS))
		ret = 2;
	return ret;
}

u8 mct_Average_RcvrEnDly_Pass(struct DCTStatStruc *pDCTstat,
				u8 RcvrEnDly, u8 RcvrEnDlyLimit,
				u8 Channel, u8 Receiver, u8 Pass)

{
	return mct_Average_RcvrEnDly_1Pass(pDCTstat, Channel, Receiver, Pass);
}
