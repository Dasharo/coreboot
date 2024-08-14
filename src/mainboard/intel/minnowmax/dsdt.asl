/* SPDX-License-Identifier: GPL-2.0-only */

#define INCLUDE_LPE  1
#define INCLUDE_SCC  1
#define INCLUDE_EHCI 1
#define INCLUDE_XHCI 1
#define INCLUDE_LPSS 1


#include <acpi/acpi.h>
DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	0x02,		// DSDT revision: ACPI v2.0 and up
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x20110725	// OEM revision
)
{
	#include <acpi/dsdt_top.asl>
	// Some generic macros
	#include <soc/intel/baytrail/acpi/platform.asl>

	// global NVS and variables
	#include <soc/intel/baytrail/acpi/globalnvs.asl>

	#include <cpu/intel/common/acpi/cpu.asl>

	Scope (\_SB) {
		Device (PCI0)
		{
			#include <soc/intel/baytrail/acpi/southcluster.asl>
		}
	}

	/* Chipset specific sleep states */
	#include <southbridge/intel/common/acpi/sleepstates.asl>

	#include "acpi/mainboard.asl"
}
