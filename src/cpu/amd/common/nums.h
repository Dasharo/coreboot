/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef AMD_NUMS_H
#define AMD_NUMS_H

#include <device/pci_def.h>
#include <device/pci_ops.h>

#if CONFIG_MAX_PHYSICAL_CPUS > 8
	#if CONFIG_MAX_PHYSICAL_CPUS > 32
		#define NODE_NUMS 64
	#else
		#define NODE_NUMS 32
	#endif
#else
	#define NODE_NUMS 8
#endif

// max HC installed at the same time. ...could be bigger than (48+24) if we have 3x4x4
#define HC_NUMS 32

//it could be more bigger
#define HC_POSSIBLE_NUM 32

#if NODE_NUMS == 64
#if ENV_PCI_SIMPLE_DEVICE
	#define NODE_PCI(x, fn) ((x < 32) ? \
		(PCI_DEV(CONFIG_CBB, (CONFIG_CDB + x), fn)) : \
		(PCI_DEV((CONFIG_CBB - 1),(CONFIG_CDB + x - 32), fn)))
#else
	#define NODE_PCI(x, fn) ((x < 32) ? \
		(pcidev_path_on_bus(CONFIG_CBB, PCI_DEVFN((CONFIG_CDB + x), fn))) : \
		(pcidev_path_on_bus((CONFIG_CBB - 1), PCI_DEVFN((CONFIG_CDB + x - 32), fn))))
#endif
#else
#if ENV_PCI_SIMPLE_DEVICE
	#define NODE_PCI(x, fn) PCI_DEV(CONFIG_CBB, (CONFIG_CDB + x), fn)
#else
	#define NODE_PCI(x, fn) pcidev_path_on_bus(CONFIG_CBB, PCI_DEVFN((CONFIG_CDB + x), fn))
#endif
#endif

#endif
