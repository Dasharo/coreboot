/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpi.h>
DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	0x02,		/* DSDT revision: ACPI v2.0 and up */
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x00010001	/* OEM revision */
)
{
	#include "onboard.h"

	#include <acpi/dsdt_top.asl>
	#include <acpi/platform.asl>
	#include <acpi/globalnvs.asl>
	#include <cpu/intel/common/acpi/cpu.asl>

	Device (\_SB.PCI0)
	{
		#include <acpi/southcluster.asl>

		Device (RP03)
		{
			Name (_ADR, 0x001C0002)  // _ADR: Address
			OperationRegion(RPXX, PCI_Config, 0x00, 0x10)
		}

		Scope (GFX0)
		{
			OperationRegion (IGDP, PCI_Config, 0xE0, 0x04)
			Field (IGDP, AnyAcc, NoLock, Preserve)
			{
				GSSE,   1,
			}
		}
	}

	#include <southbridge/intel/common/acpi/sleepstates.asl>

	#include "acpi/mainboard.asl"
}
