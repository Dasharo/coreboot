/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <soc/gpio.h>

static const struct pad_config early_uart_gpio_table[] = {
	/* UART0 */
	PAD_CFG_NF(GPP_H08, NONE, DEEP, NF1), /* UART0_RXD */
	PAD_CFG_NF(GPP_H09, NONE, DEEP, NF1), /* UART0_TXD */
};

void bootblock_mainboard_early_init(void)
{
	gpio_configure_pads(early_uart_gpio_table, ARRAY_SIZE(early_uart_gpio_table));
}
