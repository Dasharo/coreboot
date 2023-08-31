/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <console/console.h>
#include <delay.h>
#include <gpio.h>
#include <soc/ramstage.h>
#include <soc/gpio.h>
#include <variant/ramstage.h>
#include <device/device.h>
#include <smmstore.h>

#include <drivers/efi/efivars.h>

#define DGPU_RST_N GPP_U4
#define DGPU_PWR_EN GPP_U5

void variant_configure_fsps(FSP_S_CONFIG *params)
{
	/*
	 * Disable AER for the SSD slot to support S0ix with SSDs running
	 * buggy firmware
	 */
	params->CpuPcieRpAdvancedErrorReporting[0] = 0;
	params->CpuPcieRpLtrEnable[0] = 1;
	params->CpuPcieRpSlotImplemented[0] = 1;
}

static void dgpu_power_enable(int onoff) {
	printk(BIOS_DEBUG, "nvidia: DGPU power %d\n", onoff);
	if (onoff) {
		gpio_set(DGPU_RST_N, 0);
		mdelay(4);
		gpio_set(DGPU_PWR_EN, 1);
		mdelay(4);
		gpio_set(DGPU_RST_N, 1);
	} else {
		gpio_set(DGPU_RST_N, 0);
		mdelay(4);
		gpio_set(DGPU_PWR_EN, 0);
	}
	mdelay(50);
}

static void mainboard_pre_device(void *unused) {
	dgpu_power_enable(1);
}

void mainboard_update_soc_chip_config(struct soc_intel_tigerlake_config *config) {
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
            printk(BIOS_DEBUG, "Wifi + BT radios: failed to read enablement status from EFI vars, using board default\n");
efi_err: ;
#endif
	config->CnviBtCore = enable;
	config->usb2_ports[10].enable = enable;
	printk(BIOS_DEBUG, enable ? "POZDRO\n" : "ESSA\n");
}

BOOT_STATE_INIT_ENTRY(BS_PRE_DEVICE, BS_ON_ENTRY, mainboard_pre_device, NULL);
