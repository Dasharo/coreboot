/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pciexp.h>
#include <device/pci_ids.h>
#include <assert.h>

static void pcie_disable(struct device *dev)
{
	printk(BIOS_INFO, "%s: Disabling device\n", dev_path(dev));
	dev->enabled = 0;
}

#if CONFIG(HAVE_ACPI_TABLES)
static const char *pcie_acpi_name(const struct device *dev)
{
	assert(dev);

	if (dev->path.type != DEVICE_PATH_PCI)
		return NULL;

	assert(dev->bus);
	if (dev->bus->secondary == 0)
		switch (dev->path.pci.devfn) {
		case PCI_DEVFN(1, 0):
			return "PEGP";
		case PCI_DEVFN(1, 1):
			return "PEG1";
		case PCI_DEVFN(1, 2):
			return "PEG2";
		};

	struct device *const port = dev->bus->dev;
	assert(port);
	assert(port->bus);

	if (dev->path.pci.devfn == PCI_DEVFN(0, 0) &&
	    port->bus->secondary == 0 &&
	    (port->path.pci.devfn == PCI_DEVFN(1, 0) ||
	    port->path.pci.devfn == PCI_DEVFN(1, 1) ||
	    port->path.pci.devfn == PCI_DEVFN(1, 2)))
		return "DEV0";

	return NULL;
}
#endif

static struct device_operations device_ops = {
	.read_resources		= pci_bus_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_bus_enable_resources,
	.scan_bus		= pciexp_scan_bridge,
	.reset_bus		= pci_bus_reset,
	.disable		= pcie_disable,
	.init			= pci_dev_init,
	.ops_pci		= &pci_dev_ops_pci,
#if CONFIG(HAVE_ACPI_TABLES)
	.acpi_name		= pcie_acpi_name,
#endif
};

static const unsigned short pci_device_ids[] = {
	0x0c01, 0x0c05, 0x0c09, 0x0c0d,
	0x0d01, 0x0d05, 0x0d09, /* Crystal Well */
	0 };

static const struct pci_driver pch_pcie __pci_driver = {
	.ops		= &device_ops,
	.vendor		= PCI_VENDOR_ID_INTEL,
	.devices	= pci_device_ids,
};
