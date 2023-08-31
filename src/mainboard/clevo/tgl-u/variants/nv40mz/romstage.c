/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/meminit.h>
#include <variant/romstage.h>
#include <device/device.h>
#include <drivers/efi/efivars.h>
#include <ec/acpi/ec.h>
#include <ec/system76/ec/commands.h>
#include <fmap.h>
#include <lib.h>
#include <security/vboot/vboot_common.h>
#include <smbios.h>
#include <smmstore.h>
#include <soc/ramstage.h>
#include <variant/gpio.h>
#include <variant/ramstage.h>
#include <Uefi/UefiBaseType.h>

#include <chip.h>
#include <cpu/intel/turbo.h>
#include <device/pci_def.h>
#include <option.h>
#include <types.h>
//static void set_wifi_bt_pcie_root_ports(void);

static void set_wifi_bt_pcie_root_ports(void) {
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
//	struct device* wifi_card = pcidev_on_root(0x14, 3);
//	wifi_card->enabled = enable;
//	printk(BIOS_DEBUG, enable ? "POZDRO\n" : "ESSA\n");
}

void variant_configure_fspm(FSPM_UPD *memupd)
{
	const struct mb_cfg board_cfg = {
		.type = MEM_TYPE_DDR4,
	};
	const struct mem_spd spd_info = {
		.topo = MEM_TOPO_DIMM_MODULE,
		.smbus = {
			[0] = { .addr_dimm[0] = 0x50, },
			[1] = { .addr_dimm[0] = 0x52, },
		},
	};
	const bool half_populated = false;

	memcfg_init(memupd, &board_cfg, &spd_info, half_populated);

	set_wifi_bt_pcie_root_ports();
}


