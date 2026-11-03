/* SPDX-License-Identifier: GPL-2.0-only */

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
	#include <soc/intel/common/block/acpi/acpi/platform.asl>
	#include <soc/intel/common/block/acpi/acpi/globalnvs.asl>
	#include <cpu/intel/common/acpi/cpu.asl>

	Device (\_SB.PCI0) {
		#include <soc/intel/common/block/acpi/acpi/northbridge.asl>
		#include <soc/intel/alderlake/acpi/southbridge.asl>
	}

	Scope (\_SB.PCI0.LPCB)
	{
		#include "acpi/superio.asl"
	}

	#include "acpi/mainboard.asl"

	Scope (\_SB) {
		/* Must match EC_IO_BASE in bootblock.c */
		#define NCT6687D_EC_BASE	0xa20
		/* CPU_FAN and PUMP_FAN */
		#define NCT6687D_DSHR_CPU_FANS	0x03
		#include <superio/nuvoton/nct6687d/acpi/dshr.asl>
	}

	#include <southbridge/intel/common/acpi/sleepstates.asl>
}
