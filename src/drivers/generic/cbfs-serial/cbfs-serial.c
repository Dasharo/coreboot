/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <device/device.h>
#include <drivers/efi/efivars.h>
#include <smmstore.h>
#include <smbios.h>
#include <string.h>

#include <Uefi/UefiBaseType.h>

#define MAX_SERIAL_LENGTH 0x100

static bool get_serial_number_from_efivar(char *serial_number)
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

	size = MAX_SERIAL_LENGTH;
	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "Type2SN", serial_number, &size);
	if (ret != CB_SUCCESS || size > MAX_SERIAL_LENGTH)
		return false;

	serial_number[size] = '\0';

	return true;
#else
	/* In absence of EFI variables, follow compilation options */
	return false;
#endif
}

const char *smbios_mainboard_serial_number(void)
{
	static char serial_number[MAX_SERIAL_LENGTH + 1] = {0};

	if (serial_number[0] != 0)
		return serial_number;

	size_t serial_len = cbfs_load("serial_number", serial_number, MAX_SERIAL_LENGTH);
	if (serial_len) {
		serial_number[serial_len] = '\0';
		return serial_number;
	}

	if (get_serial_number_from_efivar(serial_number))
		return serial_number;

	strncpy(serial_number, CONFIG_MAINBOARD_SERIAL_NUMBER,
			MAX_SERIAL_LENGTH);

	return serial_number;
}
