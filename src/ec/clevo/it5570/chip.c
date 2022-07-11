/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <acpi/acpi_device.h>
#include <acpi/acpigen.h>
#include <arch/io.h>
#include <cbmem.h>
#include <commonlib/bsd/helpers.h>
#include <console/console.h>
#include <device/pnp.h>
#include <pc80/keyboard.h>
#include <superio/conf_mode.h>
#include <stdint.h>
#include <stdio.h>
#include "chip.h"

#define I2EC_ADDR_L			0x10
#define I2EC_ADDR_H			0x11
#define I2EC_DATA			0x12

#define IT5570_SMFI_LDN			0xf

#define IT5570_SMFI_LPCMWBA		0xf0	/* LPC Memory Window Base Address */
#define IT5570_SMFI_HLPCRAMBA		0xf5	/* H2RAM-HLPC Base Address */
#define IT5570_SMFI_HLPCRAMBA_24	0xfc	/* H2RAM-HLPC Base Address */

#define IT5570_SMFI_HRAMWC		0x105a	/* Host RAM Window Control */
#define IT5570_SMFI_HRAMW0AAS		0x105d	/* Host RAM Window 0 Access Allow Size */

enum {
	H2RAM_WINDOW_16B,
	H2RAM_WINDOW_32B,
	H2RAM_WINDOW_64B,
	H2RAM_WINDOW_128B,
	H2RAM_WINDOW_256B,
	H2RAM_WINDOW_512B,
	H2RAM_WINDOW_1024B,
	H2RAM_WINDOW_2048B,
};

/* Depth 2 space is always avaiable, no need for conf mode */
static void i2ec_depth2_write(uint8_t index, uint8_t data)
{
	outb(0x2e,0x2e);
	outb(index, 0x2f);
	outb(0x2f, 0x2e);
	outb(data, 0x2f);
}

static uint8_t i2ec_depth2_read(uint8_t index)
{
	outb(0x2e,0x2e);
	outb(index, 0x2f);
	outb(0x2f, 0x2e);
	return inb(0x2f);
}

static uint8_t i2ec_direct_read(uint16_t addr)
{
	i2ec_depth2_write(I2EC_ADDR_H, addr >> 8);
	i2ec_depth2_write(I2EC_ADDR_L, addr & 0xff);
	return i2ec_depth2_read(I2EC_DATA);
}

static void i2ec_direct_write(uint16_t addr, uint8_t data)
{

	i2ec_depth2_write(I2EC_ADDR_H, addr >> 8);
	i2ec_depth2_write(I2EC_ADDR_L, addr & 0xff);
	i2ec_depth2_write(I2EC_DATA, data);
}

static void it5570_set_h2ram_base(struct device *dev, uint32_t addr)
{
	uint8_t *h2ram_base = (uint8_t *)&addr;

	if (h2ram_base[3] != 0xfe && h2ram_base[3] != 0xff) {
		printk(BIOS_ERR, "ERROR: Invalid H2RAM memory window base\n");
		return;
	}

	pnp_enter_conf_mode(dev);
	pnp_set_logical_device(dev);

	/* Configure LPC memory window for LGMR */
	pnp_write_config(dev, IT5570_SMFI_LPCMWBA, h2ram_base[3]);
	pnp_write_config(dev, IT5570_SMFI_LPCMWBA + 1, h2ram_base[2]);

	/* Configure H2RAM-HLPC base */
	pnp_write_config(dev, IT5570_SMFI_HLPCRAMBA + 1, h2ram_base[2]);
	pnp_write_config(dev, IT5570_SMFI_HLPCRAMBA, h2ram_base[1]);
	/* H2RAM-HLPC[24] */
	pnp_write_config(dev, IT5570_SMFI_HLPCRAMBA_24, h2ram_base[3] & 1);

	pnp_exit_conf_mode(dev);

	/* Set H2RAM window 0 size to 1K */
	i2ec_direct_write(IT5570_SMFI_HRAMW0AAS,
			  i2ec_direct_read(IT5570_SMFI_HRAMW0AAS) | H2RAM_WINDOW_1024B);
	/* Set H2RAM through LPC Memory cycle, enable window 0 */
	i2ec_direct_write(IT5570_SMFI_HRAMWC,
			  (i2ec_direct_read(IT5570_SMFI_HRAMWC) & 0xef) | 1);
}

static bool is_curve_valid(struct ec_clevo_it5570_fan_curve curve)
{
	int i;

	/*
	 * Fan curve speeds have to be non-decreasing.
	 * Fan curve temperatures have to be increasing (to avoid division by 0).
	 * This also covers the case when the curve is all zeroes (i.e. not configured).
	 */

	for (i = 1; i < IT5570_FAN_CURVE_LEN; ++i)
		if (   curve.speed[i]       <  curve.speed[i-1]
		    || curve.temperature[i] <= curve.temperature[i-1])
			return false;

	return true;
}

