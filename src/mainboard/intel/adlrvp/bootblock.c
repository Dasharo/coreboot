/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <bootblock_common.h>

void bootblock_mainboard_init(void)
{
	variant_configure_early_gpio_pads();
}
