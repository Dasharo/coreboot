/* SPDX-License-Identifier: GPL-2.0-or-later */

#define __SIMPLE_DEVICE__

#include <acpi/acpi.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include <dasharo/options.h>
#include <device/pnp_def.h>
#include <device/pnp_ops.h>
#include <intelblocks/smihandler.h>
#include <soc/pm.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/common/ite_gpio.h>
#include <superio/ite/it8613e/it8613e.h>

#define GPIO_DEV PNP_DEV(0x2e, IT8613E_GPIO)

#define ITE_GPIO_PIN(x)		(1 << ((x) % 10))
#define ITE_GPIO_SET(x)		(((x) / 10) - 1)
#define ITE_GPIO_IO_ADDR(x)	(gpio_iobase + ITE_GPIO_SET(x))

static uint16_t ite_get_gpio_iobase(void)
{
	uint16_t iobase = 0;
	pnp_enter_conf_state(GPIO_DEV);
	pnp_set_logical_device(GPIO_DEV);
	iobase = pnp_read_config(GPIO_DEV, PNP_IDX_IO1) << 8;
	iobase |= pnp_read_config(GPIO_DEV, PNP_IDX_IO1 + 1);
	pnp_exit_conf_state(GPIO_DEV);

	return iobase;
}

static void ite_reset_hw_cfg(void)
{
	ite_reg_write(GPIO_DEV, 0x02, 0x01);
}

void mainboard_smi_sleep(u8 slp_typ)
{
	printk(BIOS_DEBUG, "Mainboard SMI sleep handler: %02x\n", slp_typ);
	uint16_t gpio_iobase = ite_get_gpio_iobase();
	uint8_t usb_power = dasharo_get_usb_port_power();

	if (slp_typ < ACPI_S3)
		return;

	if (usb_power == USB_PORT_ALWAYS_ON)
		return;

	/* Fallback: GPIO I/O base not found, hard reset the Super I/O */
	if (gpio_iobase == 0 || gpio_iobase == 0xffff) {
		ite_reset_hw_cfg();
		return;
	}

	/* GP21 and GP23 to high to disable USB ports VBUS */
	outb(inb(ITE_GPIO_IO_ADDR(21)) | ITE_GPIO_PIN(21), ITE_GPIO_IO_ADDR(21));
	outb(inb(ITE_GPIO_IO_ADDR(23)) | ITE_GPIO_PIN(23), ITE_GPIO_IO_ADDR(23));
}

void mainboard_smi_pm1_handler(uint16_t pm1_sts, uint16_t pm1_en)
{
	uint16_t gpio_iobase = ite_get_gpio_iobase();
	uint8_t usb_power = dasharo_get_usb_port_power();

	if ((pm1_sts & PWRBTN_STS) && (pm1_en & PWRBTN_EN)) {
		if (usb_power == USB_PORT_ALWAYS_ON)
			return;

		/* Fallback: GPIO I/O base not found, hard reset the Super I/O */
		if (gpio_iobase == 0 || gpio_iobase == 0xffff) {
			ite_reset_hw_cfg();
			return;
		}

		/* GP21 and GP23 to high to disable USB ports VBUS */
		outb(inb(ITE_GPIO_IO_ADDR(21)) | ITE_GPIO_PIN(21), ITE_GPIO_IO_ADDR(21));
		outb(inb(ITE_GPIO_IO_ADDR(23)) | ITE_GPIO_PIN(23), ITE_GPIO_IO_ADDR(23));
	}
}
