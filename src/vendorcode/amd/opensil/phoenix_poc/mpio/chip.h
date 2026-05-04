/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef OPENSIL_PHOENIX_POC_MPIO_CHIP_H
#define OPENSIL_PHOENIX_POC_MPIO_CHIP_H

#include <stdint.h>

#define MAX_DDI_PORTS 5

/*
 * PHOENIX MPIO mapping
 * PCIE0 -> [0-19] bridges 1.1-1.5
 * PCIE1 -> [20-27] bridges 2.1-2.6
 */

enum mpio_type {
	IFTYPE_UNUSED,
	IFTYPE_PCIE,
	IFTYPE_SATA,
	IFTYPE_DDI,
};

/* Sync with PCIE_HOTPLUG_TYPE */
enum mpio_hotplug {
	HotplugDisabled,                             ///< Hotplug disable
	Basic,                                       ///< Basic Hotplug
	ServerExpress,                               ///< Server Hotplug Express Module
	Enhanced,                                    ///< Enhanced
	Inboard,                                     ///< Inboard
	ServerEntSSD,                                ///< Server Hotplug Enterprise SSD
	UBM,                                         ///< UBM Backplane
	OCP,                                         ///< OCP NIC 3.0
};

enum pcie_link_speed {
	MaxSupported,
	Gen1,
	Gen2,
	Gen3,
	Gen4,
	Gen5,
};

/* Sync with PCIE_ASPM_TYPE */
enum pcie_aspm {
	aspm_disabled,
	L0s,
	L1,
	L0sL1,
};

enum ddi_type {
	ConnDP,
	ConnEDP,
	ConnSingleLinkDVI,
	ConnDualLinkDVI,
	ConnHDMI,
	ConnDpToVga,
	ConnDpToLvds,
	ConnNutmegDpToVga,
	ConnSingleLinkDviI,
	ConnDpWithTypeC,
	ConnDpWithTypeCWithoutRetimer,
	ConnDpWithoutTypeC,
	ConnEDPToLvds,
	ConnEDPToLvdsSwInit,
	ConnAutoDetect,
};

enum ddi_aux {
	DdiAux1,
	DdiAux2,
	DdiAux3,
	DdiAux4,
	DdiAux5,
	DdiAux6,
};

enum ddi_hdp {
	DdiHdp1,
	DdiHdp2,
	DdiHdp3,
	DdiHdp4,
	DdiHdp5,
	DdiHdp6,
};
struct drivers_amd_opensil_mpio_config {
	enum mpio_type type;
	uint8_t start_lane;
	uint8_t end_lane;
	uint8_t gpio_group;
	enum mpio_hotplug hotplug;
	enum pcie_link_speed speed;
	enum pcie_aspm aspm;
	uint8_t aspm_l1_1 : 1;
	uint8_t aspm_l1_2 : 1;
	uint8_t clock_pm : 1;
	uint8_t sb_link : 1;
	/* DDI specific */
	enum ddi_type ddi_connector;
	enum ddi_aux aux;
	enum ddi_hdp hdp;
};

#endif /* OPENSIL_PHOENIX_POC_MPIO_CHIP_H */
