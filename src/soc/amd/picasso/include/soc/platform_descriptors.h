/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __PICASSO_PLATFORM_DESCRIPTORS_H__
#define __PICASSO_PLATFORM_DESCRIPTORS_H__

#include <types.h>
#include <platform_descriptors.h>
#include <FspsUpd.h>

/* These tempory macros apply to emmc0_mode field in FSP_S_CONFIG.
 * TODO: Remove when official definitions arrive. */
#define SD_DISABLE		0
#define SD_LOW_SPEED		1
#define SD_HIGH_SPEED		2
#define SD_UHS_I_SDR_50		3
#define SD_UHS_I_DDR_50		4
#define SD_UHS_I_SDR_104	5
#define EMMC_SDR_26		6
#define EMMC_SDR_52		7
#define EMMC_DDR_52		8
#define EMMC_HS200		9
#define EMMC_HS400		10
#define EMMC_HS300		11

/* Mainboard callback to obtain DXI/PCIe and DDI descriptors. */
void mainboard_get_dxio_ddi_descriptors(
		const fsp_dxio_descriptor **dxio_descs, size_t *dxio_num,
		const fsp_ddi_descriptor **ddi_descs, size_t *ddi_num);

#endif /* __PICASSO_PLATFORM_DESCRIPTORS_H__ */
