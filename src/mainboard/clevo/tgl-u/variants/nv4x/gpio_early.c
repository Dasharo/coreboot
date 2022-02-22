/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <commonlib/helpers.h>

/* Early pad configuration in bootblock */
static const struct pad_config early_gpio_table[] = {
	PAD_CFG_NF(GPP_C20, UP_20K, DEEP, NF1), /* UART2_RXD */
	PAD_CFG_NF(GPP_C21, UP_20K, DEEP, NF1), /* UART2_TXD */
	PAD_CFG_TERM_GPO(GPP_U4, 0, NONE, DEEP), /* DGPU_RST#_PCH */
	PAD_CFG_TERM_GPO(GPP_U5, 0, NONE, DEEP), /* DGPU_PWR_EN */
};

const struct pad_config *variant_early_gpio_table(size_t *num)
{
	*num = ARRAY_SIZE(early_gpio_table);
	return early_gpio_table;
}
