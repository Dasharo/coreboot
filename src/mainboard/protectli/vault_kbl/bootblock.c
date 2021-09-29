/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootblock_common.h>
#include <device/pnp_ops.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/it8613e/it8613e.h>

#define GPIO_DEV PNP_DEV(0x2e, IT8613E_GPIO)
#define UART_DEV PNP_DEV(0x2e, IT8613E_SP1)

/* Return an id of the installed chip. */
static int ite_read_id(void)
{
	pnp_enter_conf_state(GPIO_DEV);
	int id = (pnp_read_config(GPIO_DEV, 0x20) << 8)
		| pnp_read_config(GPIO_DEV, 0x21);
	pnp_exit_conf_state(GPIO_DEV);
	return id;
}

void bootblock_mainboard_early_init(void)
{
	if (ite_read_id() == 0x8613){
		ite_reg_write(GPIO_DEV, 0x23, 0x49); /* CLK Select Ext CLKIN, 24MHz */
		ite_reg_write(GPIO_DEV, 0x2c, 0x41); /* Disable k8 power seq */
		ite_reg_write(GPIO_DEV, 0x2d, 0x02); /* PCICLK 25MHz */
		ite_reg_write(GPIO_DEV, 0x71, 0x08); /* Ext CLKIN PCICLK */
	} else {
		ite_conf_clkin(GPIO_DEV, ITE_UART_CLK_PREDIVIDE_24);
	}
	ite_enable_3vsbsw(GPIO_DEV);
	ite_kill_watchdog(GPIO_DEV);
	ite_enable_serial(UART_DEV, CONFIG_TTYS0_BASE);
}
