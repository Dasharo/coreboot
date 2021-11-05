/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SB700_H
#define SB700_H

#include <types.h>
#include <device/device.h>

/* Power management index/data registers */
#define BIOSRAM_INDEX	0xcd4
#define BIOSRAM_DATA	0xcd5
#define PM_INDEX	0xcd6
#define PM_DATA		0xcd7
#define PM2_INDEX	0xcd0
#define PM2_DATA	0xcd1

#define SB700_ACPI_IO_BASE 0x800

#define ACPI_PM_EVT_BLK		(SB700_ACPI_IO_BASE + 0x00) /* 4 bytes */
#define ACPI_PM1_CNT_BLK	(SB700_ACPI_IO_BASE + 0x04) /* 2 bytes */
#define ACPI_PMA_CNT_BLK	(SB700_ACPI_IO_BASE + 0x16) /* 1 byte */
#define ACPI_PM_TMR_BLK		(SB700_ACPI_IO_BASE + 0x20) /* 4 bytes */
#define ACPI_GPE0_BLK		(SB700_ACPI_IO_BASE + 0x18) /* 8 bytes */
#define ACPI_CPU_CONTROL	(SB700_ACPI_IO_BASE + 0x08) /* 6 bytes */
#define ACPI_CPU_P_LVL2		(ACPI_CPU_CONTROL + 0x4)    /* 1 byte */

#endif /* SB700_H */
