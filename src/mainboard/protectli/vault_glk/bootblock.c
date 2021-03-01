/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <intelblocks/lpc_lib.h>
#include <soc/gpio.h>
#include "variants.h"
#include <superio/ite/common/ite.h>
#include <superio/ite/it8613e/it8613e.h>

#define GPIO_DEV PNP_DEV(0x2e, IT8613E_GPIO)
#define UART_DEV PNP_DEV(0x2e, IT8613E_SP1)

void bootblock_mainboard_early_init(void)
{	
	ite_reg_write(GPIO_DEV, 0x2c, 0x41); /* disable k8 power seq */
	ite_reg_write(GPIO_DEV, 0x2d, 0x02); /* PCICLK 25MHz */
	ite_enable_serial(UART_DEV, CONFIG_TTYS0_BASE);
}

void bootblock_mainboard_init(void)
{
	const struct pad_config *pads;
	size_t num;

	lpc_configure_pads();
	pads = variant_early_gpio_table(&num);
	gpio_configure_pads(pads, num);
}
