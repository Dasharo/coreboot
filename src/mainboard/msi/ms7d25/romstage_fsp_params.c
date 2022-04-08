/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <console/console.h>
#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>

static const struct mb_cfg ddr4_mem_config = {
	.type = MEM_TYPE_DDR4,

	/* According to DOC #573387 rcomp no longer have to be provided */
	.rcomp = {
		/* Baseboard uses only 100ohm Rcomp resistor */
		.resistor = 0,

		/* Baseboard Rcomp target values */
		.targets = { 0 },
	},
	/* DDR DIMM configuration does not need to set DQ/DQS maps */
	.dq_map = { 0 },
	.dqs_map = { 0 },

	.ect = true, /* Early Command Training */

	.UserBd = BOARD_TYPE_DESKTOP_2DPC,

	.LpDdrDqDqsReTraining = 0,	/* LPDDR only, set to 0 */

	.CmdMirror = 0x33,

	.ddr_config = {
		.dq_pins_interleaved = true,
	},
};

static const struct mem_spd dimm_module_spd_info = {
	.topo = MEM_TOPO_DIMM_MODULE,
	.smbus = {
		[0] = {
			.addr_dimm[0] = 0x50,
			.addr_dimm[1] = 0x51,
		},
		[1] = {
			.addr_dimm[0] = 0x52,
			.addr_dimm[1] = 0x53,
		},
	},
};

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	memupd->FspmConfig.FirstDimmBitMask = 0xA;

	memcfg_init(memupd, &ddr4_mem_config, &dimm_module_spd_info, false);
}
