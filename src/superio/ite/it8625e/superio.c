/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <device/device.h>
#include <device/pnp.h>
#include <pc80/keyboard.h>
#include <superio/conf_mode.h>
#include <superio/ite/common/env_ctrl.h>
#include <superio/hwm5_conf.h>

#include "chip.h"
#include "it8625e.h"

#define ITE_EC_SMI_MASK_3	0x06
#define BANK_SEL_MASK		0x60

static inline void it8625e_ec_select_bank(const u16 base, const u8 bank)
{
	uint8_t reg;

	/* BANK_SEL: SMI# Mask Register 3, bits 6-5 */
	reg = pnp_read_hwm5_index(base, ITE_EC_SMI_MASK_3);
	printk(BIOS_DEBUG, "Current SMI_MASK_3: %x\n", reg);
	reg &= ~BANK_SEL_MASK;
	reg |= (bank << 5);
	pnp_write_hwm5_index(base, ITE_EC_SMI_MASK_3, reg);
	printk(BIOS_DEBUG, "Wrote SMI_MASK_3: %x\n", reg);
}

static void enable_peci(const u16 base)
{
	/* Enable PECI interface */
	pnp_write_hwm5_index(base, ITE_EC_INTERFACE_SELECT,
			   ITE_EC_INTERFACE_SEL_PECI | ITE_EC_INTERFACE_SPEED_TOLERANCE);

	/* Setup External Temperature using PECI GetTemp */
	pnp_write_hwm5_index(base, ITE_EC_EXTEMP_ADDRESS, PECI_CLIENT_ADDRESS);
	pnp_write_hwm5_index(base, ITE_EC_EXTEMP_COMMAND, PECI_GETTEMP_COMMAND);
	pnp_write_hwm5_index(base, ITE_EC_EXTEMP_WRITE_LENGTH, PECI_GETTEMP_WRITE_LENGTH);
	pnp_write_hwm5_index(base, ITE_EC_EXTEMP_READ_LENGTH, PECI_GETTEMP_READ_LENGTH);
	pnp_write_hwm5_index(base, ITE_EC_EXTEMP_CONTROL,
		ITE_EC_EXTEMP_CTRL_AUTO_4HZ | ITE_EC_EXTEMP_CTRL_AUTO_START);
}

static void it8625e_ec_extra_setup(u16 base, const struct it8625e_tmpin_config *conf)
{
	/* Program TSS1 registers */
	it8625e_ec_select_bank(base, 0x2);
	pnp_write_hwm5_index(base, 0x1d, conf->tss1[1] << 4 | conf->tss1[0]);
	printk(BIOS_DEBUG, "Wrote TSS1 #1: %x\n", conf->tss1[1] << 4 | conf->tss1[0]);
	pnp_write_hwm5_index(base, 0x1e, conf->tss1[3] << 4 | conf->tss1[2]);
	printk(BIOS_DEBUG, "Wrote TSS1 #2: %x\n", conf->tss1[3] << 4 | conf->tss1[2]);
	pnp_write_hwm5_index(base, 0x1f, conf->tss1[5] << 4 | conf->tss1[4]);
	printk(BIOS_DEBUG, "Wrote TSS1 #3: %x\n", conf->tss1[5] << 4 | conf->tss1[4]);

	/* Return to bank 0 */
	it8625e_ec_select_bank(base, 0x0);

	/* TODO: Check if PECI is used based on TSS regs */
	if (conf->enable_peci)
		enable_peci(base);

}

static void it8625e_init(struct device *dev)
{
	const struct superio_ite_it8625e_config *conf = dev->chip_info;
	const struct resource *res;

	if (!dev->enabled)
		return;

	switch (dev->path.pnp.device) {
	case IT8625E_EC:
		res = probe_resource(dev, PNP_IDX_IO0);
		if (!conf || !res)
			break;
		ite_ec_init(res->base, &conf->ec);
		it8625e_ec_extra_setup(res->base, &conf->extra);
		break;
	case IT8625E_KBCK:
		pc_keyboard_init(NO_AUX_DEVICE);
		break;
	case IT8625E_KBCM:
		break;
	}
}

static struct device_operations ops = {
	.read_resources   = pnp_read_resources,
	.set_resources    = pnp_set_resources,
	.enable_resources = pnp_enable_resources,
	.enable           = pnp_alt_enable,
	.init             = it8625e_init,
	.ops_pnp_mode     = &pnp_conf_mode_870155_aa,
};

static struct pnp_info pnp_dev_info[] = {
	{ NULL, IT8625E_SP1, PNP_IO0 | PNP_IRQ0 | PNP_MSC0, 0x0ff8, },
	{ NULL, IT8625E_SP2, PNP_IO0 | PNP_IRQ0 | PNP_MSC0, 0x0ff8, },
	{ NULL, IT8625E_PP, PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_DRQ0 | PNP_MSC0,
		0x0ff8, 0x0ffc, },
	{ NULL, IT8625E_EC, PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_MSC0 | PNP_MSC1 |
			PNP_MSC2 | PNP_MSC3 | PNP_MSC4 | PNP_MSC5 | PNP_MSC6 | PNP_MSC7 |
			PNP_MSC8 | PNP_MSC9 | PNP_MSCA | PNP_MSCB,
		0x0ff8, 0x0ffc, },
	{ NULL, IT8625E_KBCK, PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_MSC0,
		0x0fff, 0x0fff, },
	{ NULL, IT8625E_KBCM, PNP_IRQ0 | PNP_IRQ1 | PNP_MSC0, },
	{ NULL, IT8625E_GPIO, PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_MSC0 | PNP_MSC1 |
			PNP_MSC2 | PNP_MSC3 | PNP_MSC4 | PNP_MSC5 | PNP_MSC6 | PNP_MSC7 |
			PNP_MSC8 | PNP_MSC9 | PNP_MSCA | PNP_MSCB,
		0x0fff, 0x0fdb, },
	{ NULL, IT8625E_CIR, PNP_IO0 | PNP_IRQ0 | PNP_MSC0, 0x0ff8, },
};

static void enable_dev(struct device *dev)
{
	pnp_enable_devices(dev, &ops, ARRAY_SIZE(pnp_dev_info), pnp_dev_info);
}

struct chip_operations superio_ite_it8625e_ops = {
	CHIP_NAME("ITE IT8625E Super I/O")
	.enable_dev = enable_dev,
};
