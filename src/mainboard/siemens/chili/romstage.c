/* SPDX-License-Identifier: GPL-2.0-only */

#include <option.h>
#include <soc/cnl_memcfg_init.h>
#include <soc/gpio.h>
#include <soc/romstage.h>

#include "variant.h"

static void mainboard_init(void)
{
	const struct pad_config *pads;
	size_t num;

	pads = variant_gpio_table(&num);
	gpio_configure_pads(pads, num);
}

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	uint8_t vtd = 1;
	const struct cnl_mb_cfg cfg = {
		.spd = {
			[0] = { READ_SMBUS, { 0x50 << 1 } },
			[2] = { READ_SMBUS, { 0x52 << 1 } },
		},
		.rcomp_resistor = { 121, 75, 100 },
		.rcomp_targets = { 50, 25, 20, 20, 26 },
		.dq_pins_interleaved = 1,
		.vref_ca_config = 2,
		.ect = 0,
	};
	cannonlake_memcfg_init(&memupd->FspmConfig, &cfg);
	memupd->FspmConfig.EccSupport = 1;
	memupd->FspmConfig.UserBd = BOARD_TYPE_MOBILE;

	get_option(&vtd, "vtd");
	memupd->FspmTestConfig.VtdDisable = !vtd;
	get_option(&memupd->FspmConfig.HyperThreading, "hyper_threading");

	variant_romstage_params(memupd);

	mainboard_init();
}

__weak void variant_romstage_params(FSPM_UPD *const mupd)
{
}
