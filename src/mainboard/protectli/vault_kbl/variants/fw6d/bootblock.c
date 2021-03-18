/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootblock_common.h>
#include <superio/ite/it8613e/it8613e.h>
#include <superio/ite/common/ite.h>

#define GPIO_DEV PNP_DEV(0x2e, IT8613E_GPIO)
#define UART_DEV PNP_DEV(0x2e, IT8613E_SP1)

void bootblock_mainboard_early_init(void)
{
	ite_reg_write(GPIO_DEV, 0x2c, 0x41); /* disable k8 power seq */
	ite_reg_write(GPIO_DEV, 0x2d, 0x02); /*PCICLK 25MHz */
	ite_enable_serial(UART_DEV, CONFIG_TTYS0_BASE);
}

