/* SPDX-License-Identifier: GPL-2.0-only */

#include <mainboard/gpio.h>
#include <soc/gpio.h>

static const struct pad_config early_gpio_table[] = {
	PAD_CFG_NF(GPP_H08, NONE, DEEP, NF1), /* UART0_RXD */
	PAD_CFG_NF(GPP_H09, NONE, DEEP, NF1), /* UART0_TXD */
};

void mainboard_configure_early_gpios(void)
{
	gpio_configure_pads(early_gpio_table, ARRAY_SIZE(early_gpio_table));
}
