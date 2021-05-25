/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_PCI_DEVS_H
#define SOC_PCI_DEVS_H

#include <device/pci_def.h>

#if !defined(__SIMPLE_DEVICE__)
#include <device/device.h>
#define _SOC_DEV(slot, func)	pcidev_on_root(slot, func)
#else
#define _SOC_DEV(slot, func)	PCI_DEV(0, slot, func)
#endif

/* LPC BUS */
#define PCU_DEV			0x14
#define LPC_FUNC		3
#define LPC_DEVFN		PCI_DEVFN(PCU_DEV, LPC_FUNC)
#define SOC_LPC_DEV		_SOC_DEV(PCU_DEV, LPC_FUNC)

#endif /* SOC_PCI_DEVS_H */
