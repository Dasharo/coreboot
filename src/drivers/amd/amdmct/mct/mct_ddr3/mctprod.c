/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

void mct_ExtMCTConfig_Dx(struct DCTStatStruc *pDCTstat)
{
	u32 dword;

	if (pDCTstat->LogicalCPUID & AMD_DR_Dx) {
		dword = 0x0ce00f00 | 0x1 << 29;	/* FlushWrOnStpGnt */
		if (!(pDCTstat->GangedMode))
			dword |= 0x18 << 2;	/* MctWrLimit = 0x18 for unganged mode */
		else
			dword |= 0x10 << 2;	/* MctWrLimit = 0x10 for ganged mode */
		Set_NB32(pDCTstat->dev_dct, 0x11c, dword);

		dword = Get_NB32(pDCTstat->dev_dct, 0x1b0);
		dword &= ~0x3;			/* AdapPrefMissRatio = 0x1 */
		dword |= 0x1;
		dword &= ~(0x3 << 2);		/* AdapPrefPositiveStep = 0x0 */
		dword &= ~(0x3 << 4);		/* AdapPrefNegativeStep = 0x0 */
		dword &= ~(0x7 << 8);		/* CohPrefPrbLmt = 0x1 */
		dword |= (0x1 << 8);
		dword |= (0x7 << 22);		/* PrefFourConf = 0x7 */
		dword |= (0x7 << 25);		/* PrefFiveConf = 0x7 */

		if (!(pDCTstat->GangedMode))
			dword |= (0x1 << 12);	/* EnSplitDctLimits = 0x1 */
		else
			dword &= ~(0x1 << 12);	/* EnSplitDctLimits = 0x0 */

		dword &= ~(0xf << 28);		/* DcqBwThrotWm = ... */
		switch (pDCTstat->Speed) {
		case 4:
			dword |= (0x5 << 28);	/* ...5 for DDR800 */
			break;
		case 5:
			dword |= (0x6 << 28);	/* ...6 for DDR1066 */
			break;
		case 6:
			dword |= (0x8 << 28);	/* ...8 for DDR800 */
			break;
		default:
			dword |= (0x9 << 28);	/* ...9 for DDR1600 */
			break;
		}
		Set_NB32(pDCTstat->dev_dct, 0x1b0, dword);
	}
}
