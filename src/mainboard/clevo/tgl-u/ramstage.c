/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <drivers/efi/efivars.h>
#include <ec/system76/ec/commands.h>
#include <fmap.h>
#include <lib.h>
#include <smbios.h>
#include <smmstore.h>
#include <soc/ramstage.h>
#include <variant/gpio.h>
#include <variant/ramstage.h>

#include <Uefi/UefiBaseType.h>

struct fan_point {
	uint8_t temp;
	uint8_t duty;
} __packed;

struct fan_curve {
	struct fan_point point1;
	struct fan_point point2;
	struct fan_point point3;
	struct fan_point point4;
} __packed;

struct smfi_cmd_set_fan_curve {
	uint8_t fan;
	struct fan_curve curve;
} __packed;

struct smfi_cmd_camera_enablement_set {
    bool enable;
} __packed;

struct fan_curve fan_curve_silent = {
	{ .temp = 0,   .duty = 25  },
	{ .temp = 65,  .duty = 30  },
	{ .temp = 75,  .duty = 35  },
	{ .temp = 100, .duty = 100 }
};

struct fan_curve fan_curve_performance = {
	{ .temp = 0,   .duty = 25 },
	{ .temp = 55,  .duty = 35 },
	{ .temp = 75,  .duty = 60 },
	{ .temp = 100, .duty = 100}
};

#define FAN_CURVE_OPTION_SILENT 0
#define FAN_CURVE_OPTION_PERFORMANCE 1

#define FAN_CURVE_OPTION_DEFAULT FAN_CURVE_OPTION_SILENT

#define CAMERA_ENABLED_OPTION 1
#define CAMERA_DISABLED_OPTION 0

#define CAMERA_ENABLEMENT_DEFAULT CAMERA_ENABLED_OPTION

const char *smbios_system_sku(void)
{
	return "Not Applicable";
}

smbios_enclosure_type smbios_mainboard_enclosure_type(void)
{
	return SMBIOS_ENCLOSURE_NOTEBOOK;
}

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	variant_configure_fsps(params);
}

static void set_fan_curve(void)
{
	int i;
	uint8_t selection = FAN_CURVE_OPTION_DEFAULT;
	struct smfi_cmd_set_fan_curve cmd;

#if CONFIG(DRIVERS_EFI_VARIABLE_STORE)
	struct region_device rdev;
	enum cb_err ret;
	uint32_t size;

	const EFI_GUID dasharo_system_features_guid = {
		0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
	};

	if (smmstore_lookup_region(&rdev))
		goto efi_err;

	size = sizeof(selection);
	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "FanCurveOption", &selection, &size);
	if (ret != CB_SUCCESS)
		printk(BIOS_DEBUG, "fan: failed to read curve selection from EFI vars, using board default\n");
efi_err:
#endif

	switch (selection) {
	case FAN_CURVE_OPTION_SILENT:
		cmd.curve = fan_curve_silent;
		break;
	case FAN_CURVE_OPTION_PERFORMANCE:
		cmd.curve = fan_curve_performance;
		break;
	default:
		cmd.curve = fan_curve_silent;
		break;
	}

	for (i = 0; i < (CONFIG(EC_SYSTEM76_EC_DGPU) ? 2 : 1); ++i) {
		cmd.fan = i;
		system76_ec_smfi_cmd(CMD_FAN_CURVE_SET, sizeof(cmd) / sizeof(uint8_t), (uint8_t *)&cmd);
	}
}

static void set_camera_enablement(void)
{
    struct smfi_cmd_camera_enablement_set cmd;
    bool enabled = CAMERA_ENABLEMENT_DEFAULT;

#if CONFIG(DRIVERS_EFI_VARIABLE_STORE)
    struct region_device rdev;
    enum cb_err ret;
    uint32_t size;

    const EFI_GUID dasharo_system_features_guid = {
		0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
	};

    if (smmstore_lookup_region(&rdev))
		goto efi_err;

    size = sizeof(enabled);
    ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "EnableCamera", &enabled, &size);

    if (ret != CB_SUCCESS) {
		printk(BIOS_DEBUG, "camera: failed to read camera enablement status from EFI vars, using board default\n");
		goto efi_err;
    }

efi_err:
#endif

    cmd.enable = enabled;
    system76_ec_smfi_cmd(CMD_CAMERA_ENABLEMENT_SET, sizeof(cmd) / sizeof(uint8_t), (uint8_t *)&cmd);

}

static void init_mainboard(void *chip_info)
{
	variant_configure_gpios();

	set_fan_curve();

    set_camera_enablement();
}

struct chip_operations mainboard_ops = {
	.init = init_mainboard,
};
