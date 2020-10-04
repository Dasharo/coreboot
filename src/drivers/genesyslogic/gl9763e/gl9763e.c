/* SPDX-License-Identifier: GPL-2.0-only */

/* Driver for Genesys Logic GL9763E */

#include <console/console.h>
#include <device/device.h>
#include <device/path.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <device/pci_ids.h>
#include "gl9763e.h"

static void gl9763e_init(struct device *dev)
{
	printk(BIOS_INFO, "GL9763E: init\n");
	pci_dev_init(dev);

	/* Set VHS (Vendor Header Space) to be writable */
	pci_update_config32(dev, VHS, ~VHS_REV_MASK, VHS_REV_W);
	/* Set single AXI request */
	pci_or_config32(dev, SCR, SCR_AXI_REQ);
	/* Disable L0s support */
	pci_and_config32(dev, CFG_REG_2, ~CFG_REG_2_L0S);
	/* Set SSC to 30000 ppm */
	pci_update_config32(dev, PLL_CTL_2, ~PLL_CTL_2_MAX_SSC_MASK, MAX_SSC_30000PPM);
	/* Enable SSC */
	pci_or_config32(dev, PLL_CTL, PLL_CTL_SSC);
	/* Set VHS to read-only */
	pci_update_config32(dev, VHS, ~VHS_REV_MASK, VHS_REV_R);
}

static struct device_operations gl9763e_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.ops_pci		= &pci_dev_ops_pci,
	.init			= gl9763e_init,
};

static const unsigned short pci_device_ids[] = {
	PCI_DEVICE_ID_GLI_9763E,
	0
};

static const struct pci_driver genesyslogic_gl9763e __pci_driver = {
	.ops		= &gl9763e_ops,
	.vendor		= PCI_VENDOR_ID_GLI,
	.devices	= pci_device_ids,
};

struct chip_operations drivers_generic_genesyslogic_ops = {
	CHIP_NAME("Genesys Logic GL9763E")
};
