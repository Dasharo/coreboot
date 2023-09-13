/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/romstage.h>
#include <variant/romstage.h>
#include <smmstore.h>
#include <device/device.h>
#include <drivers/efi/efivars.h>
#include <ec/system76/ec/commands.h>

//bool is_wifi_bt_radios_enabled(void);
//
//bool is_wifi_bt_radios_enabled(void)
//{
//	bool enable = true;
//
//#if CONFIG(DRIVERS_EFI_VARIABLE_STORE)
//	struct region_device rdev;
//	enum cb_err ret;
//
//	uint32_t size = sizeof(enable);
//
//	const EFI_GUID dasharo_system_features_guid = {
//		0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
//	};
//
//	if (smmstore_lookup_region(&rdev))
//		goto efi_err;
//
//	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "EnableWifiBt",
//	                        &enable, &size);
//
//	if (ret != CB_SUCCESS)
//	        printk(BIOS_DEBUG, "Wifi + BT radios: failed to read selection from EFI vars, using board default\n");
//efi_err: ;
//#endif
//	return enable;
//}

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	variant_configure_fspm(memupd);

//	bool wifi_bt_radios_enabled = is_wifi_bt_radios_enabled();
//	system76_ec_smfi_cmd(CMD_WIFI_BT_ENABLEMENT_SET, sizeof(wifi_bt_radios_enabled) / sizeof(uint8_t), (uint8_t *)&wifi_bt_radios_enabled);
//	printk(BIOS_DEBUG, wifi_bt_radios_enabled ? "---WIFI ENABLED:)\n" : "---WIFI DISABLED:(\n");
}

