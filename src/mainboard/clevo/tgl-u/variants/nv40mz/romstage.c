/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/meminit.h>
#include <variant/romstage.h>

void variant_configure_fspm(FSPM_UPD *memupd)
{
	const struct mb_cfg board_cfg = {
		.type = MEM_TYPE_DDR4,
	};
	const struct mem_spd spd_info = {
		.topo = MEM_TOPO_DIMM_MODULE,
		.smbus = {
			[0] = { .addr_dimm[0] = 0x50, },
			[1] = { .addr_dimm[0] = 0x52, },
		},
	};
	const bool half_populated = false;

	memcfg_init(memupd, &board_cfg, &spd_info, half_populated);
}
