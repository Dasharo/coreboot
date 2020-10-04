/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	0x02,		// DSDT revision: ACPI v2.0 and up
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x20090811	// OEM revision
)
{
	// global NVS and variables
	#include <southbridge/intel/common/acpi/platform.asl>
	#include <southbridge/intel/i82801jx/acpi/globalnvs.asl>

	Device (\_SB.PCI0)
	{
		#include <northbridge/intel/x4x/acpi/x4x.asl>
		#include <southbridge/intel/i82801jx/acpi/ich10.asl>
	}

	#include <southbridge/intel/common/acpi/sleepstates.asl>
}
