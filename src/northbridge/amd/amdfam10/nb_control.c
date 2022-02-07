/* SPDX-License-Identifier: GPL-2.0-only */

/* Configure various power control registers, including processor boost
 * and TDP monitoring support.
 */

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>

#include "amdfam10.h"

static void nb_control_init(struct device *dev)
{
	u32 dword;
	u32 f5x80;
	u8 cu_enabled;
	u8 compute_unit_count = 0;

	printk(BIOS_DEBUG, "NB: Function 5 Northbridge Control.. ");

	/* Determine the number of active compute units on this node */
	f5x80 = pci_read_config32(dev, 0x80);
	cu_enabled = f5x80 & 0xf;
	if (cu_enabled == 0x1)
		compute_unit_count = 1;
	if (cu_enabled == 0x3)
		compute_unit_count = 2;
	if (cu_enabled == 0x7)
		compute_unit_count = 3;
	if (cu_enabled == 0xf)
		compute_unit_count = 4;

	/* Configure Processor TDP Running Average */
	dword = pci_read_config32(dev, 0xe0);
	dword &= ~0xf;				/* RunAvgRange = 0x9 */
	dword |= 0x9;
	pci_write_config32(dev, 0xe0, dword);

	/* Configure northbridge P-states */
	dword = pci_read_config32(dev, 0x170);
	dword &= ~(0x7 << 9);			/* NbPstateThreshold = compute_unit_count */
	dword |= (compute_unit_count & 0x7) << 9;
	pci_write_config32(dev, 0x170, dword);

	printk(BIOS_DEBUG, "done.\n");
}

static struct device_operations mcf5_ops  = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = nb_control_init,
	.scan_bus         = 0,
	.ops_pci          = 0,
};

static const struct pci_driver mcf5_driver_fam15_model10 __pci_driver = {
	.ops    = &mcf5_ops,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = 0x1405,
};

static const struct pci_driver mcf5_driver_fam15 __pci_driver = {
	.ops    = &mcf5_ops,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = 0x1605,
};