static void write_it5570_fan_curve(struct ec_clevo_it5570_fan_curve curve, int fan)
{
	uint16_t ramp;
	int j;
	char fieldname[5];

	/* Curve points */
	for (j = 0; j < 4; ++j) {
		snprintf(fieldname, 5, "P%dF%d", j+1, fan+1);
		acpigen_write_store_int_to_namestr(curve.temperature[j], fieldname);
		snprintf(fieldname, 5, "P%dD%d", j+1, fan+1);
		acpigen_write_store_int_to_namestr(curve.speed[j] * 255 / 100, fieldname);
	}

	/* Ramps */
	for (j = 0; j < 3; ++j) {
		snprintf(fieldname, 5, "SH%d%d", fan+1, j+1);
		ramp	= (float)(curve.speed[j+1]       - curve.speed[j])
			/ (float)(curve.temperature[j+1] - curve.temperature[j])
			* 2.55 * 16.0;
		acpigen_write_store_int_to_namestr(ramp >> 8, fieldname);
		snprintf(fieldname, 5, "SL%d%d", fan+1, j+1);
		acpigen_write_store_int_to_namestr(ramp & 0xFF, fieldname);
	}
}

static void clevo_it5570_ec_init(struct device *dev)
{
	if (!dev->enabled)
		return;

	printk(BIOS_SPEW, "Clevo IT5570 EC init\n");

	struct device sio_smfi = {
		.path.type = DEVICE_PATH_PNP,
		.path.pnp.port = 0x2e,
		.path.pnp.device = IT5570_SMFI_LDN,
	};
	sio_smfi.ops->ops_pnp_mode = &pnp_conf_mode_870155_aa;

	it5570_set_h2ram_base(&sio_smfi, CONFIG_EC_CLEVO_IT5570_RAM_BASE & 0xfffff000);

	pc_keyboard_init(NO_AUX_DEVICE);
}

static void clevo_it5570_ec_resource(struct device *dev, int index, size_t base, size_t size)
{
	struct resource *res = new_resource(dev, index);
	res->flags = IORESOURCE_MEM | IORESOURCE_ASSIGNED | IORESOURCE_FIXED;
	res->base = base;
	res->size = size;
}

static void clevo_it5570_ec_read_resources(struct device *dev)
{
	/* H2RAM resource */
	clevo_it5570_ec_resource(dev, 0, CONFIG_EC_CLEVO_IT5570_RAM_BASE & 0xffff0000, 64*KiB);
}

static void clevo_it5570_ec_fill_ssdt_generator(const struct device *dev)
{
	const struct ec_clevo_it5570_config *config = config_of(dev);
	uint8_t i;

	/* Fan curve */
	/* have the function exist even if the fan curve isn't enabled in devicetree */
	acpigen_write_scope(acpi_device_path(dev));
	acpigen_write_method("SFCV", 0);
	if (config->fan_mode == FAN_MODE_CUSTOM) {
		if (!is_curve_valid(config->fans[0].curve)) {
			printk (BIOS_ERR, "EC: Fan 0 curve invalid. Check your devicetree.cb.\n");
			acpigen_pop_len(); /* Method */
			acpigen_pop_len(); /* Scope */
			return;
		}

		for (i = 0; i < IT5570_FAN_CNT; ++i) {
			if (!is_curve_valid(config->fans[i].curve)) {
				printk (BIOS_WARNING, "EC: Fan %d curve invalid. Using fan 0 curve.\n", i);
				write_it5570_fan_curve(config->fans[0].curve, i);
			} else {
				write_it5570_fan_curve(config->fans[i].curve, i);
			}
		}

		/* Enable custom fan mode */
		acpigen_write_store_int_to_namestr(0x04, "FDAT");
		acpigen_write_store_int_to_namestr(0xD7, "FCMD");
	}
	acpigen_pop_len(); /* Method */
	acpigen_pop_len(); /* Scope */
}

static const char *clevo_it5570_ec_acpi_name(const struct device *dev)
{
	return "EC0";
}

static struct device_operations ops = {
	.init			= clevo_it5570_ec_init,
	.read_resources		= clevo_it5570_ec_read_resources,
	.set_resources		= noop_set_resources,
	.acpi_fill_ssdt		= clevo_it5570_ec_fill_ssdt_generator,
	.acpi_name		= clevo_it5570_ec_acpi_name,
};

static struct pnp_info info[] = {
	{ NULL, 0, 0, 0, }
};

static void clevo_it5570_ec_enable_dev(struct device *dev)
{
	pnp_enable_devices(dev, &ops, ARRAY_SIZE(info), info);
}

struct chip_operations ec_clevo_it5570_ops = {
	CHIP_NAME("Clevo ITE IT5570 EC")
	.enable_dev = clevo_it5570_ec_enable_dev,
};
