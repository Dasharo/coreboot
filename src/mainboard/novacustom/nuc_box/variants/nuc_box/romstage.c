/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/meminit.h>
#include <soc/romstage.h>

void mainboard_memory_init_params(FSPM_UPD *mupd)
{
	const struct mb_cfg board_cfg = {
		.type = MEM_TYPE_DDR5,
		.ect = true,
	};
	const struct mem_spd spd_info = {
		.topo = MEM_TOPO_DIMM_MODULE,
		.smbus = {
			[0] = { .addr_dimm[0] = 0x50, },
			[1] = { .addr_dimm[0] = 0x52, },
		},
	};
	mupd->FspmConfig.DmiMaxLinkSpeed = 4;

	/*
	 * The FSP POST codes have to be routed to port 80 on the NUC.
	 * When incorrectly routed to I2C, they increase FSP execution time many
	 * times over.
	 */
	mupd->FspmConfig.I2cPostCodeEnable = 0;

	memcfg_init(mupd, &board_cfg, &spd_info, false);
}
