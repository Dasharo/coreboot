/* SPDX-License-Identifier: GPL-2.0-only */

#include <mainboard/gpio.h>
#include <soc/ramstage.h>
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

smbios_wakeup_type smbios_system_wakeup_type(void)
{
	return SMBIOS_WAKEUP_TYPE_POWER_SWITCH;
}

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	params->CnviRfResetPinMux = 0x194CE404; // GPP_F4
	params->CnviClkreqPinMux = 0x394CE605; // GPP_F5

	params->PchSerialIoI2cSdaPinMux[0] = 0x1947c404; // GPP_H4
	params->PchSerialIoI2cSclPinMux[0] = 0x1947a405; // GPP_H5
	params->PchSerialIoI2cSdaPinMux[1] = 0x1947c606; // GPP_H6
	params->PchSerialIoI2cSclPinMux[1] = 0x1947a607; // GPP_H7

	// DEVSLP doesn't work due to broken board design (crossed wiring)
	params->SataPortDevSlpPinMux[0] = 0x59673e0c; // GPP_H12
	params->SataPortDevSlpPinMux[1] = 0x5967400d; // GPP_H13

	params->SataPortsSolidStateDrive[1] = 1;

	params->PcieRpEnableCpm[4] = 1;
	params->PcieRpEnableCpm[5] = 1;
	params->PcieRpEnableCpm[7] = 1;
	params->PcieRpEnableCpm[8] = 1;

	params->PcieRpAcsEnabled[4] = 1;
	params->PcieRpAcsEnabled[5] = 1;
	params->PcieRpAcsEnabled[7] = 1;
	params->PcieRpAcsEnabled[8] = 1;

	params->PcieRpMaxPayload[4] = 1;
	params->PcieRpMaxPayload[5] = 1;
	params->PcieRpMaxPayload[7] = 1;
	params->PcieRpMaxPayload[8] = 1;

	params->PcieRpPmSci[4] = 1;
	params->PcieRpPmSci[5] = 1;
	params->PcieRpPmSci[7] = 1;
	params->PcieRpPmSci[8] = 1;

	params->CpuPcieRpPmSci[0] = 1;
	params->CpuPcieRpEnableCpm[0] = 1;
	params->CpuPcieClockGating[0] = 1;
	params->CpuPciePowerGating[0] = 1;
	params->CpuPcieRpMultiVcEnabled[0] = 1;
	params->CpuPcieRpPeerToPeerMode[0] = 1;
	params->CpuPcieRpMaxPayload[0] = 2; // 512B
	params->CpuPcieRpAcsEnabled[0] = 1;
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

static void mainboard_init(void *chip_info)
{
	mainboard_configure_gpios();

	set_fan_curve();
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
};
