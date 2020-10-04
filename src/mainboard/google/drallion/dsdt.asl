/* SPDX-License-Identifier: GPL-2.0-only */

#include <variant/ec.h>

#include <acpi/acpi.h>
DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	0x02,		/* DSDT revision: ACPI v2.0 and up */
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x20110725	/* OEM revision */
)
{
	#include <soc/intel/cannonlake/acpi/platform.asl>

	/* global NVS and variables */
	#include <soc/intel/common/block/acpi/acpi/globalnvs.asl>

	/* CPU */
	#include <cpu/intel/common/acpi/cpu.asl>

	Scope (\_SB) {
		Device (PCI0)
		{
			#include <soc/intel/common/block/acpi/acpi/northbridge.asl>
			#include <soc/intel/cannonlake/acpi/southbridge.asl>
		}
		/* Per board variant mainboard hooks. */
		#include <variant/acpi/mainboard.asl>
	}

#if CONFIG(CHROMEOS)
	/* Chrome OS specific */
	#include <vendorcode/google/chromeos/acpi/chromeos.asl>
	/* VPD support */
	#include <vendorcode/google/chromeos/acpi/vpd.asl>
#endif

	#include <southbridge/intel/common/acpi/sleepstates.asl>

	/* Low power idle table */
	#include <soc/intel/common/acpi/lpit.asl>

	/* Chrome OS Embedded Controller */
	Scope (\_SB.PCI0.LPCB)
	{
		/* ACPI code for EC SuperIO functions */
		#include <ec/google/wilco/acpi/superio.asl>
		/* ACPI code for EC functions */
		#include <ec/google/wilco/acpi/ec.asl>
	}

	/* Dynamic Platform Thermal Framework */
	Scope (\_SB)
	{
		/* Per board variant specific definitions. */
		#include <variant/acpi/dptf.asl>
		/* Include common dptf ASL files */
		#include <soc/intel/common/acpi/dptf/dptf.asl>
	}
}
