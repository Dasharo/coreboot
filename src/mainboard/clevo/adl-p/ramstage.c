/* SPDX-License-Identifier: GPL-2.0-only */

#include <dasharo/options.h>
#include <ec/system76/ec/commands.h>
#include <ec/acpi/ec.h>
#include <ec/system76/ec/acpi.h>
#include <fmap.h>
#include <lib.h>
#include <mainboard/gpio.h>
#include <security/vboot/vboot_common.h>
#include <smbios.h>
#include <soc/ramstage.h>

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

static void set_fan_curve(void)
{
	int i;
	uint8_t selection = get_fan_curve_option();
	struct smfi_cmd_set_fan_curve cmd;

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
	bool enabled = get_camera_option();

	system76_ec_smfi_cmd(CMD_CAMERA_ENABLEMENT_SET, sizeof(enabled) / sizeof(uint8_t), (uint8_t *)&enabled);
}

static void mainboard_init(void *chip_info)
{
	mainboard_configure_gpios();

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
	.init = mainboard_init,
	.final = mainboard_final,
};

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
	params->PcieRpEnableCpm[9] = 1;

	params->PcieRpAcsEnabled[4] = 1;
	params->PcieRpAcsEnabled[5] = 1;
	params->PcieRpAcsEnabled[7] = 1;
	params->PcieRpAcsEnabled[8] = 1;
	params->PcieRpAcsEnabled[9] = 1;

	params->PcieRpMaxPayload[4] = 1;
	params->PcieRpMaxPayload[5] = 1;
	params->PcieRpMaxPayload[7] = 1;
	params->PcieRpMaxPayload[8] = 1;
	params->PcieRpMaxPayload[9] = 1;

	params->PcieRpPmSci[4] = 1;
	params->PcieRpPmSci[5] = 1;
	params->PcieRpPmSci[7] = 1;
	params->PcieRpPmSci[8] = 1;
	params->PcieRpPmSci[9] = 1;

	params->CpuPcieRpPmSci[0] = 1;
	params->CpuPcieRpEnableCpm[0] = 1;
	params->CpuPcieClockGating[0] = 1;
	params->CpuPciePowerGating[0] = 1;
	params->CpuPcieRpMultiVcEnabled[0] = 1;
	params->CpuPcieRpPeerToPeerMode[0] = 1;
	params->CpuPcieRpMaxPayload[0] = 2; // 512B
	params->CpuPcieRpAcsEnabled[0] = 1;

	// Enable reporting CPU C10 state over eSPI
	params->PchEspiHostC10ReportEnable = 1;

	params->LidStatus = system76_ec_get_lid_state();
}

void mainboard_update_soc_chip_config(struct soc_intel_alderlake_config *config)
{
	if (get_sleep_type_option() == SLEEP_TYPE_OPTION_S3)
		config->s0ix_enable = 0;
	else
		config->s0ix_enable = 1;
}
