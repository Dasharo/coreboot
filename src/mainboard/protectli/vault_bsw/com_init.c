/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootblock_common.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/it8613e/it8613e.h>

#define SERIAL1_DEV PNP_DEV(0x2e, IT8613E_SP1)
#define GPIO_DEV PNP_DEV(0x2e, IT8613E_GPIO)

void bootblock_mainboard_early_init(void)
{

	if (CONFIG(BOARD_PROTECTLI_FW4C)) {
		ite_reg_write(GPIO_DEV, 0x23, 0x09);	/* External CLK */
		ite_reg_write(GPIO_DEV, 0x71, 0x09);	/* PCICLK */
		ite_reg_write(GPIO_DEV, 0x26, 0xf7);
	} else {
		ite_reg_write(GPIO_DEV, 0x2c, 0x41); /* disable K8 power seq */
		ite_reg_write(GPIO_DEV, 0x2d, 0x02); /* PCICLK 25MHz */
	}

	ite_enable_serial(SERIAL1_DEV, CONFIG_TTYS0_BASE);
}
