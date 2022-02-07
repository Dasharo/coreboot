/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>

#include "sb700.h"
#include "chip.h"

#if CONFIG(SOUTHBRIDGE_AMD_SUBTYPE_SP5100)
#define SB_NAME "ATI SP5100"
#else
#define SB_NAME "ATI SB700"
#endif

static struct device *find_sm_dev(struct device *dev, u32 devfn)
{
	struct device *sm_dev;

	sm_dev = pcidev_path_behind(dev->bus, devfn);
	if (!sm_dev)
		return sm_dev;

	if ((sm_dev->vendor != PCI_VENDOR_ID_ATI) ||
	    ((sm_dev->device != PCI_DEVICE_ID_ATI_SB700_SM))) {
		u32 id;
		id = pci_read_config32(sm_dev, PCI_VENDOR_ID);
		if ((id !=
		     (PCI_VENDOR_ID_ATI | (PCI_DEVICE_ID_ATI_SB700_SM << 16))))
		{
			sm_dev = 0;
		}
	}

	return sm_dev;
}

void set_sm_enable_bits(struct device *sm_dev, u32 reg_pos, u32 mask, u32 val)
{
	u32 reg_old, reg;
	reg = reg_old = pci_read_config32(sm_dev, reg_pos);
	reg &= ~mask;
	reg |= val;
	if (reg != reg_old) {
		pci_write_config32(sm_dev, reg_pos, reg);
	}
}

static void set_pmio_enable_bits(struct device *sm_dev, u32 reg_pos,
				 u32 mask, u32 val)
{
	u8 reg_old, reg;
	reg = reg_old = pmio_read(reg_pos);
	reg &= ~mask;
	reg |= val;
	if (reg != reg_old) {
		pmio_write(reg_pos, reg);
	}
}

void sb7xx_51xx_enable(struct device *dev)
{
	struct device *sm_dev = NULL;
	struct device *bus_dev = NULL;
	int index;
	u32 device_id;
	u32 vendor_id;
	int i;
	u32 devfn;

	printk(BIOS_DEBUG, "sb7xx_51xx_enable()\n");

	/*
	 * 0:11.0  SATA	bit 8 of sm_dev 0xac : 1 - enable, default         + 32 * 3
	 * 0:12.0  OHCI0-USB1	bit 0 of sm_dev 0x68
	 * 0:12.1  OHCI1-USB1	bit 1 of sm_dev 0x68
	 * 0:12.2  EHCI-USB1	bit 2 of sm_dev 0x68
	 * 0:13.0  OHCI0-USB2	bit 4 of sm_dev 0x68
	 * 0:13.1  OHCI1-USB2	bit 5 of sm_dev 0x68
	 * 0:13.2  EHCI-USB2	bit 6 of sm_dev 0x68
	 * 0:14.5  OHCI0-USB3	bit 7 of sm_dev 0x68
	 * 0:14.0  SMBUS							0
	 * 0:14.1  IDE							1
	 * 0:14.2  HDA	bit 3 of pm_io 0x59 : 1 - enable, default	    + 32 * 4
	 * 0:14.3  LPC	bit 20 of sm_dev 0x64 : 0 - disable, default  + 32 * 1
	 * 0:14.4  PCI							4
	 */
	if (dev->device == 0x0000) {
		vendor_id = pci_read_config32(dev, PCI_VENDOR_ID);
		device_id = (vendor_id >> 16) & 0xffff;
		vendor_id &= 0xffff;
	} else {
		vendor_id = dev->vendor;
		device_id = dev->device;
	}

	bus_dev = dev->bus->dev;
	if ((bus_dev->vendor == PCI_VENDOR_ID_ATI) &&
	    (bus_dev->device == PCI_DEVICE_ID_ATI_SB700_PCI)) {
		devfn = (bus_dev->path.pci.devfn) & ~7;
		sm_dev = find_sm_dev(bus_dev, devfn);
		if (!sm_dev)
			return;

		/* something under 00:01.0 */
		switch (dev->path.pci.devfn) {
		case 5 << 3:
			;
		}
		return;
	}

	i = (dev->path.pci.devfn) & ~7;
	i += (3 << 3);
	for (devfn = (0x14 << 3); devfn <= i; devfn += (1 << 3)) {
		sm_dev = find_sm_dev(dev, devfn);
		if (sm_dev)
			break;
	}
	if (!sm_dev)
		return;

	switch (dev->path.pci.devfn - (devfn - (0x14 << 3))) {
	case PCI_DEVFN(0x11, 0):
		index = 8;
		set_sm_enable_bits(sm_dev, 0xac, 1 << index,
				   (dev->enabled ? 1 : 0) << index);
		break;
	case PCI_DEVFN(0x12, 0):
	case PCI_DEVFN(0x12, 1):
	case PCI_DEVFN(0x12, 2):
		index = dev->path.pci.devfn & 3;
		set_sm_enable_bits(sm_dev, 0x68, 1 << index,
				   (dev->enabled ? 1 : 0) << index);
		break;
	case PCI_DEVFN(0x13, 0):
	case PCI_DEVFN(0x13, 1):
	case PCI_DEVFN(0x13, 2):
		index = (dev->path.pci.devfn & 3) + 4;
		set_sm_enable_bits(sm_dev, 0x68, 1 << index,
				   (dev->enabled ? 1 : 0) << index);
		break;
	case PCI_DEVFN(0x14, 5):
		index = 7;
		set_sm_enable_bits(sm_dev, 0x68, 1 << index,
				   (dev->enabled ? 1 : 0) << index);
		break;
	case PCI_DEVFN(0x14, 0):
		break;
	case PCI_DEVFN(0x14, 1):
		break;
	case PCI_DEVFN(0x14, 2):
		index = 3;
		set_pmio_enable_bits(sm_dev, 0x59, 1 << index,
				     (dev->enabled ? 1 : 0) << index);
		break;
	case PCI_DEVFN(0x14, 3):
		index = 20;
		set_sm_enable_bits(sm_dev, 0x64, 1 << index,
				   (dev->enabled ? 1 : 0) << index);
		break;
	case PCI_DEVFN(0x14, 4):
		break;
	default:
		printk(BIOS_DEBUG, "unknown dev: %s device_id=%4x\n", dev_path(dev),
			     device_id);
	}
}

struct chip_operations southbridge_amd_sb700_ops = {
	CHIP_NAME(SB_NAME)
	.enable_dev = sb7xx_51xx_enable,
};
