/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef AMDFAM10_PCI_H
#define AMDFAM10_PCI_H

#include <stdint.h>
#include <device/pci_type.h>
#include <device/pci_def.h>

u32 pci_read_config32_index(pci_devfn_t dev, u32 index_reg, u32 index);
u32 pci_read_config32_index_wait(pci_devfn_t dev, u32 index_reg, u32 index);

#endif
