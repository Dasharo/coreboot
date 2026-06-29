/* SPDX-License-Identifier: GPL-2.0-only */

#include <gpio.h>
#include "gpio.h"

/* GPIO pins used by coreboot should be initialized in bootblock */

static const struct soc_amd_gpio gpio_table[] = {
	PAD_GPI(GPIO_2, PULL_UP),
	PAD_INT(GPIO_3, PULL_UP, LEVEL_LOW, STATUS_DELIVERY),
	PAD_GPO(GPIO_5, HIGH),
	PAD_GPO(GPIO_6, HIGH),
	PAD_GPO(GPIO_7, HIGH),
	PAD_GPO(GPIO_8, LOW),
	PAD_GPI(GPIO_9, PULL_NONE),
	PAD_NF(GPIO_10, S0A3_GPIO, PULL_UP),
	PAD_NF(GPIO_11, BLINK, PULL_UP),
	PAD_GPI(GPIO_12, PULL_UP),
	PAD_GPI(GPIO_18, PULL_UP),
	PAD_GPI(GPIO_23, PULL_UP),
	PAD_GPI(GPIO_27, PULL_UP),
	PAD_INT(GPIO_32, PULL_NONE, LEVEL_LOW, STATUS_DELIVERY),
	PAD_GPO(GPIO_40, HIGH),
	PAD_GPI(GPIO_42, PULL_UP),
	PAD_NF(GPIO_91, SPKR, PULL_NONE),
	PAD_GPI(GPIO_115, PULL_UP),
};

void mainboard_program_gpios(void)
{
	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
}
