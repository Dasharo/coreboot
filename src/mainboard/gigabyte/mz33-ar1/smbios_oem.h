/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef MAINBOARD_GIGABYTE_MZ33_AR1_SMBIOS_OEM_H
#define MAINBOARD_GIGABYTE_MZ33_AR1_SMBIOS_OEM_H

#include <device/device.h>

/* 24 DIMMs (12 channels, 2 DPC) of up to 256 GiB each */
#define MZ33_MAX_MEMORY_GIB	(6 * 1024)

/* Writes the Gigabyte OEM SMBIOS records (types 0x80, 0x83, 0x84, 0x85) */
int mainboard_smbios_oem_data(struct device *dev, int *handle, unsigned long *current);

#endif
