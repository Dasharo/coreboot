/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpi.h>
DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	ACPI_DSDT_REV_2,
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x20110725
)
{
	#include <acpi/dsdt_top.asl>
	#include "acpi/platform.asl"

	#include <soc/intel/common/block/acpi/acpi/globalnvs.asl>

	#include <cpu/intel/common/acpi/cpu.asl>

	#include <soc/intel/xeon_sp/spr/acpi/uncore.asl>

	// LPC related entries
	Scope (\_SB.PC00)
	{
		#include <soc/intel/xeon_sp/spr/acpi/pch.asl>
	}

	Scope (\_SB)
	{
		/* BMC sensor numbers, from the SDR */
		#define DSHR_IPMI_CPU_TEMPS	0x28			/* TEMP_CPU */
		#define DSHR_IPMI_BOARD_TEMPS	0x54, 0x52, 0x53	/* TEMP_CARDSIDE1-3 */
		/* FAN1_1-FAN6_1, FAN1_2-FAN6_2 */
		#define DSHR_IPMI_CHASSIS_FANS	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, \
						0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b
		#include <drivers/ipmi/acpi/dshr.asl>
	}
}
