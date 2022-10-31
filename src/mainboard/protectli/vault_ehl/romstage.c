/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <console/console.h>
#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>
#include "gpio.h"

static const struct mb_cfg ddr4_mem_config = {
	.UserBd = BOARD_TYPE_DESKTOP, /* FIXME */
	.dq_pins_interleaved = 0x1,
	.vref_ca_config = 0x0,
	.ect = 0x0,
};

static const struct spd_info module_spd_info = {
	.read_type = READ_SMBUS,
};

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	FSP_M_CONFIG *mem_cfg = &memupd->FspmConfig;

	mem_cfg->SkipExtGfxScan = 0x0;
	mem_cfg->PchHdaAudioLinkHdaEnable = 0x1;
	mem_cfg->PchHdaSdiEnable[0] = 0x1;

	memcfg_init(mem_cfg, &ddr4_mem_config, &module_spd_info, false);
	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
}
