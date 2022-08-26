/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <soc/ramstage.h>
#include <variant/gpio.h>
#include <variant/ramstage.h>
#include <smbios.h>
#include <pc80/keyboard.h>

const char *smbios_system_sku(void)
{
	return "Not Applicable";
}

smbios_enclosure_type smbios_mainboard_enclosure_type(void)
{
	return SMBIOS_ENCLOSURE_NOTEBOOK;
}

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	variant_configure_fsps(params);
}

static void init_mainboard(void *chip_info)
{
	variant_configure_gpios();

	pc_keyboard_init(NO_AUX_DEVICE);
}

struct chip_operations mainboard_ops = {
	.init = init_mainboard,
};
