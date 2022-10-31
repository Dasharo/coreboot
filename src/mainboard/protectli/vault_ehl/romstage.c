/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <types.h>
#include <soc/meminit.h>
#include <soc/romstage.h>


static void fill_ddr4_params(FSP_M_CONFIG *cfg)
{
}

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	FSP_M_CONFIG *cfg = &memupd->FspmConfig;
	fill_ddr4_params(cfg);
}
