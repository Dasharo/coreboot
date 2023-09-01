/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/romstage.h>
#include <variant/romstage.h>
#include <smmstore.h>
#include <device/device.h>
#include <drivers/efi/efivars.h>

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	variant_configure_fspm(memupd);

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
	        printk(BIOS_DEBUG, "Wifi + BT radios: failed to read curve selection from EFI vars, using board default\n");
efi_err: ;
#endif
	if(!enable) {
		uint32_t mask = memupd->FspmConfig.PcieRpEnableMask;
		// disable PCIe root port 5
		uint32_t disabled_root_port = ~((uint32_t)1 << 4);
		memupd->FspmConfig.PcieRpEnableMask = mask & disabled_root_port;
	}
}

