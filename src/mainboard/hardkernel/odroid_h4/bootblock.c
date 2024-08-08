/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <bootblock_common.h>
#include <device/pnp_ops.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/it8613e/it8613e.h>

#define GPIO_DEV PNP_DEV(0x2e, IT8613E_GPIO)
#define UART_DEV PNP_DEV(0x2e, IT8613E_SP1)

#define ITE_GPIO_BASE		0xa00
#define ITE_GPIO_PIN(x)		(1 << ((x) % 10))
#define ITE_GPIO_SET(x)		(((x) / 10) - 1)
#define ITE_GPIO_IO_ADDR(x)	(ITE_GPIO_BASE + ITE_GPIO_SET(x))

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
	ite_reg_write(GPIO_DEV, 0x23, 0x40);
	/* Pin7 as GP23 - USB2_EN */
	ite_reg_write(GPIO_DEV, 0x26, 0xfb);
	/* Pin24 as GPO50 (value of 0 on bit0 is reserved, JP1 strapping)*/
	ite_reg_write(GPIO_DEV, 0x29, 0x01);
	/* K8 power sequence sofyware disabled */
	ite_reg_write(GPIO_DEV, 0x2c, 0x41);
	/* PCICLK 25MHz */
	ite_reg_write(GPIO_DEV, 0x2d, 0x02);
	ite_kill_watchdog(GPIO_DEV);
	/* GP21 - USB3_EN, VBUS power gate for the USB3.x ports */
	ite_gpio_setup(21, GPIO_POL_NO_INVERT, GPIO_PULLUP_DIS,
		       GPIO_OUTPUT_MODE, GPIO_SIMPLE_IO);
	/* GP23 - USB2_EN, VBUS power gate for the USB2.x ports */
	ite_gpio_setup(23, GPIO_POL_NO_INVERT, GPIO_PULLUP_DIS,
		       GPIO_OUTPUT_MODE, GPIO_SIMPLE_IO);
	ite_set_gpio_iobase(ITE_GPIO_BASE);
	/* GP21 and GP23 to low to enable USB ports VBUS */
	outb(inb(ITE_GPIO_IO_ADDR(21)) & ~ITE_GPIO_PIN(21), ITE_GPIO_IO_ADDR(21));
	outb(inb(ITE_GPIO_IO_ADDR(23)) & ~ITE_GPIO_PIN(23), ITE_GPIO_IO_ADDR(23));

	ite_enable_serial(UART_DEV, CONFIG_TTYS0_BASE);
}
