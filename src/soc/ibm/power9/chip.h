/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_CHIP_H
#define __SOC_IBM_POWER9_CHIP_H

struct soc_ibm_power9_config {
};

void build_homer_image(void *homer_bar, void *common_occ_area, uint64_t nominal_freq[]);

#endif /* __SOC_CAVIUM_CN81XX_CHIP_H */
