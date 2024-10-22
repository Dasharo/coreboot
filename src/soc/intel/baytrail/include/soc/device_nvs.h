/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _BAYTRAIL_DEVICE_NVS_H_
#define _BAYTRAIL_DEVICE_NVS_H_

#include <stdint.h>

#define LPSS_NVS_SIO_DMA1	0
#define LPSS_NVS_I2C1		1
#define LPSS_NVS_I2C2		2
#define LPSS_NVS_I2C3		3
#define LPSS_NVS_I2C4		4
#define LPSS_NVS_I2C5		5
#define LPSS_NVS_I2C6		6
#define LPSS_NVS_I2C7		7
#define LPSS_NVS_SIO_DMA2	8
#define LPSS_NVS_SPI		9
#define LPSS_NVS_PWM1		10
#define LPSS_NVS_PWM2		11
#define LPSS_NVS_HSUART1	12
#define LPSS_NVS_HSUART2	13

#define SCC_NVS_MMC		0
#define SCC_NVS_SDIO		1
#define SCC_NVS_SD		2

struct __packed device_nvs {
	/* Device Enabled in ACPI Mode */
	u8	lpss_en[14];
	u8	scc_en[3];
	u8	lpe_en;
	u8	otg_en;

	/* BAR 0 */
	u32	lpss_bar0[14];
	u32	scc_bar0[3];
	u32	lpe_bar0;
	u32	otg_bar0;

	/* BAR 1 */
	u32	lpss_bar1[14];
	u32	scc_bar1[3];
	u32	lpe_bar1;
	u32	otg_bar1;

	/* Extra */
	u32	lpe_fw; /* LPE Firmware */
};

#endif
