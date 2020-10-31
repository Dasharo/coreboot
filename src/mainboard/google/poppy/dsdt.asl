/* SPDX-License-Identifier: GPL-2.0-only */

#include <variant/ec.h>
#include <variant/gpio.h>

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
	#include <soc/intel/common/block/acpi/acpi/platform.asl>

	/* global NVS and variables */
	#include <soc/intel/skylake/acpi/globalnvs.asl>

	/* CPU */
	#include <cpu/intel/common/acpi/cpu.asl>

	Scope (\_SB)
	{
		Device (PCI0)
		{
			/* Image processing unit */
			#include <soc/intel/skylake/acpi/ipu.asl>
			#include <soc/intel/skylake/acpi/systemagent.asl>
			#include <soc/intel/skylake/acpi/pch.asl>
			#include <drivers/intel/gma/acpi/default_brightness_levels.asl>
		}
	}

#if CONFIG(VARIANT_HAS_CAMERA_ACPI)
	/* Camera */
	#include <variant/acpi/camera.asl>
#endif

	/* Chrome OS specific */
	#include <vendorcode/google/chromeos/acpi/chromeos.asl>

	#include <southbridge/intel/common/acpi/sleepstates.asl>

	/* Chrome OS Embedded Controller */
	Scope (\_SB.PCI0.LPCB)
	{
		/* ACPI code for EC SuperIO functions */
		#include <ec/google/chromeec/acpi/superio.asl>
		/* ACPI code for EC functions */
		#include <ec/google/chromeec/acpi/ec.asl>
	}

	Scope (\_SB)
	{
		/* Dynamic Platform Thermal Framework */
		#include <variant/acpi/dptf.asl>
	}
}
