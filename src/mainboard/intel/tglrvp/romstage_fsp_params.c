/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <console/console.h>
#include <fsp/api.h>
#include <soc/romstage.h>
#include <spd_bin.h>
#include <soc/meminit.h>
#include <baseboard/variants.h>

void mainboard_memory_init_params(FSPM_UPD *mupd)
{
	FSP_M_CONFIG *mem_cfg = &mupd->FspmConfig;

	mem_cfg->DciUsb3TypecUfpDbg = 0;
	mem_cfg->VtdItbtEnable = 0;
	mem_cfg->UsbTcPortEnPreMem = 0;
	

	const struct ddr_memory_cfg *mem_config = variant_memory_params();
	const struct spd_info spd_info = variant_spd_info();
	bool half_populated = false;

	meminit_ddr(mem_cfg, mem_config, &spd_info, half_populated);

}
