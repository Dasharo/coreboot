/* SPDX-License-Identifier: GPL-2.0-only */

#include <gpio.h>
#include "gpio.h"

/* GPIO pins used by coreboot should be initialized in bootblock */

static const struct soc_amd_gpio gpio_table[] = {
	/* S0A3 */
	PAD_NF(GPIO_10, S0A3_GPIO, PULL_NONE)
};

void mainboard_program_gpios(void)
{
	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
}
