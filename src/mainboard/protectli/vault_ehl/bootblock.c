
/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/it8613e/it8613e.h>
#include <device/pnp_ops.h>
#include <superio/ite/common/ite_gpio.h>


#define GPIO_DEV PNP_DEV(0x2e, IT8613E_GPIO)
#define UART_DEV PNP_DEV(0x2e, IT8613E_SP1)

static void ite_set_gpio_iobase(u16 iobase)
{
	pnp_enter_conf_state(GPIO_DEV);
	pnp_set_logical_device(GPIO_DEV);
	pnp_set_iobase(GPIO_DEV, PNP_IDX_IO1, iobase);
	pnp_exit_conf_state(GPIO_DEV);
}


void bootblock_mainboard_early_init(void)
{
	ite_reg_write(GPIO_DEV, 0x25, 0x04);  /* mux: GP12 instead of PCIRST1# */
	ite_reg_write(GPIO_DEV, 0x29, 0xc1);
	ite_reg_write(GPIO_DEV, 0x2a, 0x00); /* Disable FAN_CTL4 */
	ite_reg_write(GPIO_DEV, 0x2c, 0x41); /* disable k8 power seq */
	ite_reg_write(GPIO_DEV, 0x2d, 0x02); /* PCICLK=25MHz */
	ite_gpio_setup(GPIO_DEV, 12, ITE_GPIO_INPUT, ITE_GPIO_SIMPLE_IO_MODE, ITE_GPIO_PULLUP_ENABLE);
	ite_set_gpio_iobase(0xa00);

	ite_kill_watchdog(GPIO_DEV);
	ite_enable_serial(UART_DEV, CONFIG_TTYS0_BASE);
}

void bootblock_mainboard_init(void)
{
}
