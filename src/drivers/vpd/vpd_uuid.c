/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <dasharo/options.h>
#include <smbios.h>
#include <string.h>
#include <uuid.h>

#include "vpd.h"

#define VPD_KEY_SYSTEM_UUID	"system_uuid"

void smbios_system_set_uuid(u8 *uuid)
{
	char uuid_str[UUID_STRLEN + 1];
	uint8_t system_uuid[UUID_LEN];

	if (vpd_gets(VPD_KEY_SYSTEM_UUID, uuid_str, sizeof(uuid_str), VPD_RO)) {
		if (!parse_uuid(system_uuid, uuid_str)) {
			memcpy(uuid, system_uuid, UUID_LEN);
			return;
		}
		printk(BIOS_WARNING, "VPD: invalid %s: '%s'\n", VPD_KEY_SYSTEM_UUID, uuid_str);
	}

	/* Units provisioned before the move to VPD only have the UUID in Type1UUID */
	if (get_uuid_from_efivar(system_uuid))
		memcpy(uuid, system_uuid, UUID_LEN);
}
