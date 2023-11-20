/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/io.h>
#include <bootblock_common.h>
#include <device/pnp.h>
#include <device/pnp_ops.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/it8613e/it8613e.h>

#define SERIAL1_DEV PNP_DEV(0x2e, IT8613E_SP1)
#define GPIO_DEV PNP_DEV(0x2e, IT8613E_GPIO)

static void ite_set_gpio_base(pnp_devfn_t dev, uint16_t iobase)
{
	pnp_enter_conf_state(dev);
	pnp_set_logical_device(dev);
	pnp_set_iobase(dev, PNP_IDX_IO1, iobase);
	pnp_exit_conf_state(dev);
}

void bootblock_mainboard_early_init(void)
{
	ite_kill_watchdog(GPIO_DEV);
	ite_reg_write(GPIO_DEV, 0x26, 0xfb); /* Pin7 as GP23 */
	ite_reg_write(GPIO_DEV, 0x29, 0x00);
	ite_reg_write(GPIO_DEV, 0x2c, 0x41); /* disable K8 power seq */
	ite_reg_write(SERIAL1_DEV, 0xf0, 0x01); /* enable IRQ sharing */
	ite_enable_serial(SERIAL1_DEV, CONFIG_TTYS0_BASE);

	ite_reg_write(GPIO_DEV, 0xc1, 0x0a); /* GP21 and GP23 as simple I/O */
	ite_reg_write(GPIO_DEV, 0xc9, 0x0a); /* GP21 and GP23 as output */
	ite_set_gpio_base(GPIO_DEV, 0xa00);

	/* GP21 and GP23 to low to enable USB ports VBUS */
	outb(inb(0xa01) & ~0x0a, 0xa01);
}
