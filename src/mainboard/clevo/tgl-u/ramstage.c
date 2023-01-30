/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <soc/ramstage.h>
#include <variant/gpio.h>
#include <variant/ramstage.h>
#include <smbios.h>
#include <ec/system76/ec/commands.h>
#include <drivers/efi/efivars.h>
#include <fmap.h>
#include <smmstore.h>

#include <Uefi/UefiBaseType.h>

#include <lib.h>

struct fan_point {
  uint8_t temp;
  uint8_t duty;
};

struct fan_curve {
  struct fan_point point1;
  struct fan_point point2;
  struct fan_point point3;
  struct fan_point point4;
};

struct smfi_cmd_set_fan_curve {
	uint8_t fan;
	struct fan_curve curve;
};

#define FAN_CURVE_SILENT {      \
  { .temp = 0,   .duty = 25  }, \
  { .temp = 65,  .duty = 30  }, \
  { .temp = 75,  .duty = 35  }, \
  { .temp = 100, .duty = 100 }  \
}

#define FAN_CURVE_PERFORMANCE { \
  { .temp = 0,   .duty = 25 },  \
  { .temp = 55,  .duty = 35 },  \
  { .temp = 75,  .duty = 60 },  \
  { .temp = 100, .duty = 100}   \
}

#define FAN_CURVE_OPTION_SILENT 0
#define FAN_CURVE_OPTION_PERFORMANCE 1

#define FAN_CURVE_OPTION_DEFAULT FAN_CURVE_OPTION_SILENT

static struct fan_curve fan_curves[] = {
  [FAN_CURVE_OPTION_SILENT]      = FAN_CURVE_SILENT,
  [FAN_CURVE_OPTION_PERFORMANCE] = FAN_CURVE_PERFORMANCE
};

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

	cmd.curve = fan_curves[selection];

	for (i = 0; i < (CONFIG(EC_SYSTEM76_EC_DGPU) ? 2 : 1); ++i) {
		cmd.fan = i;
		system76_ec_smfi_cmd(CMD_FAN_CURVE_SET, sizeof(cmd) / sizeof(uint8_t), (uint8_t *)&cmd);
	}
}

static void init_mainboard(void *chip_info)
{
	variant_configure_gpios();

	set_fan_curve();
}

struct chip_operations mainboard_ops = {
	.init = init_mainboard,
};
