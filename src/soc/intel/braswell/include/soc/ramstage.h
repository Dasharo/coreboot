/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _SOC_RAMSTAGE_H_
#define _SOC_RAMSTAGE_H_

#include <device/device.h>
#include <fsp/ramstage.h>

#include "../../chip.h"

/*
 * The soc_init_pre_device() function is called prior to device
 * initialization, but it's after console and cbmem has been reinitialized.
 */
void soc_init_pre_device(struct soc_intel_braswell_config *config);
void southcluster_enable_dev(struct device *dev);
void scc_enable_acpi_mode(struct device *dev, int iosf_reg, int nvs_index);
void board_silicon_USB2_override(SILICON_INIT_UPD *params);

extern struct pci_operations soc_pci_ops;

#endif /* _SOC_RAMSTAGE_H_ */
