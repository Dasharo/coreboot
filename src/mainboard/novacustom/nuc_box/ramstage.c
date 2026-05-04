/* SPDX-License-Identifier: GPL-2.0-only */

#include <dasharo/options.h>
#include <device/smbus_host.h>
#include <intelblocks/cse.h>
#include <mainboard/gpio.h>
#include <soc/ramstage.h>

static int mainboard_smbios_data(struct device *dev, int *handle, unsigned long *current)
{
	int len = 0;

	len += cse_write_smbios_type14(handle, current);

	return len;
}

static void mainboard_init(void *chip_info)
{
	// The DACC feature resets CMOS if the firmware does not send this message
	printk(BIOS_DEBUG, "Handling DACC\n");
	do_smbus_write_byte(CONFIG_FIXED_SMBUS_IO_BASE, 0xBA >> 1, 0x0F, 0xAA);
}

void mainboard_update_soc_chip_config(struct soc_intel_meteorlake_config *config)
{
	if (get_sleep_type_option() == SLEEP_TYPE_OPTION_S3)
		config->s0ix_enable = 0;
	else
		config->s0ix_enable = 1;
}

static void mainboard_enable(struct device *dev)
{
#if CONFIG(GENERATE_SMBIOS_TABLES)
	dev->ops->get_smbios_data = mainboard_smbios_data;
#endif
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.init = mainboard_init,
};
