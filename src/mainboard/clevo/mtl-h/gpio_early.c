/* SPDX-License-Identifier: GPL-2.0-only */

#include <mainboard/gpio.h>
#include <soc/gpio.h>

static const struct pad_config early_gpio_table[] = {
	PAD_CFG_NF(GPP_H08, NONE, DEEP, NF1), // UART0_RX
	PAD_CFG_NF(GPP_H09, NONE, DEEP, NF1), // UART0_TX
	/* GPP_C00 - SMBCLK */
	/* DW0: 0x44000702, DW1: 0x0003f000 */
	/* DW0: PAD_TRIG(OFF) | PAD_BUF(TX_RX_DISABLE) | (1 << 1) - IGNORED */
	/* PAD_CFG_NF(GPP_C00, UP_20K, DEEP, NF1), */
	_PAD_CFG_STRUCT(GPP_C00, PAD_FUNC(NF1) | PAD_RESET(DEEP) | PAD_TRIG(OFF) | PAD_BUF(TX_RX_DISABLE) | (1 << 1), PAD_PULL(UP_20K) | PAD_IOSSTATE(IGNORE)),

	/* GPP_C01 - SMBDATA */
	/* DW0: 0x44000702, DW1: 0x0003f000 */
	/* DW0: PAD_TRIG(OFF) | PAD_BUF(TX_RX_DISABLE) | (1 << 1) - IGNORED */
	/* PAD_CFG_NF(GPP_C01, UP_20K, DEEP, NF1), */
	_PAD_CFG_STRUCT(GPP_C01, PAD_FUNC(NF1) | PAD_RESET(DEEP) | PAD_TRIG(OFF) | PAD_BUF(TX_RX_DISABLE) | (1 << 1), PAD_PULL(UP_20K) | PAD_IOSSTATE(IGNORE)),
};

void mainboard_configure_early_gpios(void)
{
	gpio_configure_pads(early_gpio_table, ARRAY_SIZE(early_gpio_table));
}
