/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <device/device.h>
#include <device/pnp.h>
#include <pc80/keyboard.h>
#include <superio/conf_mode.h>

#include "nct6779d.h"

static void nct6779d_init(struct device *dev)
{
	if (!dev->enabled)
		return;

	switch (dev->path.pnp.device) {
	/* TODO: Might potentially need code for HWM or FDC etc. */
	case NCT6779D_KBC:
		pc_keyboard_init(NO_AUX_DEVICE);
		break;
	}
}

static struct device_operations ops = {
	.read_resources   = pnp_read_resources,
	.set_resources    = pnp_set_resources,
	.enable_resources = pnp_enable_resources,
	.enable           = pnp_alt_enable,
	.init             = nct6779d_init,
	.ops_pnp_mode     = &pnp_conf_mode_8787_aa,
};

static struct pnp_info pnp_dev_info[] = {
	{ NULL, NCT6779D_PP, PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, 0x0ff8, },
	{ NULL, NCT6779D_SP1, PNP_IO0 | PNP_IRQ0, 0x0ff8, },
	{ NULL, NCT6779D_SP2, PNP_IO0 | PNP_IRQ0, 0x0ff8, },
	{ NULL, NCT6779D_KBC, PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_IRQ1,
		0x0fff, 0x0fff, },
	{ NULL, NCT6779D_CIR, PNP_IO0 | PNP_IRQ0, 0x0ff8, },
	{ NULL, NCT6779D_ACPI, PNP_MSC2,},
	{ NULL, NCT6779D_HWM_FPLED, PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_MSC0,
		0x0ffe, 0x0ffe, },
	{ NULL, NCT6779D_WDT1},
	{ NULL, NCT6779D_CIRWKUP, PNP_IO0 | PNP_IRQ0, 0x0ff8, },
	{ NULL, NCT6779D_GPIO_PP_OD},
	{ NULL, NCT6779D_PRT80},
	{ NULL, NCT6779D_DSLP},
	{ NULL, NCT6779D_GPIOBASE, PNP_IO0, 0x0ff8, },
	{ NULL, NCT6779D_GPIO0},
	{ NULL, NCT6779D_GPIO1},
	{ NULL, NCT6779D_GPIO2},
	{ NULL, NCT6779D_GPIO3},
	{ NULL, NCT6779D_GPIO4},
	{ NULL, NCT6779D_GPIO5, PNP_MSC4 | PNP_MSC5},
	{ NULL, NCT6779D_GPIO6, PNP_MSC4 | PNP_MSC5},
	{ NULL, NCT6779D_GPIO7},
	{ NULL, NCT6779D_GPIO8},
};

static void enable_dev(struct device *dev)
{
	pnp_enable_devices(dev, &ops, ARRAY_SIZE(pnp_dev_info), pnp_dev_info);
}

struct chip_operations superio_nuvoton_nct6779d_ops = {
	CHIP_NAME("NUVOTON NCT6779D Super I/O")
	.enable_dev = enable_dev,
};
