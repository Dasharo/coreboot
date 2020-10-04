/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/variants.h>

static const struct lpddr4x_cfg malefor_memcfg = {
	/* DQ byte map */
	.dq_map = {
		[0] = {
			{ 3,  1,  0,  2,  4,  6,  7,  5, }, /* DDR0_DQ0[7:0] */
			{ 12, 13, 14, 15,  8,  9, 10, 11 }, /* DDR0_DQ1[7:0] */
		},
		[1] = {
			{ 0,  7,  1,  6,  2,  4,  3,  5, }, /* DDR1_DQ0[7:0] */
			{ 8,  15, 14,  9, 13, 10, 12, 11 }, /* DDR1_DQ1[7:0] */
		},
		[2] = {
			{ 3,  2,  0,  1,  4,  5,  6,  7, }, /* DDR2_DQ0[7:0] */
			{ 12, 13, 15, 14,  8,  9, 10, 11 }, /* DDR2_DQ1[7:0] */
		},
		[3] = {
			{ 6,  0,  1,  7,  5,  4,  2,  3, }, /* DDR3_DQ0[7:0] */
			{ 15, 14,  8,  9, 10, 12, 11, 13 }, /* DDR3_DQ1[7:0] */
		},
		[4] = {
			{ 5,  0,  1,  3,  4,  2,  7,  6, }, /* DDR4_DQ0[7:0] */
			{ 11, 14, 13, 12,  8,  9, 15, 10 }, /* DDR4_DQ1[7:0] */
		},
		[5] = {
			{  3,  4,  2,  5,  0,  6,  1,  7, }, /* DDR5_DQ0[7:0] */
			{  13, 12, 11, 10, 14, 15,  9,  8 }, /* DDR5_DQ1[7:0] */
		},
		[6] = {
			{  3,  2,  1,  0,  5,  4,  7,  6, }, /* DDR6_DQ0[7:0] */
			{  12, 13, 15, 14,  8, 11,  9, 10 }, /* DDR6_DQ1[7:0] */
		},
		[7] = {
			{  3,  4,  2,  5,  1,  0,  7,  6, }, /* DDR7_DQ0[7:0] */
			{  15, 14,  9,  8, 12, 10, 11, 13 }, /* DDR7_DQ1[7:0] */
		},
	},

	/* DQS CPU<>DRAM map */
	.dqs_map = {
		[0] = { 0, 1 },  /* DDR0_DQS[1:0] */
		[1] = { 0, 1 },  /* DDR1_DQS[1:0] */
		[2] = { 0, 1 },  /* DDR2_DQS[1:0] */
		[3] = { 0, 1 },  /* DDR3_DQS[1:0] */
		[4] = { 0, 1 },  /* DDR4_DQS[1:0] */
		[5] = { 0, 1 },  /* DDR5_DQS[1:0] */
		[6] = { 0, 1 },  /* DDR6_DQS[1:0] */
		[7] = { 0, 1 },  /* DDR7_DQS[1:0] */
	},

	.ect = 1, /* Enable Early Command Training */
};

static const struct ddr_memory_cfg board_memcfg = {
	.mem_type = MEMTYPE_LPDDR4X,
	.lpddr4_cfg = &malefor_memcfg
};

const struct ddr_memory_cfg *variant_memory_params(void)
{
	return &board_memcfg;
}
