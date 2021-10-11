/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <device/device.h>

static void mainboard_enable(struct device *dev)
{
	printk(BIOS_INFO, "Mainboard KGPE-D16 Enable. dev=0x%p\n", dev);
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
