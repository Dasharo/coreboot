/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/mmio.h>
#include <device/device.h>
#include <soc/iomap.h>

static void mainboard_enable(struct device *dev)
{
}

struct chip_operations mainboard_ops = {
    .enable_dev = mainboard_enable,
};
