/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>

/*
 * mainboard_enable is executed as first thing after enumerate_buses().
 * This is the earliest point to add customization.
 */
static void mainboard_enable(struct device *dev)
{
}

/*
 * mainboard_final is executed as one of the last items before loading the
 * payload.
 *
 * This is the latest point to add customization.
 */
static void mainboard_final(void *chip_info)
{
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};
