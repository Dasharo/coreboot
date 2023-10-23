/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <dasharo/options.h>
#include <device/device.h>
#include <device/pnp.h>
#include <pc80/keyboard.h>
#include <option.h>
#include <superio/conf_mode.h>

#include "chip.h"
#include "nct6687d.h"

#define MAINBOARD_POWER_OFF	0
#define MAINBOARD_POWER_ON	1
#define MAINBOARD_POWER_KEEP	2

/* 0xeb register definitions */
#define NCT6687D_ACPI_POWER_LOSS_CONTROL_MASK	0x60
#define NCT6687D_ACPI_POWER_LOSS_CONTROL_SHIFT	5
#define NCT6687D_ACPI_POWER_ALWAYS_OFF		(0 << NCT6687D_ACPI_POWER_LOSS_CONTROL_SHIFT)
#define NCT6687D_ACPI_POWER_ALWAYS_ON		(1 << NCT6687D_ACPI_POWER_LOSS_CONTROL_SHIFT)
#define NCT6687D_ACPI_POWER_PREV_STATE		(2 << NCT6687D_ACPI_POWER_LOSS_CONTROL_SHIFT)
#define NCT6687D_ACPI_POWER_USER_DEFINED	(3 << NCT6687D_ACPI_POWER_LOSS_CONTROL_SHIFT)

/* 0xe7 register definitions */
#define NCT6687D_ACPI_ON_PSOUT_MASK		0x10
#define NCT6687D_ACPI_ON_PSOUT_SHIFT		4
#define NCT6687D_ACPI_ON_PSOUT_DIS		(0 << NCT6687D_ACPI_ON_PSOUT_SHIFT)
#define NCT6687D_ACPI_ON_PSOUT_EN		(1 << NCT6687D_ACPI_ON_PSOUT_SHIFT)

static void nct6687d_configure_power_fail_state(struct device *dev)
{
	uint8_t reg_eb, reg_e7;
	uint8_t power_status;
	/* Set power state after power fail */
	power_status = dasharo_get_power_on_after_fail();

	pnp_enter_conf_mode(dev);
	pnp_set_logical_device(dev);

	reg_eb = pnp_read_config(dev, 0xeb);
	reg_e7 = pnp_read_config(dev, 0xe7);
	reg_eb &= ~NCT6687D_ACPI_POWER_LOSS_CONTROL_MASK;
	reg_e7 &= ~NCT6687D_ACPI_ON_PSOUT_MASK;

	if (power_status == MAINBOARD_POWER_ON) {
		reg_eb |= NCT6687D_ACPI_POWER_ALWAYS_ON;
		reg_e7 |= NCT6687D_ACPI_ON_PSOUT_EN;
	} else if (power_status == MAINBOARD_POWER_KEEP) {
		reg_eb |= NCT6687D_ACPI_POWER_PREV_STATE;
		reg_e7 |= NCT6687D_ACPI_ON_PSOUT_EN;
	}

	pnp_write_config(dev, 0xeb, reg_eb);
	pnp_write_config(dev, 0xe7, reg_e7);
	pnp_exit_conf_mode(dev);

	printk(BIOS_INFO, "set power %s after power fail\n",
		power_status ? "on" : "off");
}

static void nct6687d_configure_ps2_wake(struct device *dev)
{
	uint8_t reg;
	struct device *ps2_dev = dev_find_slot_pnp(dev->path.pnp.port, NCT6687D_KBC);

	if (!ps2_dev || !ps2_dev->enabled)
		return;

	pnp_enter_conf_mode(dev);
	pnp_set_logical_device(dev);

	reg = pnp_read_config(dev, 0xe0);
	reg |= 0x01; /* Any character from the keybaord can wake up the system */
	reg &= 0xed; /* Clear MSXKEY and MSRKEY to allow either left or right click to wake */
	pnp_write_config(dev, 0xe0, reg);

	reg = pnp_read_config(dev, 0xe6);
	reg |= 0x80; /* Set ENMDAT to allow either left or right click to wake */
	pnp_write_config(dev, 0xe6, reg);

	reg = pnp_read_config(dev, 0xe7);
	reg &= 0x3f; /* Disable 2nd and 3rd set of wake-up key combinations */
	reg |= 0x20; /* Enable Win98 dedicated wake up key */
	pnp_write_config(dev, 0xe7, reg);

	reg = pnp_read_config(dev, 0xe4);
	reg |= 0xc0; /* Enable keyboard and mouse wake via PSOUT# */
	reg |= 0x08; /* Enable keyboard wake with any key */
	pnp_write_config(dev, 0xe4, reg);

	pnp_exit_conf_mode(dev);
}

static void nct6687d_init(struct device *dev)
{
	const struct superio_nuvoton_nct6687d_config *conf;
	const struct resource *res;

	if (!dev->enabled)
		return;

	switch (dev->path.pnp.device) {
	case NCT6687D_KBC:
		pc_keyboard_init(PROBE_AUX_DEVICE);
		break;
	case NCT6687D_ACPI:
		nct6687d_configure_power_fail_state(dev);
		nct6687d_configure_ps2_wake(dev);
		break;
	case NCT6687D_EC:
		conf = dev->chip_info;
		res = probe_resource(dev, PNP_IDX_IO0);
		if (!conf || !res)
			break;
		nct6687d_hwm_init(res->base, conf);
	}
}

static struct device_operations ops = {
	.read_resources   = pnp_read_resources,
	.set_resources    = pnp_set_resources,
	.enable_resources = pnp_enable_resources,
	.enable           = pnp_alt_enable,
	.init             = nct6687d_init,
	.ops_pnp_mode     = &pnp_conf_mode_8787_aa,
};

static struct pnp_info pnp_dev_info[] = {
	{ NULL, NCT6687D_PP, PNP_IO0 | PNP_IRQ0 | PNP_DRQ0 | PNP_MSC0, 0x0FF8,},
	{ NULL, NCT6687D_SP1, PNP_IO0 | PNP_IRQ0 | PNP_MSC0, 0x0FF8, },
	{ NULL, NCT6687D_SP2, PNP_IO0 | PNP_IRQ0 | PNP_MSC0 | PNP_MSC1, 0x0FF8, },
	{ NULL, NCT6687D_KBC, PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_IRQ1 | PNP_MSC0,
		0x0FFF, 0x0FFF, },
	{ NULL, NCT6687D_CIR, PNP_IO0 | PNP_IRQ0 | PNP_MSC0 | PNP_MSC1 | PNP_MSC2 | PNP_MSC3,
		0x0FF8, },
	{ NULL, NCT6687D_GPIO_0_7, PNP_IO0 | PNP_IRQ0, 0x0FF0,},
	{ NULL, NCT6687D_P80_UART},
	{ NULL, NCT6687D_GPIO_8_9_AF},
	{ NULL, NCT6687D_ACPI, PNP_IO0 | PNP_IRQ0, 0x0FF8,},
	{ NULL, NCT6687D_EC, PNP_IO0 | PNP_IRQ0, 0x0FF8,},
	{ NULL, NCT6687D_RTC},
	{ NULL, NCT6687D_SLEEP_PWR},
	{ NULL, NCT6687D_TACH_PWM},
	{ NULL, NCT6687D_FREG},
};

static void enable_dev(struct device *dev)
{
	pnp_enable_devices(dev, &ops, ARRAY_SIZE(pnp_dev_info), pnp_dev_info);
}

struct chip_operations superio_nuvoton_nct6687d_ops = {
	.name = "NUVOTON NCT6687D Super I/O",
	.enable_dev = enable_dev,
};
