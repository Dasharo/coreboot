/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/mmio.h>
#include <device/device.h>
#include <soc/iomap.h>
#include <soc/ramstage.h>

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	params->UsbPdoProgramming = 0;
}

static void mainboard_enable(struct device *dev)
{
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
