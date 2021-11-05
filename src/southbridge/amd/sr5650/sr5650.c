/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>

static void sr5650_enable(struct device *dev)
{
}

struct chip_operations southbridge_amd_sr5650_ops = {
	CHIP_NAME("ATI SR5650")
	.enable_dev = sr5650_enable,
};
