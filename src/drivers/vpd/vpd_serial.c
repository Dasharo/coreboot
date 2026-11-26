/* SPDX-License-Identifier: BSD-3-Clause */

#include <dasharo/options.h>
#include <smbios.h>

#include "vpd.h"
#include "vpd_tables.h"

#define VPD_KEY_SYSTEM_SERIAL		"serial_number"
#define VPD_KEY_MAINBOARD_SERIAL	"mlb_serial_number"
/* get_serial_number_from_efivar() writes up to 0x100 characters plus NUL */
#define VPD_SERIAL_LEN			(0x100 + 1)

static bool vpd_get_serial(const char *key, char *serial)
{
	return vpd_gets(key, serial, VPD_SERIAL_LEN, VPD_RO) && serial[0] != '\0';
}

/* Units provisioned before the move to VPD only have the serial in Type2SN */
static const char *serial_fallback(char *serial)
{
	if (get_serial_number_from_efivar(serial))
		return serial;

	return CONFIG_MAINBOARD_SERIAL_NUMBER;
}

const char *smbios_system_serial_number(void)
{
	static char serial[VPD_SERIAL_LEN];

	if (vpd_get_serial(VPD_KEY_SYSTEM_SERIAL, serial))
		return serial;

	return serial_fallback(serial);
}

const char *smbios_mainboard_serial_number(void)
{
	static char serial[VPD_SERIAL_LEN];

	if (vpd_get_serial(VPD_KEY_MAINBOARD_SERIAL, serial) ||
	    vpd_get_serial(VPD_KEY_SYSTEM_SERIAL, serial))
		return serial;

	return serial_fallback(serial);
}
