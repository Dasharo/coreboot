/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

u8 mct_check_number_of_dqs_rcv_en_1_pass(u8 pass)
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

u32 setup_dqs_pattern_1_pass_a(u8 pass)
{
	return (u32) test_pattern_1_d;
}

u32 setup_dqs_pattern_1_pass_b(u8 pass)
{
	return (u32) test_pattern_0_d;
}

static u16 mct_average_rcvr_en_dly_1_pass(struct DCTStatStruc *p_dct_stat, u8 channel, u8 receiver,
					u8 pass)
{
	u16 i, max_value;
	u16 *p;
	u16 val;

	max_value = 0;
	p = p_dct_stat->ch_d_b_rcvr_dly[channel][receiver >> 1];

	for (i = 0; i < 8; i++) {
		/* get left value from DCTStatStruc.CHA_D0_B0_RCVRDLY*/
		val = p[i];
		/* get right value from DCTStatStruc.CHA_D0_B0_RCVRDLY_1*/
		val += PASS_1_MEM_CLK_DLY;
		/* write back the value to stack */
		if (val > max_value)
			max_value = val;

		p[i] = val;
	}
	/* p_dct_stat->DimmTrainFail &= ~(1<<receiver+channel); */

	return max_value;
}

u8 mct_save_rcv_en_dly_d_1_pass(struct DCTStatStruc *p_dct_stat, u8 pass)
{
	u8 ret;
	ret = 0;
	if ((p_dct_stat->dqs_rcv_en_pass == 0xff) && (pass == FIRST_PASS))
		ret = 2;
	return ret;
}

u16 mct_average_rcvr_en_dly_pass(struct DCTStatStruc *p_dct_stat,
				u16 rcvr_en_dly, u16 rcvr_en_dly_limit,
				u8 channel, u8 receiver, u8 pass)

{
	return mct_average_rcvr_en_dly_1_pass(p_dct_stat, channel, receiver, pass);
}
