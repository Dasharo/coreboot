/* SPDX-License-Identifier: GPL-2.0-or-later */

/*
 * NOTE: The layout of the global_nvs structure below must match the layout
 * in soc/soc/amd/stoneyridge/acpi/globalnvs.asl !!!
 *
 */

#ifndef __SOC_STONEYRIDGE_NVS_H__
#define __SOC_STONEYRIDGE_NVS_H__

#include <commonlib/helpers.h>
#include <stdint.h>
#include <vendorcode/google/chromeos/gnvs.h>
#include <soc/southbridge.h>

struct __packed global_nvs {
	/* Miscellaneous */
	uint8_t		pcnt; /* 0x00 - Processor Count */
	uint8_t		lids; /* 0x01 - LID State */
	uint8_t		pwrs; /* 0x02 - AC Power State */
	uint32_t	cbmc; /* 0x03 - 0x06 - coreboot Memory Console */
	uint64_t	pm1i; /* 0x07 - 0x0e - System Wake Source - PM1 Index */
	uint64_t	gpei; /* 0x0f - 0x16 - GPE Wake Source */
	uint8_t		tmps; /* 0x17 - Temperature Sensor ID */
	uint8_t		tcrt; /* 0x18 - Critical Threshold */
	uint8_t		tpsv; /* 0x19 - Passive Threshold */
	uint8_t		pad1[6];
	aoac_devs_t	aoac; /* 0x20 - AOAC device enables */
	uint16_t	fw00; /* 0x24 - XhciFwRomAddr_Rom, Boot RAM */
	uint16_t	fw02; /* 0x26 - XhciFwRomAddr_Ram, Instr RAM */
	uint32_t	fw01; /* 0x28 - XhciFwRamAddr_Rom, Boot RAM sz/base */
	uint32_t	fw03; /* 0x2c - XhciFwRomAddr_Ram, Instr RAM sz/base */
	uint32_t	eh10; /* 0x30 - EHCI BAR */
	uint8_t		unused[204];

	/* ChromeOS specific (0x100 - 0xfff) */
	chromeos_acpi_t chromeos;
};

check_member(global_nvs, chromeos, GNVS_CHROMEOS_ACPI_OFFSET);

#endif /* __SOC_STONEYRIDGE_NVS_H__ */
