/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <gpio.h>

static const struct soc_amd_gpio gpio_table[] = {
	/* TODO */
};

void bootblock_mainboard_init(void)
{
	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
}
