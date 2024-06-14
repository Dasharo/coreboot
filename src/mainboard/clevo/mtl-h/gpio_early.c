/* SPDX-License-Identifier: GPL-2.0-only */

#include <mainboard/gpio.h>
#include <soc/gpio.h>

static const struct pad_config early_gpio_table[] = {
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

/* Pins needed for graphical FSP-M SOL */
static const struct pad_config early_vga_gpio_table[] = {
	/* GPP_B11 - DDSP_HPD2 */
	PAD_CFG_NF(GPP_B11, NONE, DEEP, NF2),
	/* GPP_C20 - DDP2_CTRLCLK */
	PAD_CFG_NF(GPP_C20, NONE, DEEP, NF2),
	/* GPP_C21 - DDP2_CTRLDATA */
	PAD_CFG_NF(GPP_C21, NONE, DEEP, NF2),
	/* GPP_E14 - DDSP_HPDA */
	PAD_CFG_NF(GPP_E14, NONE, DEEP, NF1),
	/* GPP_D00 - GPIO */
	PAD_CFG_GPO(GPP_D00, 1, DEEP), /* SB_BLON */
};

void mainboard_configure_early_gpios(void)
{
	gpio_configure_pads(early_gpio_table, ARRAY_SIZE(early_gpio_table));

	/* XXX: Doesn't actually work with any FSP bins tested so far */
	if (CONFIG(SOC_INTEL_METEORLAKE_SIGN_OF_LIFE))
		gpio_configure_pads(early_vga_gpio_table, ARRAY_SIZE(early_vga_gpio_table));
}
