/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <console/console.h>
#include <device/device.h>
#include <soc/gpio.h>
#include "variants.h"

static void mainboard_init(void *chip_info)
{
	const struct pad_config *pads;
	size_t num;
}

static void mainboard_enable(struct device *dev)
{
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = mainboard_enable,
};
