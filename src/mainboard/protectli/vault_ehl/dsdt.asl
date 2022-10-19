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
	#include <acpi/dsdt_top.asl>
	/* global NVS and variables */
	#include <soc/intel/elkhartlake/acpi/globalnvs.asl>

	/* CPU */
	#include <cpu/intel/common/acpi/cpu.asl>

	Scope (\_SB) {
		Device (PCI0)
		{
			#include <soc/intel/elkhartlake/acpi/northbridge.asl>
			#include <soc/intel/elkhartlake/acpi/southbridge.asl>
			#include <soc/intel/elkhartlake/acpi/pch_hda.asl>
		}
	}

	#include <southbridge/intel/common/acpi/sleepstates.asl>
}