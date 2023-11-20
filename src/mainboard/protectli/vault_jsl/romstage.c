/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <soc/gpio.h>
#include <soc/meminit.h>
#include <soc/romstage.h>

#include "gpio.h"

static const struct mb_cfg memcfg_cfg = {

	/* Static array from the worksheet */
	.dq_map[DDR_CH0] = {
		{0xf, 0xf0},
		{0xf, 0xf0},
		{0xff, 0x0},
		{0x0, 0x0},
		{0x0, 0x0},
		{0x0, 0x0}
	},
	.dq_map[DDR_CH1] = {
		{0xf, 0xf0},
		{0xf, 0xf0},
		{0xff, 0x0},
		{0x0, 0x0},
		{0x0, 0x0},
		{0x0, 0x0}
	},

	/* Copied from the worksheet after filling DQS map */
	.dqs_map[DDR_CH0] = {0, 1, 2, 3, 4, 5, 6, 7},
	.dqs_map[DDR_CH1] = {0, 1, 2, 3, 4, 5, 6, 7},

	/* Copied from the worksheet for JSL LPDDR4X */
	.rcomp_resistor = {100, 100, 100},
	.rcomp_targets = {60, 40, 30, 20, 30},

	/* Enable Early Command Training */
	.ect = 1,

	/* User Board Type */
	.UserBd = BOARD_TYPE_ULT_ULX,
};

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	const struct spd_info board_spd_info = {
		.read_type = READ_SPD_CBFS,
		.spd_spec.spd_index = CONFIG(BOARD_PROTECTLI_V1210) ? 0 : 1,
	};

	memcfg_init(&memupd->FspmConfig, &memcfg_cfg, &board_spd_info, false);

	/* V1210 and V1410 populates only channel 1 */
	if (!CONFIG(BOARD_PROTECTLI_V1610))
		memupd->FspmConfig.MemorySpdPtr00 = 0;

	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
}
