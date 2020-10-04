/* SPDX-License-Identifier: GPL-2.0-only */

#define ENABLE_TPM

#include <acpi/acpi.h>
DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	0x02,		// DSDT revision: ACPI v2.0 and up
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x20090419	// OEM revision
)
{
	#include <southbridge/intel/common/acpi/platform.asl>

	#include "acpi/platform.asl"

	// global NVS and variables
	#include <southbridge/intel/i82801gx/acpi/globalnvs.asl>

	// General Purpose Events
	#include "acpi/gpe.asl"

	// mainboard specific devices
	#include "acpi/mainboard.asl"

	// Thermal Zone
	#include "acpi/thermal.asl"

	#include <cpu/intel/speedstep/acpi/cpu.asl>

	Scope (\_SB) {
		Device (PCI0)
		{
			#include <northbridge/intel/i945/acpi/i945.asl>
			#include <southbridge/intel/i82801gx/acpi/ich7.asl>
		}
	}

	#include <southbridge/intel/common/acpi/sleepstates.asl>
}
