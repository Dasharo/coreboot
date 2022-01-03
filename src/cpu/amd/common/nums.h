/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef AMD_NUMS_H
#define AMD_NUMS_H

#include <device/pci_def.h>
#include <device/pci_ops.h>

#define NODE_NUMS 8

// max HC installed at the same time. ...could be bigger than (48+24) if we have 3x4x4
#define HC_NUMS 32

//it could be more bigger
#define HC_POSSIBLE_NUM 32

#if ENV_PCI_SIMPLE_DEVICE
	#define NODE_PCI(x, fn) PCI_DEV(0, CONFIG_CDB + x, fn)
#else
	#define NODE_PCI(x, fn) pcidev_on_root(CONFIG_CDB + x, fn)
#endif

#endif
