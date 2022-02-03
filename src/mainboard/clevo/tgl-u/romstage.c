/* SPDX-License-Identifier: GPL-2.0-only */

#include <option.h>
#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>
#include <baseboard/variants.h>

static const struct mb_cfg variant_mem_config = {
	.type = MEM_TYPE_DDR4,
	.ect = false, /* Early Command Training */
	.ddr4_config = {
		.dq_pins_interleaved = false,
	}
};

static const struct mem_spd variant_spd_info = {
	.topo = MEM_TOPO_DIMM_MODULE,
	.smbus = {
		[0] = { .addr_dimm[0] = 0x50, },
		[1] = { .addr_dimm[0] = 0x52, },
	},
};

void variant_configure_fspm(FSPM_UPD *memupd)
{
	const struct mb_cfg *mem_config = &variant_mem_config;
	const struct mem_spd *spd_info = &variant_spd_info;
	const bool half_populated = false;

	memcfg_init(&memupd->FspmConfig, mem_config, spd_info, half_populated);

	const uint8_t vtd = get_uint_option("vtd", 1);
	memupd->FspmConfig.VtdDisable = !vtd;

	const uint8_t ht = get_uint_option("hyper_threading",
		memupd->FspmConfig.HyperThreading);
	memupd->FspmConfig.HyperThreading = ht;
}
