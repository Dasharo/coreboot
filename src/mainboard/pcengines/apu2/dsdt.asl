/* SPDX-License-Identifier: GPL-2.0-only */

/* DefinitionBlock Statement */

#if CONFIG(BOARD_PCENGINES_APU2)
#define DEVICE_NAME "apu2
#elif CONFIG(BOARD_PCENGINES_APU3)
#define DEVICE_NAME "apu3
#elif CONFIG(BOARD_PCENGINES_APU4)
#define DEVICE_NAME "apu4
#elif CONFIG(BOARD_PCENGINES_APU5)
#define DEVICE_NAME "apu5
#elif CONFIG(BOARD_PCENGINES_APU6)
#define DEVICE_NAME "apu6
#elif CONFIG(BOARD_PCENGINES_APU7)
#define DEVICE_NAME "apu7
#endif

DefinitionBlock (
	"dsdt.aml",
	"DSDT",
	ACPI_DSDT_REV_2,
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x00010001	/* OEM Revision */
	)
{	/* Start of ASL file */
	#include <acpi/dsdt_top.asl>

	/* Globals for the platform */
	#include "acpi/mainboard.asl"

	/* Describe the USB Overcurrent pins */
	#include "acpi/usb_oc.asl"

	/* PCI IRQ mapping for the Southbridge */
	#include <southbridge/amd/pi/hudson/acpi/pcie.asl>

	/* Describe the processor tree (\_SB) */
	#include <cpu/amd/pi/00730F01/acpi/cpu.asl>

	/* Contains the supported sleep states for this chipset */
	#include <southbridge/amd/common/acpi/sleepstates.asl>

	/* Contains the Sleep methods (WAK, PTS, GTS, etc.) */
	#include "acpi/sleep.asl"

	/* System Bus */
	Scope(\_SB) { /* Start \_SB scope */
		/* global utility methods expected within the \_SB scope */
		#include <arch/x86/acpi/globutil.asl>

		/* Describe IRQ Routing mapping for this platform (within the \_SB scope) */
		#include "acpi/routing.asl"

		Device(PCI0) {
			/* Describe the AMD Northbridge */
			#include <northbridge/amd/pi/00730F01/acpi/northbridge.asl>

			/* Describe the AMD Fusion Controller Hub Southbridge */
			#include <southbridge/amd/pi/hudson/acpi/fch.asl>
		}

		/* Describe PCI INT[A-H] for the Southbridge */
		#include <southbridge/amd/pi/hudson/acpi/pci_int.asl>

		/* Describe the GPIO controller in southbridge */
		#include "acpi/gpio.asl"
	} /* End \_SB scope */

	/* Describe SMBUS for the Southbridge */
	#include <southbridge/amd/pi/hudson/acpi/smbus.asl>

	/* Define the General Purpose Events for the platform */
	#include "acpi/gpe.asl"

	/* Super IO devices (COM ports) */
	#include "acpi/superio.asl"

	/* GPIO buttons and leds */
	#include "acpi/buttons.asl"
	#include "acpi/leds.asl"
}
/* End of ASL file */
