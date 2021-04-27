/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <baseboard/variants.h>
#include <intelblocks/mp_init.h>

size_t __weak variant_memory_sku(void)
{
	return 0;
}

static const struct mb_ddr4_cfg mem_config = {
	.dq_pins_interleaved = 0,
	.ect = 1, /* Early Command Training */
};

static const struct ddr_memory_cfg board_mem_config = {
	.mem_type = MEMTYPE_DDR4,
	.ddr4_cfg = &mem_config,

};

const struct ddr_memory_cfg  *__weak variant_memory_params(void)
{
	return &board_mem_config;
}

const struct spd_info __weak variant_spd_info(void)
{	
	const struct spd_info spd_info = {
		.topology = SODIMM,
		.smbus_info[0] = {.addr_dimm0 = 0x50,
					.addr_dimm1 = 0 },
		.smbus_info[1] = {.addr_dimm0 = 0x52,
					.addr_dimm1 = 0 },
	};

	return spd_info;
}