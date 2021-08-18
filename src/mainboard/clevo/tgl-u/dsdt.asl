/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <baseboard/gpio.h>

DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	ACPI_DSDT_REV_2,
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x20110725	/* OEM revision */
)
{
	#include <acpi/dsdt_top.asl>
	#include <soc/intel/common/block/acpi/acpi/platform.asl>

	/* global NVS and variables */
	#include <soc/intel/common/block/acpi/acpi/globalnvs.asl>

	/* CPU */
	#include <cpu/intel/common/acpi/cpu.asl>

	Device (\_SB.PCI0)
	{
		#include <soc/intel/common/block/acpi/acpi/northbridge.asl>
		#include <soc/intel/tigerlake/acpi/southbridge.asl>
		#include <soc/intel/tigerlake/acpi/tcss.asl>
		#include <soc/intel/common/block/acpi/acpi/ipu.asl>
	}

	Scope (\_SB.PCI0.LPCB)
	{
		#include <drivers/pc80/pc/ps2_controller.asl>
		#include <ec/clevo/it5570/ec.asl>
	}

	#include <southbridge/intel/common/acpi/sleepstates.asl>
	#include <acpi/mainboard.asl>
}
