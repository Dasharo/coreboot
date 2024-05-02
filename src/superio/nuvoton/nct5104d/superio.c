/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <device/pnp.h>
#include <device/device.h>
#include <superio/conf_mode.h>

#include "nct5104d.h"
#include "chip.h"

static void set_irq_trigger_type(struct device *dev, bool trig_level)
{
	u8 reg10, reg11, reg26;

	//Before accessing CR10 OR CR11 Bit 4 in CR26 must be set to 1
	reg26 = pnp_read_config(dev, GLOBAL_OPTION_CR26);
	reg26 |= CR26_LOCK_REG;
	pnp_write_config(dev, GLOBAL_OPTION_CR26, reg26);

	switch (dev->path.pnp.device) {
	//SP1 (UARTA) IRQ type selection (1:level,0:edge) is controlled by CR 10, bit 5
	case NCT5104D_SP1:
		reg10 = pnp_read_config(dev, IRQ_TYPE_SEL_CR10);
		if (trig_level)
			reg10 |= (1 << 5);
		else
			reg10 &= ~(1 << 5);
		pnp_write_config(dev, IRQ_TYPE_SEL_CR10, reg10);
		break;
	//SP2 (UARTB) IRQ type selection (1:level,0:edge) is controlled by CR 10, bit 4
	case NCT5104D_SP2:
		reg10 = pnp_read_config(dev, IRQ_TYPE_SEL_CR10);
		if (trig_level)
			reg10 |= (1 << 4);
		else
			reg10 &= ~(1 << 4);
		pnp_write_config(dev, IRQ_TYPE_SEL_CR10, reg10);
		break;
	//SP3 (UARTC) IRQ type selection (1:level,0:edge) is controlled by CR 11, bit 5
	case NCT5104D_SP3:
		reg11 = pnp_read_config(dev,IRQ_TYPE_SEL_CR11);
		if (trig_level)
			reg11 |= (1 << 5);
		else
			reg11 &= ~(1 << 5);
		pnp_write_config(dev, IRQ_TYPE_SEL_CR11, reg11);
		break;
	//SP4 (UARTD) IRQ type selection (1:level,0:edge) is controlled by CR 11, bit 4
	case NCT5104D_SP4:
		reg11 = pnp_read_config(dev,IRQ_TYPE_SEL_CR11);
		if (trig_level)
			reg11 |= (1 << 4);
		else
			reg11 &= ~(1 << 4);
		pnp_write_config(dev, IRQ_TYPE_SEL_CR11, reg11);
		break;
	default:
		break;
	}

	//Clear access control register
	reg26 = pnp_read_config(dev, GLOBAL_OPTION_CR26);
	reg26 &= ~CR26_LOCK_REG;
	pnp_write_config(dev, GLOBAL_OPTION_CR26, reg26);
}

static void route_pins_to_uart(struct device *dev, bool to_uart)
{
	u8 reg;

	reg = pnp_read_config(dev, 0x1c);

	switch (dev->path.pnp.device) {
	case NCT5104D_SP3:
	case NCT5104D_GPIO0:
		/* Route pins 33 - 40. */
		if (to_uart)
			reg |= (1 << 3);
		else
			reg &= ~(1 << 3);
		break;
	case NCT5104D_SP4:
	case NCT5104D_GPIO1:
		/* Route pins 41 - 48. */
		if (to_uart)
			reg |= (1 << 2);
		else
			reg &= ~(1 << 2);
		break;
	default:
		break;
	}

	pnp_write_config(dev, 0x1c, reg);
}

static void reset_gpio_default_in(struct device *dev)
{
	pnp_set_logical_device(dev);
	/*
	 * Soft reset GPIOs to default state: IN.
	 * The main GPIO LDN holds registers that configure the pins as output
	 * or input. These registers are located at offset 0xE0 plus the GPIO
	 * bank number multiplied by 4: 0xE0 for GPIO0, 0xE4 for GPIO1 and
	 * 0xF8 for GPIO6.
	 */
	pnp_write_config(dev, NCT5104D_GPIO0_IO + (dev->path.pnp.device >> 8) * 4, 0xFF);
}

