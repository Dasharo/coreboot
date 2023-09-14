/* SPDX-License-Identifier: GPL-2.0-only */

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

struct fan_curve fan_curve_silent = {
	{ .temp = 0,   .duty = 20  },
	{ .temp = 65,  .duty = 25  },
	{ .temp = 75,  .duty = 35  },
	{ .temp = 85,  .duty = 100 }
};

struct fan_curve fan_curve_performance = {
	{ .temp = 0,   .duty = 25 },
	{ .temp = 55,  .duty = 35 },
	{ .temp = 75,  .duty = 60 },
	{ .temp = 85,  .duty = 100}
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

	// Enable reporting CPU C10 state over eSPI
	params->PchEspiHostC10ReportEnable = 1;
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
	system76_ec_smfi_cmd(CMD_CAMERA_ENABLEMENT_SET, sizeof(enabled) / sizeof(uint8_t), (uint8_t *)&enabled);
}

static void init_mainboard(void *chip_info)
{
	variant_configure_gpios();

	set_fan_curve();

	set_camera_enablement();
}

#if CONFIG(GENERATE_SMBIOS_TABLES)
static uint8_t read_proprietary_ec_version(uint8_t *data)
{
	int i, result;
	char ec_version[16];

	if (!data)
		return -1;

	if (send_ec_command(0x93))
		return -1;

	for (i = 0; i < 16 - 1; i++) {

		result = recv_ec_data();
		if (result != -1)
			ec_version[i] = result & 0xff;

		if (ec_version[i] == '$') {
			ec_version[i] = '\0';
			break;
		}
	}
	ec_version[15] = '\0';

	data[0] = '1';
	data[1] = '.';

	strcpy((char *)&data[2], ec_version);

	return 0;
}

static void mainboard_smbios_strings(struct device *dev, struct smbios_type11 *t)
{
	char ec_version[256];
	uint8_t result;

	result = system76_ec_read_version((uint8_t *)ec_version);

	/* If the command fails it mean we are running proprietary EC most likely */
	if (result != 0) {
		printk(BIOS_ERR, "Failed to read open EC firmware version\n");
		result = read_proprietary_ec_version((uint8_t *)ec_version);
		if (result == 0)
			t->count = smbios_add_string(t->eos, "EC: proprietary");
		else
			t->count = smbios_add_string(t->eos, "EC: unknown");
	} else {
		t->count = smbios_add_string(t->eos, "EC: open-source");
	}

	if (result == 0) {
		printk(BIOS_DEBUG, "EC firmware version: %s\n", ec_version);
		t->count = smbios_add_string(t->eos, strconcat("EC firmware version: ",
					     ec_version));
	} else {
		printk(BIOS_ERR, "Unable to probe EC firmware version\n");
		t->count = smbios_add_string(t->eos, "EC firmware version: unknown");
	}

}
#endif

static void mainboard_enable(struct device *dev)
{
#if CONFIG(GENERATE_SMBIOS_TABLES)
	dev->ops->get_smbios_strings = mainboard_smbios_strings;
#endif
}

static void mainboard_final(void *chip_info)
{
	if (CONFIG(VBOOT))
		vboot_clear_recovery_request();
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.init = init_mainboard,
	.final = mainboard_final,
};
