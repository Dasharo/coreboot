/* SPDX-License-Identifier: GPL-2.0-only */

#include <dasharo/options.h>
#include <ec/dasharo/ec/commands.h>
#include <ec/acpi/ec.h>
#include <ec/dasharo/ec/acpi.h>
#include <fmap.h>
#include <gpio.h>
#include <intelblocks/cse.h>
#include <lib.h>
#include <mainboard/gpio.h>
#include <mainboard/variants.h>
#include <smbios.h>
#include <soc/ramstage.h>
#include <static.h>

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
	case FAN_CURVE_OPTION_PERFORMANCE:
		cmd.curve = fan_curve_performance;
		break;
	case FAN_CURVE_OPTION_SILENT:
	default:
		cmd.curve = fan_curve_silent;
		break;
	}

	for (i = 0; i < (CONFIG(EC_DASHARO_EC_DGPU) ? 2 : 1); ++i) {
		cmd.fan = i;
		dasharo_ec_smfi_cmd(CMD_FAN_CURVE_SET, sizeof(cmd), (uint8_t *)&cmd);
	}
}

static void set_camera_enablement(void)
{
	bool enabled = get_camera_option();

	dasharo_ec_smfi_cmd(CMD_CAMERA_ENABLEMENT_SET, sizeof(enabled), (uint8_t *)&enabled);
}

static void set_battery_thresholds(void)
{
	struct battery_config bat_cfg;

	get_battery_config(&bat_cfg);

	dasharo_ec_set_bat_threshold(BAT_THRESHOLD_START, bat_cfg.start_threshold);
	dasharo_ec_set_bat_threshold(BAT_THRESHOLD_STOP, bat_cfg.stop_threshold);
}

static void set_power_on_ac(void)
{
	struct smfi_option_get_cmd {
		uint8_t index;
		uint8_t value;
	} __packed cmd = {
		OPT_POWER_ON_AC,
		0
	};

	cmd.value = dasharo_get_power_on_after_fail();

	dasharo_ec_smfi_cmd(CMD_OPTION_SET, sizeof(cmd), (uint8_t *)&cmd);
}

static void set_usb_charge_port(void)
{
	struct smfi_option_get_cmd {
		uint8_t index;
		uint8_t value;
	} __packed cmd = {
		OPT_ALWAYS_ON_USB,
		0
	};

	cmd.value = dasharo_get_usb_port_power();

	dasharo_ec_smfi_cmd(CMD_OPTION_SET, sizeof(cmd) / sizeof(uint8_t), (uint8_t *)&cmd);
}

void __weak variant_devtree_update(void)
{
	/* Override dev tree settings per board */
}

static void mainboard_init(void *chip_info)
{
	config_t *cfg = config_of_soc();
	struct device *wlan_dev = pcidev_on_root(0x1c, 7);
	struct device *cnvi_dev = pcidev_on_root(0x14, 3);
	bool radio_enable = get_wireless_option();

	printk(BIOS_DEBUG, "Wireless is %sabled\n", radio_enable ? "en" : "dis");

	wlan_dev->enabled = radio_enable;
	cnvi_dev->enabled = radio_enable;
	cfg->cnvi_bt_core = radio_enable;
	cfg->cnvi_bt_audio_offload = radio_enable;
	cfg->cnvi_wifi_core = radio_enable;
	cfg->usb2_ports[9].enable = radio_enable;

	dasharo_ec_smfi_cmd(CMD_WIFI_BT_ENABLEMENT_SET, 1, (uint8_t *)&radio_enable);

	variant_configure_gpios();
	set_fan_curve();
	set_camera_enablement();
	set_battery_thresholds();
	set_power_on_ac();
	set_usb_charge_port();

	variant_devtree_update();
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

	result = dasharo_ec_read_version((uint8_t *)ec_version);

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
static int mainboard_smbios_data(struct device *dev, int *handle, unsigned long *current)
{
	int len = 0;

	len += cse_write_smbios_type14(handle, current);

	return len;
}
#endif


static void mainboard_enable(struct device *dev)
{
#if CONFIG(GENERATE_SMBIOS_TABLES)
	dev->ops->get_smbios_data = mainboard_smbios_data;
	dev->ops->get_smbios_strings = mainboard_smbios_strings;
#endif
}

void mainboard_update_soc_chip_config(struct soc_intel_meteorlake_config *config)
{
	if (get_sleep_type_option() == SLEEP_TYPE_OPTION_S3)
		config->s0ix_enable = 0;
	else
		config->s0ix_enable = 1;
}

void __weak variant_final(void)
{

}

static void mainboard_final(void *chip_info)
{
	variant_final();
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	// Enable reporting CPU C10 state over eSPI
	params->PchEspiHostC10ReportEnable = 1;

	// Pinmux configuration
	params->PchSerialIoI2cSdaPinMux[3] = 0x1A45CA06; // GPP_H6
	params->PchSerialIoI2cSclPinMux[3] = 0x1A45AA07; // GPP_H7

	params->PchSerialIoI2cSdaPinMux[4] = 0x8A44CC0C; // GPP_E12
	params->PchSerialIoI2cSclPinMux[4] = 0x8A44AC0D; // GPP_E13

	params->PchSerialIoI2cSdaPinMux[5] = 0x8A46CE0D; // GPP_F13
	params->PchSerialIoI2cSclPinMux[5] = 0x8A46AE0C; // GPP_F12

	params->CnviRfResetPinMux = 0x194CE404; // GPP_F04
	params->CnviClkreqPinMux = 0x394CE605;  // GPP_F05

	/*
	 * [3:0] MappingPchXhciUsbA (1-based USB2 port numbering)
	 * [5:4] Reserved
	 * [6]   Orientation - TCSS port uses TX0/RX0 pairs or TX1/RX1 pairs
	 * [7]   Enable
	 */
	params->EnableTcssCovTypeA[1] = 0x82;

	params->LidStatus = dasharo_ec_get_lid_state();

	params->PortResetMessageEnable[1] = 1;
	params->PortResetMessageEnable[5] = 1;

	/* Disable S0i2.x due to wake issues */
	params->PmcLpmS0ixSubStateEnableMask = BIT(0);

	params->PmcPdEnable = 1;
	params->TcCstateLimit = 10;
	params->TcNotifyIgd = 1;
	params->PsOnEnable = 0;
}
