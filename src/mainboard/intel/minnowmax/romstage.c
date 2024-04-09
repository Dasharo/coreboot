/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2013 Sage Electronic Engineering, LLC.
 * Copyright (C) 2014 Intel Corporation
 * Copyright (C) 2018 CMR Surgical Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>
#include <cbfs.h>
#include <console/console.h>
#include <cpu/x86/tsc.h>
#include <soc/gpio.h>
#include <soc/mrc_wrapper.h>
#include <soc/romstage.h>

void mainboard_fill_mrc_params(struct mrc_params *mp)
{
	uint8_t *spd_file;
	void *spd_content;
	size_t spd_fsize;
	u8 gpio34 = 0;
	u8 is_1333_sku;

	/*
	 * The E3827 and E3845 SKUs are fused at 1333MHz DDR3 speeds. There's
	 * no good way of knowing the SKU'ing so frequency is used as a proxy.
	 * The E3805, E3815, E3825, and E3826 are all <= 1460MHz while the
	 * E3827 and E3845 are 1750MHz and 1910MHz, respectively.
	 */
	is_1333_sku = !!(tsc_freq_mhz() >= 1700);

	printk(BIOS_INFO, "Using %d MHz DDR3 settings.\n",
		is_1333_sku ? 1333 : 1066);

	/*
	 * Minnow Max Board
	 * Read SSUS gpio 5 (34) to determine memory type
	 *                    0 : 1GB SKU uses 2Gb density memory
	 *                    1 : 2GB SKU uses 4Gb density memory
	 *
	 * devicetree.cb assumes 1GB SKU board
	 */
	setup_ssus_gpio(34, PAD_FUNC0 | PAD_PULL_DISABLE, PAD_VAL_INPUT);

	gpio34 = ssus_get_gpio(34);
	if (gpio34)
		printk(BIOS_NOTICE, "%s GB Minnowboard Max detected.\n",
				    gpio34 ? "2 / 4" : "1");

	spd_file = (uint8_t *)cbfs_map("spd.bin", &spd_fsize);
	if (!spd_file)
		die("SPD data not found.");

	spd_content = &spd_file[CONFIG_DIMM_SPD_SIZE * gpio34];

	mp->mainboard.dram_type = DRAM_DDR3L;
	mp->mainboard.dram_info_location = DRAM_INFO_SPD_MEM,
	mp->txe_size_mb = 16;

	mp->mainboard.dram_data[0] = spd_content;
	mp->mainboard.dram_data[1] = NULL;
}
