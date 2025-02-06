/* SPDX-License-Identifier: GPL-2.0-only */

#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>
#include <soc/gpio.h>
#include <variant/gpio.h>

#include <spd_bin.h>
#include <lib.h>

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
			.addr_dimm[0] = 0x50,
		},
	},
};

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	const struct pad_config *pads;
	size_t num;

	struct spd_block blk = {
		.addr_map = { 0x50 },
	};

	get_spd_smbus(&blk);
	if (blk.spd_array[0]) {
		printk(BIOS_DEBUG, "DIMM @ 0x%02x:\n", blk.addr_map[0]);
		hexdump(blk.spd_array[0], blk.len);
	}

	memcfg_init(memupd, &ddr5_mem_config, &dimm_module_spd_info, false);

	pads = board_gpio_table(&num);
	gpio_configure_pads(pads, num);

	// Set primary display to internal graphics
	memupd->FspmConfig.PrimaryDisplay = 0;
	memupd->FspmConfig.DmiMaxLinkSpeed = 4;
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[0] = 0;
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[1] = 0;
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[2] = 0;
}
