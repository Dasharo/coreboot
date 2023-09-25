/* SPDX-License-Identifier: GPL-2.0-only */

#include <dasharo/options.h>
#include <soc/ramstage.h>
#include <variant/ramstage.h>
#include <device/device.h>
#include <ec/system76/ec/commands.h>

void variant_configure_fsps(FSP_S_CONFIG *params)
{
	/*
	 * Disable AER for the SSD slot to support S0ix with SSDs running
	 * buggy firmware
	 */
	params->CpuPcieRpAdvancedErrorReporting[0] = 0;
	params->CpuPcieRpLtrEnable[0] = 1;
	params->CpuPcieRpSlotImplemented[0] = 1;
}

void variant_init(void)
{
	config_t *cfg = config_of_soc();
	struct device *cnvi_dev = pcidev_on_root(0x14, 3);
	struct device *wlan_dev = pcidev_on_root(0x1c, 7);
	bool radio_enable = get_wireless_option();

	printk(BIOS_DEBUG, "Wireless is %sabled\n", radio_enable ? "en" : "dis");

	wlan_dev->enabled = radio_enable;
	cnvi_dev->enabled = radio_enable;
	cfg->usb2_ports[9].enable = radio_enable;

	system76_ec_smfi_cmd(CMD_WIFI_BT_ENABLEMENT_SET, 1, (uint8_t *)&radio_enable)
}
