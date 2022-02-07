/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/pci_ops.h>
#include "pci.h"

/* bit [10,8] are dev func, bit[1,0] are dev index */

u32 pci_read_config32_index(pci_devfn_t dev, u32 index_reg, u32 index)
{
	u32 dword;

	pci_write_config32(dev, index_reg, index);
	dword = pci_read_config32(dev, index_reg + 0x4);
	return dword;
}

u32 pci_read_config32_index_wait(pci_devfn_t dev, u32 index_reg,
		u32 index)
{

	u32 dword;

	index &= ~(1 << 30);
	pci_write_config32(dev, index_reg, index);
	do {
		dword = pci_read_config32(dev, index_reg);
	} while (!(dword & (1 << 31)));
	dword = pci_read_config32(dev, index_reg + 0x4);
	return dword;
}
