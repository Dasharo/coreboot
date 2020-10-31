/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	0x02,
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x20110725
)
{
	#include <cpu/intel/common/acpi/cpu.asl>
	#include <soc/intel/common/block/acpi/acpi/platform.asl>
	#include <soc/intel/skylake/acpi/globalnvs.asl>

	Device (\_SB.PCI0) {
		#include <soc/intel/skylake/acpi/systemagent.asl>
		#include <soc/intel/skylake/acpi/pch.asl>
	}

	#include <southbridge/intel/common/acpi/sleepstates.asl>
}
