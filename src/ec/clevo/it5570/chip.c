/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <commonlib/bsd/helpers.h>
#include <console/console.h>
#include <device/pnp.h>
#include <pc80/keyboard.h>
#include <superio/conf_mode.h>
#include <stdint.h>

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

static struct device_operations ops = {
	.init			= clevo_it5570_ec_init,
	.read_resources		= clevo_it5570_ec_read_resources,
	.set_resources		= noop_set_resources,
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
