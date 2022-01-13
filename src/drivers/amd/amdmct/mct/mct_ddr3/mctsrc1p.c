/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

u8 mct_checkNumberOfDqsRcvEn_1Pass(u8 pass)
{
	u8 ret = 1;

	if (is_fam15h()) {
		/* Fam15h needs two passes */
		ret = 1;
	} else {
		if (pass == SECOND_PASS)
			ret = 0;
	}

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

static u16 mct_Average_RcvrEnDly_1Pass(struct DCTStatStruc *pDCTstat, u8 Channel, u8 Receiver,
					u8 Pass)
{
	u16 i, MaxValue;
	u16 *p;
	u16 val;

	MaxValue = 0;
	p = pDCTstat->CH_D_B_RCVRDLY[Channel][Receiver >> 1];

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
	/* pDCTstat->DimmTrainFail &= ~(1<<Receiver+Channel); */

	return MaxValue;
}

u8 mct_SaveRcvEnDly_D_1Pass(struct DCTStatStruc *pDCTstat, u8 pass)
{
	u8 ret;
	ret = 0;
	if ((pDCTstat->DqsRcvEn_Pass == 0xff) && (pass == FIRST_PASS))
		ret = 2;
	return ret;
}

u16 mct_Average_RcvrEnDly_Pass(struct DCTStatStruc *pDCTstat,
				u16 RcvrEnDly, u16 RcvrEnDlyLimit,
				u8 Channel, u8 Receiver, u8 Pass)

{
	return mct_Average_RcvrEnDly_1Pass(pDCTstat, Channel, Receiver, Pass);
}
