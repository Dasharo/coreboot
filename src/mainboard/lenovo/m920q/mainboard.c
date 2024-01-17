/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <device/device.h>
#include <drivers/intel/gma/int15.h>
#include <gpio.h>
#include <soc/gpio.h>

static void mainboard_enable(struct device *dev)
{
	// nothing?
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
