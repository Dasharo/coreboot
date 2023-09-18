/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/ramstage.h>
#include <variant/ramstage.h>
#include <device/device.h>
#include <smmstore.h>
#include <ec/system76/ec/commands.h>

#include <drivers/efi/efivars.h>

static bool get_wifi_bt_enable(void)
{
	bool enable = true;

#if CONFIG(DRIVERS_EFI_VARIABLE_STORE)
	struct region_device rdev;
	enum cb_err ret;

	uint32_t size = sizeof(enable);

	const EFI_GUID dasharo_system_features_guid = {
		0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
	};

	if (smmstore_lookup_region(&rdev))
		goto efi_err;

	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "EnableWifiBt",
				&enable, &size);

	if (ret != CB_SUCCESS)
			printk(BIOS_DEBUG, "Wifi + BT radios: failed to read selection from EFI vars, using board default\n");
efi_err:
#endif

	return enable;
}

void variant_configure_fsps(FSP_S_CONFIG *params)
{
	/*
	 * Disable AER for the SSD slot to support S0ix with SSDs running
	 * buggy firmware
	 */
	params->CpuPcieRpAdvancedErrorReporting[0] = 0;
	params->CpuPcieRpLtrEnable[0] = 1;
	params->CpuPcieRpSlotImplemented[0] = 1;

	params->CnviBtCore = get_wifi_bt_enable();
}

void variant_init() {
	bool radio_enable;

	config_t *cfg = config_of_soc();
	struct device *cnvi_dev = pcidev_on_root(0x14, 3);
	struct device *wlan_dev = pcidev_on_root(0x1c, 7);

	radio_enable = get_wifi_bt_enable();

	printk(BIOS_DEBUG, "Wireless is %sabled\n", radio_enable ? "en" : "dis");

	wlan_dev->enabled = radio_enable;
	cnvi_dev->enabled = radio_enable;
	cfg->usb2_ports[9].enable = radio_enable;

	system76_ec_smfi_cmd(CMD_WIFI_BT_ENABLEMENT_SET, 1, (uint8_t *)&radio_enable);
}
