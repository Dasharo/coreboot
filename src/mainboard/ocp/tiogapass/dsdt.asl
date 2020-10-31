/* SPDX-License-Identifier: GPL-2.0-or-later */


#include <acpi/acpi.h>
DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	ACPI_DSDT_REV_2,
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x20110725	// OEM revision
)
{
	#include "acpi/platform.asl"
	#include <soc/intel/xeon_sp/acpi/globalnvs.asl>
	#include <cpu/intel/common/acpi/cpu.asl>
	#include <soc/intel/xeon_sp/acpi/uncore.asl>
}
