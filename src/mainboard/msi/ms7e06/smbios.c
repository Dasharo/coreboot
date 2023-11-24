/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <fmap.h>
#include <smbios.h>
#include <soc/pci_devs.h>
#include <string.h>

#include "romhole.h"

struct smbios_type2_v3_4 {
	struct smbios_header header;
	u8 manufacturer;
	u8 product_name;
	u8 version;
	u8 serial_number;
	u8 asset_tag;
	u8 feature_flags;
	u8 location_in_chassis;
	u16 chassis_handle;
	u8 board_type;
	u8 contained_objects;
	u8 eos[2];
} __packed;

static struct msi_romhole *romhole_addr = NULL;

static struct msi_romhole *get_msi_romhole_address(void)
{
	struct region_device rdev;

	if (romhole_addr)
		return romhole_addr;

	if (fmap_locate_area_as_rdev("ROMHOLE", &rdev))
		return NULL;

	romhole_addr = (struct msi_romhole *)rdev_mmap_full(&rdev);

	return romhole_addr;
}

u8 smbios_mainboard_feature_flags(void)
{
	return SMBIOS_FEATURE_FLAGS_HOSTING_BOARD | SMBIOS_FEATURE_FLAGS_REPLACEABLE;
}

smbios_wakeup_type smbios_system_wakeup_type(void)
{
	return SMBIOS_WAKEUP_TYPE_POWER_SWITCH;
}

const char *smbios_system_product_name(void)
{
	return "MS-7E06";
}

const char *smbios_mainboard_product_name(void)
{
	if (CONFIG(BOARD_MSI_Z790_P_PRO_WIFI_DDR4)) {
		if (is_devfn_enabled(PCH_DEVFN_CNVI_WIFI))
			return "PRO Z790-P WIFI DDR4 (MS-7E06)";
		else
			return "PRO Z790-P DDR4 (MS-7E06)";
	}

	if (CONFIG(BOARD_MSI_Z790_P_PRO_WIFI_DDR5)) {
		if (is_devfn_enabled(PCH_DEVFN_CNVI_WIFI))
			return "PRO Z790-P WIFI (MS-7E06)";
		else
			return "PRO Z790-P (MS-7E06)";
	}

	return CONFIG_MAINBOARD_PART_NUMBER;
}

/* Only baseboard serial number is populated */
const char *smbios_system_serial_number(void)
{
	return "Default string";
}

const char *smbios_system_sku(void)
{
	return "Default string";
}

void smbios_system_set_uuid(u8 *uuid)
{
	if (!get_msi_romhole_address())
		return;

	memcpy(uuid, romhole_addr->uuid.uuid, 16);
}

static const char* get_smbios_string(u8 *str_table_start, u8 string_index,
				     const char *fallback)
{
	u8 i;
	char *smbios_str = (char *)str_table_start;

	for (i = 1; i < string_index; i++) {
		if (strlen(smbios_str))
			smbios_str += strlen(smbios_str) + 1;
		else
			return fallback;
	}

	return strlen(smbios_str) ? (const char *)smbios_str : fallback;
}

const char *smbios_mainboard_serial_number(void)
{
	struct smbios_type2_v3_4 *type2;

	if (!get_msi_romhole_address())
		return CONFIG_MAINBOARD_SERIAL_NUMBER;

	type2 = (struct smbios_type2_v3_4 *)romhole_addr->t2rw.smbios_type2;
	/* Do some sanity checks first */
	if (type2->header.type != 2 ||
	    type2->header.length == 0 ||
	    type2->header.length == 0xff ||
	    smbios_string_table_len(type2->eos) == 0)
		return CONFIG_MAINBOARD_SERIAL_NUMBER;

	return get_smbios_string(type2->eos, type2->serial_number,
				 CONFIG_MAINBOARD_SERIAL_NUMBER);
}

const char *smbios_mainboard_version(void)
{
	struct smbios_type2_v3_4 *type2;

	if (!get_msi_romhole_address())
		return CONFIG_MAINBOARD_VERSION;

	type2 = (struct smbios_type2_v3_4 *)romhole_addr->t2rw.smbios_type2;
	/* Do some sanity checks first */
	if (type2->header.type != 2 ||
	    type2->header.length == 0 ||
	    type2->header.length == 0xff ||
	    smbios_string_table_len(type2->eos) == 0)
		return CONFIG_MAINBOARD_VERSION;

	return get_smbios_string(type2->eos, type2->version,
				 CONFIG_MAINBOARD_VERSION);
}
