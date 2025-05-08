/* SPDX-License-Identifier: GPL-2.0-only */

#include <smbios.h>

void smbios_fill_dimm_locator(const struct dimm_info *dimm,
	struct smbios_type17 *t)
{
	switch (dimm->ctrlr_num) {
	case 0:
		t->device_locator = smbios_add_string(t->eos, "RAM2");
		break;
	case 1:
		t->device_locator = smbios_add_string(t->eos, "RAM1");
		break;
	default:
		t->device_locator = smbios_add_string(t->eos, "UNKNOWN");
		break;
	}
}
