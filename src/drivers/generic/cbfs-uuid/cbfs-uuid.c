/* SPDX-License-Identifier: GPL-2.0-only */

#include <lib.h>
#include <cbfs.h>
#include <console/console.h>
#include <device/device.h>
#include <drivers/efi/efivars.h>
#include <smmstore.h>
#include <smbios.h>
#include <string.h>
#include <uuid.h>

#include <Uefi/UefiBaseType.h>


static bool get_uuid_from_efivar(uint8_t *uuid)
{
#if CONFIG(DRIVERS_EFI_VARIABLE_STORE)
	struct region_device rdev;
	enum cb_err ret;
	uint32_t size;
	const EFI_GUID dasharo_system_features_guid = {
		0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
	};
	if (smmstore_lookup_region(&rdev))
		return false;

	size = UUID_LEN;
	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "Type1UUID", uuid, &size);
	if (ret != CB_SUCCESS || size != UUID_LEN)
		return false;

	return true;
#else
	/* In absence of EFI variables, follow compilation options */
	return false;
#endif
}

void smbios_system_set_uuid(u8 *uuid)
{
	/* Add 3 more bytes: 2 for possible CRLF and third for NULL char */
	char uuid_str[UUID_STRLEN + 3] = {0};
	uint8_t system_uuid[UUID_LEN];

	size_t uuid_len = cbfs_load("system_uuid", uuid_str, UUID_STRLEN);

	if (uuid_len >= UUID_STRLEN && uuid_len <= UUID_STRLEN + 3) {
		/* Cut off any trailing whitespace like CR or LF */
		uuid_str[UUID_STRLEN] = '\0';
		if (!parse_uuid(system_uuid, uuid_str)) {
			memcpy(uuid, system_uuid, UUID_LEN);
			return;
		}
	}

	if (get_uuid_from_efivar(system_uuid))
		memcpy(uuid, system_uuid, UUID_LEN);
}
