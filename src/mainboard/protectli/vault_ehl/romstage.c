/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>

static const struct mb_cfg ddr4_mem_config = {
	.UserBd = BOARD_TYPE_DESKTOP,
	.dq_pins_interleaved = 0x1,
	.vref_ca_config = 0x2,
	.ect = 0x1,
};

static const struct spd_info module_spd_info = {
	.read_type = READ_SMBUS,
	.spd_spec.spd_smbus_address = {0x50, 0x0, 0x0, 0x0},
};

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	FSP_M_CONFIG *mem_cfg = &memupd->FspmConfig;

	memcfg_init(mem_cfg, &ddr4_mem_config, &module_spd_info, false);
}
