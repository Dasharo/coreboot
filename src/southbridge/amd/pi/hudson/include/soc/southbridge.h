/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_SOUTHBRIDGE_H
#define SOC_SOUTHBRIDGE_H

#include <device/pci_def.h>
#include <types.h>

#define PWR_RESET_CFG		0x10
#define   TOGGLE_ALL_PWR_GOOD	BIT(1)

#define PSP_MAILBOX_BAR		PCI_BASE_ADDRESS_2
#define PSP_BAR_ENABLES		0x48
#define  PSP_BAR_ENABLES_ALL	0x3d
#define  BAR3HIDE		0

#define PM1_LIMIT		16
#define GPE0_LIMIT		32
#define TOTAL_BITS(a)		(8 * sizeof(a))

#define PM_ACPI_SMI_CMD		0x6a

#define PWRBTN_STS		(1 << 8)
#define RTC_STS			(1 << 10)
#define PCIEXPWAK_STS		(1 << 14)
#define WAK_STS			(1 << 15)

#endif /* SOC_SOUTHBRIDGE_H */
