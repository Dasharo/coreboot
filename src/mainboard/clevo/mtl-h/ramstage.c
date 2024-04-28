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

static void mainboard_init(void *chip_info)
{
	mainboard_configure_gpios();
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

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.init = mainboard_init,
};

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	// Enable reporting CPU C10 state over eSPI
	params->PchEspiHostC10ReportEnable = 1;

	// Pinmux configuration
	params->CnviRfResetPinMux = 0x194CE404; // GPP_F04
	params->CnviClkreqPinMux = 0x394CE605;  // GPP_F05

	params->LidStatus = system76_ec_get_lid_state();
}

