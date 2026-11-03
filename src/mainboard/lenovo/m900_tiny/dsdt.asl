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
		#include <soc/intel/skylake/acpi/ipu.asl>
		#include <soc/intel/skylake/acpi/systemagent.asl>
		#include <soc/intel/skylake/acpi/pch.asl>
	}

	Scope (\_SB) {
		/* Must match the EC (LDN 0xb) I/O base in devicetree.cb */
		#define NCT6687D_EC_BASE 0xa20
		#include <superio/nuvoton/nct6687d/acpi/dshr.asl>
	}

	#include <southbridge/intel/common/acpi/sleepstates.asl>
}
