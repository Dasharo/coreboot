/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <console/console.h>
#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>
#include <baseboard/variants.h>

void mainboard_memory_init_params(FSPM_UPD *mupd)
{
	FSP_M_CONFIG *mem_cfg = &mupd->FspmConfig;

	const struct mb_cfg *mem_config = variant_memory_params();
	const struct mem_spd spd_info = {
		.topo = MEM_TOPO_MEMORY_DOWN,
	};
	bool half_populated = false;

	memcfg_init(mem_cfg, mem_config, &spd_info, half_populated);

}