static void reset_gpio_default_od(struct device *dev)
{
	struct device *gpio0, *gpio1, *gpio6;

	pnp_set_logical_device(dev);

	gpio0 = dev_find_slot_pnp(dev->path.pnp.port, NCT5104D_GPIO0);
	gpio1 = dev_find_slot_pnp(dev->path.pnp.port, NCT5104D_GPIO1);
	gpio6 = dev_find_slot_pnp(dev->path.pnp.port, NCT5104D_GPIO6);

	/*
	 * Soft reset GPIOs to default state: Open-drain.
	 * The NCT5104D_GPIO_PP_OD LDN holds registers (1 for each GPIO bank)
	 * that configure each GPIO pin to be open dain or push pull. System
	 * reset is known to not reset the values in this register. The
	 * registers are located at offsets begginign from 0xE0 plus GPIO bank
	 * number, i.e. 0xE0 for GPIO0, 0xE1 for GPIO1 and 0xE6 for GPIO6.
	 */
	if (gpio0 && gpio0->enabled)
		pnp_write_config(dev,
				 (gpio0->path.pnp.device >> 8) + NCT5104D_GPIO0_PP_OD, 0xFF);

	if (gpio1 && gpio1->enabled)
		pnp_write_config(dev,
				 (gpio1->path.pnp.device >> 8) + NCT5104D_GPIO0_PP_OD, 0xFF);

	if (gpio6 && gpio6->enabled)
		pnp_write_config(dev,
				 (gpio6->path.pnp.device >> 8) + NCT5104D_GPIO0_PP_OD, 0xFF);
}

static void disable_gpio_io_port(struct device *dev)
{
	struct device *gpio0, *gpio1, *gpio6;

	/*
	 * Since UARTC and UARTD share pins with GPIO0 and GPIO1 and the
	 * GPIO/UART can be selected via Kconfig, check whether at least one of
	 * GPIOs is enabled and if yes keep the GPIO IO VLDN enabled. If no
	 * GPIOs are enabled, disable the VLDN in order to protect from invalid
	 * devicetree + Kconfig settings.
	 */
	gpio0 = dev_find_slot_pnp(dev->path.pnp.port, NCT5104D_GPIO0);
	gpio1 = dev_find_slot_pnp(dev->path.pnp.port, NCT5104D_GPIO1);
	gpio6 = dev_find_slot_pnp(dev->path.pnp.port, NCT5104D_GPIO6);

	if (!((gpio0 && gpio0->enabled) || (gpio1 && gpio1->enabled) ||
	      (gpio6 && gpio6->enabled))) {
		dev->enabled = 0;
		printk(BIOS_WARNING, "GPIO IO port configured,"
				     " but no GPIO enabled. Disabling...");
	}
}

static void nct5104d_init(struct device *dev)
{
	struct superio_nuvoton_nct5104d_config *conf = dev->chip_info;

	if (!dev->enabled)
		return;

	pnp_enter_conf_mode(dev);

	switch (dev->path.pnp.device) {
	case NCT5104D_SP1:
	case NCT5104D_SP2:
		set_irq_trigger_type(dev, conf->irq_trigger_type != 0);
		break;
	case NCT5104D_SP3:
	case NCT5104D_SP4:
		route_pins_to_uart(dev, true);
		set_irq_trigger_type(dev, conf->irq_trigger_type != 0);
		break;
	case NCT5104D_GPIO0:
	case NCT5104D_GPIO1:
		route_pins_to_uart(dev, false);
		__fallthrough;
	case NCT5104D_GPIO6:
		if (conf->reset_gpios)
			reset_gpio_default_in(dev);
		break;
	case NCT5104D_GPIO_PP_OD:
		if (conf->reset_gpios)
			reset_gpio_default_od(dev);
		break;
	case NCT5104D_GPIO_IO:
		disable_gpio_io_port(dev);
		break;
	default:
		break;
	}

	pnp_exit_conf_mode(dev);
}

static struct device_operations ops = {
	.read_resources   = pnp_read_resources,
	.set_resources    = pnp_set_resources,
	.enable_resources = pnp_enable_resources,
	.enable           = pnp_alt_enable,
	.init             = nct5104d_init,
	.ops_pnp_mode     = &pnp_conf_mode_8787_aa,
};

static struct pnp_info pnp_dev_info[] = {
	{ NULL, NCT5104D_FDC,  PNP_IO0 | PNP_IRQ0, 0x07f8, },
	{ NULL, NCT5104D_SP1,  PNP_IO0 | PNP_IRQ0, 0x07f8, },
	{ NULL, NCT5104D_SP2,  PNP_IO0 | PNP_IRQ0, 0x07f8, },
	{ NULL, NCT5104D_SP3,  PNP_IO0 | PNP_IRQ0, 0x07f8, },
	{ NULL, NCT5104D_SP4,  PNP_IO0 | PNP_IRQ0, 0x07f8, },
	{ NULL, NCT5104D_GPIO_WDT},
	{ NULL, NCT5104D_GPIO0},
	{ NULL, NCT5104D_GPIO1},
	{ NULL, NCT5104D_GPIO6},
	{ NULL, NCT5104D_GPIO_PP_OD},
	{ NULL, NCT5104D_GPIO_IO, PNP_IO0, 0x07f8, },
	{ NULL, NCT5104D_PORT80},
};

static void enable_dev(struct device *dev)
{
	pnp_enable_devices(dev, &ops, ARRAY_SIZE(pnp_dev_info), pnp_dev_info);
}

struct chip_operations superio_nuvoton_nct5104d_ops = {
	CHIP_NAME("Nuvoton NCT5104D Super I/O")
	.enable_dev = enable_dev,
};
