/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <console/console.h>
#include <gpio.h>
#include <memory_info.h>
#include <string.h>

static const struct mb_cfg baseboard_memcfg = {
	.type = MEM_TYPE_LP4X,

	.rcomp = {
		/* Baseboard uses only 100ohm Rcomp resistors */
		.resistor = 100,

		/* Baseboard Rcomp target values */
		.targets = {40, 30, 30, 30, 30},
	},

	/* DQ byte map */
	.lpx_dq_map = {
		.ddr0 = {
			.dq0 = {  9, 11,  8, 10, 12, 14, 13, 15, },
			.dq1 = {  4,  7,  6,  5,  2,  3,  0,  1, },
		},
		.ddr1 = {
			.dq0 = { 15, 12, 14, 13,  9, 10, 11,  8, },
			.dq1 = {  0,  1,  3,  2,  7,  5,  4,  6, },
		},
		.ddr2 = {
			.dq0 = {  2,  3,  1,  0,  6,  7,  5,  4, },
			.dq1 = { 15,  9, 14,  8, 11, 10, 13, 12, },
		},
		.ddr3 = {
			.dq0 = {  3,  1,  2,  0,  4,  6,  7,  5, },
			.dq1 = { 13, 15, 14, 12, 11,  9,  8, 10, },
		},
		.ddr4 = {
			.dq0 = { 13, 12, 14, 15,  9,  8, 10, 11, },
			.dq1 = {  4,  7,  5,  6,  1,  2,  0,  3, },
		},
		.ddr5 = {
			.dq0 = {  5,  0,  6,  4,  3,  1,  7,  2, },
			.dq1 = { 11,  9, 10,  8, 15, 12, 14, 13, },
		},
		.ddr6 = {
			.dq0 = { 15, 12, 14, 13, 10,  9, 11,  8, },
			.dq1 = {  0,  1,  2,  3,  5,  6,  4,  7, },
		},
		.ddr7 = {
			.dq0 = {  0,  3,  1,  2,  4,  5,  6,  7, },
			.dq1 = { 11,  8, 13, 14,  9, 12, 15, 10, },
		},
	},

	/* DQS CPU<>DRAM map */
	.lpx_dqs_map = {
		.ddr0 = { .dqs0 = 1, .dqs1 = 0 },
		.ddr1 = { .dqs0 = 1, .dqs1 = 0 },
		.ddr2 = { .dqs0 = 0, .dqs1 = 1 },
		.ddr3 = { .dqs0 = 0, .dqs1 = 1 },
		.ddr4 = { .dqs0 = 1, .dqs1 = 0 },
		.ddr5 = { .dqs0 = 0, .dqs1 = 1 },
		.ddr6 = { .dqs0 = 1, .dqs1 = 0 },
		.ddr7 = { .dqs0 = 0, .dqs1 = 1 },
	},

	.ect = 1, /* Enable Early Command Training */
};

static const struct mb_cfg hynix_memcfg = {
	.type = MEM_TYPE_LP4X,

	.rcomp = {
		/* Baseboard uses only 100ohm Rcomp resistors */
		.resistor = 100,

		/* Baseboard Rcomp target values */
		.targets = {40, 30, 30, 30, 30},
	},

	/* DQ byte map */
	.lpx_dq_map = {
		.ddr0 = {
			.dq0 = {  9, 11,  8, 10, 12, 14, 13, 15, },
			.dq1 = {  4,  7,  6,  5,  2,  3,  0,  1, },
		},
		.ddr1 = {
			.dq0 = { 15, 12, 14, 13,  9, 10, 11,  8, },
			.dq1 = {  0,  1,  3,  2,  7,  5,  4,  6, },
		},
		.ddr2 = {
			.dq0 = {  2,  3,  1,  0,  6,  7,  5,  4, },
			.dq1 = { 15,  9, 14,  8, 11, 10, 13, 12, },
		},
		.ddr3 = {
			.dq0 = {  3,  1,  2,  0,  4,  6,  7,  5, },
			.dq1 = { 13, 15, 14, 12, 11,  9,  8, 10, },
		},
		.ddr4 = {
			.dq0 = { 13, 12, 14, 15,  9,  8, 10, 11, },
			.dq1 = {  4,  7,  5,  6,  1,  2,  0,  3, },
		},
		.ddr5 = {
			.dq0 = {  5,  0,  6,  4,  3,  1,  7,  2, },
			.dq1 = { 11,  9, 10,  8, 15, 12, 14, 13, },
		},
		.ddr6 = {
			.dq0 = { 15, 12, 14, 13, 10,  9, 11,  8, },
			.dq1 = {  0,  1,  2,  3,  5,  6,  4,  7, },
		},
		.ddr7 = {
			.dq0 = {  0,  3,  1,  2,  4,  5,  6,  7, },
			.dq1 = { 11,  8, 13, 14,  9, 12, 15, 10, },
		},
	},

	/* DQS CPU<>DRAM map */
	.lpx_dqs_map = {
		.ddr0 = { .dqs0 = 1, .dqs1 = 0 },
		.ddr1 = { .dqs0 = 1, .dqs1 = 0 },
		.ddr2 = { .dqs0 = 0, .dqs1 = 1 },
		.ddr3 = { .dqs0 = 0, .dqs1 = 1 },
		.ddr4 = { .dqs0 = 1, .dqs1 = 0 },
		.ddr5 = { .dqs0 = 0, .dqs1 = 1 },
		.ddr6 = { .dqs0 = 1, .dqs1 = 0 },
		.ddr7 = { .dqs0 = 0, .dqs1 = 1 },
	},

	.ect = 1, /* Enable Early Command Training */

	.cs_pi_start_high_in_ect = 1
};

const struct mb_cfg *__weak variant_memory_params(void)
{
	const char *part_num = mainboard_get_dram_part_num();
	const char *hynix_mem1 = "H54G46CYRBX267";
	const char *hynix_mem2 = "H54G56CYRBX247";

	if (!strcmp(part_num, hynix_mem1) || !strcmp(part_num, hynix_mem2)) {
		printk(BIOS_INFO, "Enable cs_pi_start_high_in_ect for Hynix memory\n");
		return &hynix_memcfg;
	}
	return &baseboard_memcfg;
}

int __weak variant_memory_sku(void)
{
	/*
	 * Memory configuration board straps
	 * GPIO_MEM_CONFIG_0	GPP_E11
	 * GPIO_MEM_CONFIG_1	GPP_E2
	 * GPIO_MEM_CONFIG_2	GPP_E1
	 * GPIO_MEM_CONFIG_3	GPP_E12
	 */
	gpio_t spd_gpios[] = {
		GPP_E11,
		GPP_E2,
		GPP_E1,
		GPP_E12,
	};

	return gpio_base2_value(spd_gpios, ARRAY_SIZE(spd_gpios));
}

bool __weak variant_is_half_populated(void)
{
	/* GPIO_MEM_CH_SEL GPP_E13 */
	return gpio_get(GPP_E13);
}

void __weak variant_get_spd_info(struct mem_spd *spd_info)
{
	spd_info->topo = MEM_TOPO_MEMORY_DOWN;
	spd_info->cbfs_index = variant_memory_sku();
}
