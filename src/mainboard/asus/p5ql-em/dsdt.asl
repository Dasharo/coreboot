/* SPDX-License-Identifier: GPL-2.0-only */

#include <southbridge/intel/i82801jx/i82801jx.h>

#include <acpi/acpi.h>
DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	ACPI_DSDT_REV_2,
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x00000001	// OEM revision
)
{
	// global NVS and variables
	#include <southbridge/intel/common/acpi/platform.asl>
	#include <southbridge/intel/i82801jx/acpi/globalnvs.asl>

	Scope (\_SB) {
		Device (PCI0)
		{
			#include <northbridge/intel/x4x/acpi/x4x.asl>
			#include <southbridge/intel/i82801jx/acpi/ich10.asl>
			#include <drivers/intel/gma/acpi/default_brightness_levels.asl>
		}
	}

	#include <southbridge/intel/common/acpi/sleepstates.asl>
}
