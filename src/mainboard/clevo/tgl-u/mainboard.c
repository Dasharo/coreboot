/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <device/device.h>
#include <device/i2c_simple.h>
#include <intelblocks/lpc_lib.h>
#include <soc/gpio.h>
#include <soc/ramstage.h>
#include <smbios.h>
#include <string.h>

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	/* Disable AER for the SSD slot to support SSDs with buggy firmware */
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

static void mainboard_prune_touchpads(void *unused)
{
	struct device *child, *i2c_dev = pcidev_on_root(0x15, 0);
	uint8_t data;

	/*
	 * NS5x comes with different touchpads from various vendors. Windows
	 * gets confused when multiple touchpads are defined on the same I2C bus
	 * in devicetree, so detect which touchpad variant is actually attached
	 * to the mainboard and disable the other touchpads.
	 */
	for (child = i2c_dev->link_list->children; child; child = child->sibling)
		if (child->path.i2c.device)
			child->enabled = !(i2c_readb(0, child->path.i2c.device, 0, &data));
}

BOOT_STATE_INIT_ENTRY(BS_POST_DEVICE, BS_ON_EXIT, mainboard_prune_touchpads, NULL);

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
};
