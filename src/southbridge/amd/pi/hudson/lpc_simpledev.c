/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/lpc.h>
#include <device/pci.h>

#include "hudson.h"
#include "pci_devs.h"

uintptr_t lpc_get_spibase(void)
{
	u32 base;

	base = pci_read_config32(PCI_DEV(0, LPC_DEV, LPC_FUNC),
				 SPIROM_BASE_ADDRESS_REGISTER);
	base &= 0xffffffc0;
	return (uintptr_t)base;
}
