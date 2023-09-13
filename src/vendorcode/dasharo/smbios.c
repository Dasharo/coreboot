/* SPDX-License-Identifier: GPL-2.0-only */

#include <smbios.h>
#include <string.h>

const char *smbios_mainboard_bios_version(void)
{

	if (CONFIG(PAYLOAD_SEABIOS)) {
		if (strlen(CONFIG_LOCALVERSION) != 0)
			return "Dasharo (coreboot+SeaBIOS) " CONFIG_LOCALVERSION;
		else
			return "Dasharo (coreboot+SeaBIOS)";
	}

	if (CONFIG(PAYLOAD_EDK2)) {
		if (strlen(CONFIG_LOCALVERSION) != 0)
			return "Dasharo (coreboot+UEFI) " CONFIG_LOCALVERSION;
		else
			return "Dasharo (coreboot+UEFI)";
	}

	if (strlen(CONFIG_LOCALVERSION) != 0)
		return "Dasharo " CONFIG_LOCALVERSION;

	return "Dasharo";
}
