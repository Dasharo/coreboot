/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <device/device.h>
#include <intelblocks/lpc_lib.h>
#include <soc/gpio.h>
#include <soc/ramstage.h>
#include <smbios.h>
#include <string.h>

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	/* Disable AER for the SSD slot to support S0ix with SSDs containing buggy firmware */
	params->CpuPcieRpAdvancedErrorReporting[0] = 0;
}

const char *smbios_system_sku(void)
{
	return "Not Applicable";
}

smbios_enclosure_type smbios_mainboard_enclosure_type(void)
{
	return SMBIOS_ENCLOSURE_NOTEBOOK;
}

static void mainboard_init(void *chip_info)
{
	const struct pad_config *pads;
	size_t num;

	pads = variant_gpio_table(&num);
	gpio_configure_pads(pads, num);

	/* Configure MMIO window before FSP-S locks the DMI registers */
	lpc_open_mmio_window(CONFIG_EC_CLEVO_IT5570_RAM_BASE, 0x10000);
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
};
