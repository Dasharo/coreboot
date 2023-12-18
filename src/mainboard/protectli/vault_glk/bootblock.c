/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/it8613e/it8613e.h>


#define GPIO_DEV PNP_DEV(0x2e, IT8613E_GPIO)
#define UART_DEV PNP_DEV(0x2e, IT8613E_SP1)

void bootblock_mainboard_early_init(void)
{
	ite_reg_write(GPIO_DEV, 0x25, 0x05); /* PCIRST1# and PCIRST3# as GPIO */
	ite_reg_write(GPIO_DEV, 0x2c, 0x41); /* disable k8 power seq */
	ite_reg_write(GPIO_DEV, 0x2d, 0x02); /* PCICLK 25MHz */

	ite_reg_write(GPIO_DEV, 0xc0, 0x05); /* PCIRST3# and 5VSB_CTRL GPIO Simple I/O */
	ite_reg_write(GPIO_DEV, 0xc1, 0x02); /* GP22 GPIO Simple I/O */
	ite_reg_write(GPIO_DEV, 0xc9, 0x02); /* GP22 GPIO output */

	ite_enable_serial(UART_DEV, CONFIG_TTYS0_BASE);
}
