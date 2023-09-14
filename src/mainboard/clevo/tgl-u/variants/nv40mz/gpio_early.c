/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/gpio.h>
#include <variant/gpio.h>

/* Name format: <pad name> / <net/pin name in schematics> */
static const struct pad_config early_gpio_table[] = {
	PAD_CFG_NF(GPP_C20, UP_20K, DEEP, NF1), /* UART2_RXD */
	PAD_CFG_NF(GPP_C21, UP_20K, DEEP, NF1), /* UART2_TXD */
	PAD_CFG_GPO(GPP_U4, 0, DEEP), /* DGPU_RST#_PCH */
	PAD_CFG_GPO(GPP_U5, 0, DEEP), /* DGPU_PWR_EN */
};

void variant_configure_early_gpios(void)
{
	gpio_configure_pads(early_gpio_table, ARRAY_SIZE(early_gpio_table));
}
