/* SPDX-License-Identifier: GPL-2.0-only */

#include <cf9_reset.h>
#include <reset.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>

void do_board_reset(void)
{
	system_reset();
}

void cf9_global_reset_prepare(void)
{
#ifdef __SIMPLE_DEVICE__
	pci_devfn_t dev = PCI_DEV(0, 0x1f, 0);
#else
	struct device *dev = pcidev_on_root(0x1f, 0);
#endif
	/* Set bit 20 for global reset */
	pci_write_config32(dev, 0xac, pci_read_config32(dev, 0xac) | BIT(20));
}