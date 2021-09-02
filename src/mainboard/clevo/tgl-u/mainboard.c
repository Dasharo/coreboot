/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <device/device.h>
#include <intelblocks/lpc_lib.h>
#include <soc/gpio.h>
#include <smbios.h>
#include <string.h>

static void mainboard_init(void *chip_info)
{
	const struct pad_config *pads;
	size_t num;

	pads = variant_gpio_table(&num);
	gpio_configure_pads(pads, num);

	/* Configure MMIO window before FSP-S locks the DMI registers */
	lpc_open_mmio_window(CONFIG_EC_CLEVO_IT5570_RAM_BASE);
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
};
