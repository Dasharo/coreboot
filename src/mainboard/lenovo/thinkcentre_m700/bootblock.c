/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/it8625e/it8625e.h>
#include <soc/gpio.h>
#include "include/gpio.h"

#define GPIO_DEV PNP_DEV(0x2e, IT8625E_GPIO)

#if CONFIG_UART_FOR_CONSOLE == 0
#define SERIAL_DEV		PNP_DEV(0x2e, IT8625E_SP1)
#elif CONFIG_UART_FOR_CONSOLE == 1
#define SERIAL_DEV		PNP_DEV(0x2e, IT8625E_SP2)
#else
#error "Invalid value for CONFIG_UART_FOR_CONSOLE"
#endif

static void early_config_gpio(void)
{
	gpio_configure_pads(early_gpio_table, ARRAY_SIZE(early_gpio_table));
}

void bootblock_mainboard_init(void)
{
	early_config_gpio();
}

static void ite_gpio_conf(pnp_devfn_t dev)
{
	ite_reg_write(dev, 0x23, 0x49);
	ite_reg_write(dev, 0x24, 0x01);
	ite_reg_write(dev, 0x25, 0x10);
	ite_reg_write(dev, 0x25, 0x10);
	ite_reg_write(dev, 0x26, 0x00);
	ite_reg_write(dev, 0x2a, 0x01);
	ite_reg_write(dev, 0x62, 0x0a);
	ite_reg_write(dev, 0x71, 0x09);
	ite_reg_write(dev, 0x72, 0x20);
    ite_reg_write(dev, 0xb8, 0x10);
    ite_reg_write(dev, 0xbc, 0xc0);
    ite_reg_write(dev, 0xbd, 0x03);
    ite_reg_write(dev, 0xd3, 0x00);
    ite_reg_write(dev, 0xd5, 0x17);
    ite_reg_write(dev, 0xf0, 0x10);
    ite_reg_write(dev, 0xf1, 0x41);
    ite_reg_write(dev, 0xf4, 0x0c);
}

void bootblock_mainboard_early_init(void)
{
    /* Configure GPIOs */
    ite_gpio_conf(GPIO_DEV);

    /* Enable early serial console */
    ite_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
}
