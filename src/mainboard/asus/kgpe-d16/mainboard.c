/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/device.h>
#include <southbridge/amd/sr5650/cmn.h>

void set_pcie_reset(void)
{
	struct device *pcie_core_dev;

	pcie_core_dev = pcidev_on_root(0, 0);
	set_htiu_enable_bits(pcie_core_dev, 0xA8, 0xFFFFFFFF, 0x28282828);
	set_htiu_enable_bits(pcie_core_dev, 0xA9, 0x000000FF, 0x00000028);
}

void set_pcie_dereset(void)
{
	struct device *pcie_core_dev;

	pcie_core_dev = pcidev_on_root(0, 0);
	set_htiu_enable_bits(pcie_core_dev, 0xA8, 0xFFFFFFFF, 0x6F6F6F6F);
	set_htiu_enable_bits(pcie_core_dev, 0xA9, 0x000000FF, 0x0000006F);
}

static void mainboard_enable(struct device *dev)
{
	printk(BIOS_INFO, "Mainboard KGPE-D16 Enable. dev=0x%p\n", dev);
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
