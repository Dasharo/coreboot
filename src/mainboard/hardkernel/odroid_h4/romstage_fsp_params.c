/* SPDX-License-Identifier: GPL-2.0-only */

#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>
#include <soc/gpio.h>

#include "gpio.h"

static const struct mb_cfg ddr5_mem_config = {
	.type = MEM_TYPE_DDR5,
	.ect = true, /* Early Command Training */
	.UserBd = BOARD_TYPE_MOBILE,
	.LpDdrDqDqsReTraining = 1,
};

static const struct mem_spd dimm_module_spd_info = {
	.topo = MEM_TOPO_DIMM_MODULE,
	.smbus = {
		[0] = {
			.addr_dimm[0] = 0x52,
		},
	},
};

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	const struct pad_config *pads;
	size_t num;

	memcfg_init(memupd, &ddr5_mem_config, &dimm_module_spd_info, false);

	pads = board_gpio_table(&num);
	gpio_configure_pads(pads, num);

	memupd->FspmConfig.DmiMaxLinkSpeed = 4;
}
