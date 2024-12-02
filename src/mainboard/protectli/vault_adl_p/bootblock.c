/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <device/pnp_ops.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/it8659e/it8659e.h>

#if CONFIG_UART_FOR_CONSOLE == 0
#define UART_DEV PNP_DEV(0x2e, IT8659E_SP1)
#elif CONFIG_UART_FOR_CONSOLE == 1
#define UART_DEV PNP_DEV(0x2e, IT8659E_SP2)
#else
#error "Wrong UART_FOR_CONSOLE setting"
#endif

#define GPIO_DEV PNP_DEV(0x2e, IT8659E_GPIO)

static void ite_set_gpio_iobase(u16 iobase)
{
	pnp_enter_conf_state(GPIO_DEV);
	pnp_set_logical_device(GPIO_DEV);
	pnp_set_iobase(GPIO_DEV, PNP_IDX_IO1, iobase);
	pnp_exit_conf_state(GPIO_DEV);
}

static void ite_gpio_setup(u8 gpio, u8 polarity, u8 pullup, u8 output, u8 enable)
{
	u8 set = (gpio / 10) - 1;
	u8 pin = (gpio % 10);

	/* There are only 6 configurable sets, 8 pins each */
	if (gpio < 10 || set > 6 || pin > 7)
		return;

	pnp_enter_conf_state(GPIO_DEV);
	pnp_set_logical_device(GPIO_DEV);
	if (set < 5) {
		pnp_unset_and_set_config(GPIO_DEV, GPIO_REG_ENABLE(set),
					 enable << pin, enable << pin);
		pnp_unset_and_set_config(GPIO_DEV, GPIO_REG_POLARITY(set),
					 polarity << pin, polarity << pin);
	}
	pnp_unset_and_set_config(GPIO_DEV, GPIO_REG_OUTPUT(set), output << pin, output << pin);
	pnp_unset_and_set_config(GPIO_DEV, GPIO_REG_PULLUP(set), pullup << pin, pullup << pin);
	pnp_exit_conf_state(GPIO_DEV);
}

void bootblock_mainboard_early_init(void)
{
	/* Internal VCC_OK */
	ite_reg_write(GPIO_DEV, 0x23, 0x00);
	/* Set pin native functions */
	ite_reg_write(GPIO_DEV, 0x26, 0xc0);
	/* Pin28 as GP41 - PC speaker */
	ite_reg_write(GPIO_DEV, 0x28, 0x02);
	/* Set GPIOs exposed on pin header as GPIO functions */
	ite_reg_write(GPIO_DEV, 0x29, 0xc0);
	/* Sets a reserved bit6 to reflect original FW configuration */
	ite_reg_write(GPIO_DEV, 0x2c, 0xc9);
	ite_kill_watchdog(GPIO_DEV);
	/* GP41 - PC Speaker configuration */
	ite_gpio_setup(41, GPIO_POL_NO_INVERT, GPIO_PULLUP_DIS, GPIO_OUTPUT_MODE, GPIO_SIMPLE_IO);
	ite_set_gpio_iobase(0xa00);
	ite_enable_serial(UART_DEV, CONFIG_TTYS0_BASE);
}
