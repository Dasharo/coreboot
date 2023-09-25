/* SPDX-License-Identifier: GPL-2.0-only */

#include <dasharo/options.h>
#include <bootstate.h>
#include <console/console.h>
#include <delay.h>
#include <gpio.h>
#include <soc/ramstage.h>
#include <soc/gpio.h>
#include <variant/ramstage.h>
#include <device/device.h>
#include <ec/system76/ec/commands.h>

#define DGPU_RST_N GPP_U4
#define DGPU_PWR_EN GPP_U5

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

static void dgpu_power_enable(int onoff) {
	printk(BIOS_DEBUG, "nvidia: DGPU power %d\n", onoff);
	if (onoff) {
		gpio_set(DGPU_RST_N, 0);
		mdelay(4);
		gpio_set(DGPU_PWR_EN, 1);
		mdelay(4);
		gpio_set(DGPU_RST_N, 1);
	} else {
		gpio_set(DGPU_RST_N, 0);
		mdelay(4);
		gpio_set(DGPU_PWR_EN, 0);
	}
	mdelay(50);
}

static void mainboard_pre_device(void *unused) {
	dgpu_power_enable(1);
}

BOOT_STATE_INIT_ENTRY(BS_PRE_DEVICE, BS_ON_ENTRY, mainboard_pre_device, NULL);

void variant_init() {
	config_t *cfg = config_of_soc();
	struct device *cnvi_dev = pcidev_on_root(0x14, 3);
	struct device *wlan_dev = pcidev_on_root(0x1d, 2);
	bool radio_enable = get_wireless_option();

	printk(BIOS_DEBUG, "Wireless is %sabled\n", radio_enable ? "en" : "dis");

	wlan_dev->enabled = radio_enable;
	cnvi_dev->enabled = radio_enable;
	cfg->usb2_ports[9].enable = radio_enable;

	system76_ec_smfi_cmd(CMD_WIFI_BT_ENABLEMENT_SET, 1, (uint8_t *)&radio_enable);
}
