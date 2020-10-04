/* SPDX-License-Identifier: GPL-2.0-only */

/* Enable ACPI _SWS methods */
#include <soc/intel/common/acpi/acpi_wake_source.asl>

External (\_SB.MPTS, MethodObj)
External (\_SB.MWAK, MethodObj)

/*
 * The _PIC method is called by the OS to choose between interrupt
 * routing via the i8259 interrupt controller or the APIC.
 *
 * _PIC is called with a parameter of 0 for i8259 configuration and
 * with a parameter of 1 for Local Apic/IOAPIC configuration.
 */

Method (_PIC, 1)
{
	/* Remember the OS' IRQ routing choice. */
	Store (Arg0, PICM)
}

/*
 * The _PTS method (Prepare To Sleep) is called before the OS is
 * entering a sleep state. The sleep state number is passed in Arg0
 */

Method (_PTS, 1)
{
	If (CondRefOf (\_SB.MPTS))
	{
		\_SB.MPTS (Arg0)
	}
}

/* The _WAK method is called on system wakeup */

Method (_WAK, 1)
{
	If (CondRefOf (\_SB.MWAK))
	{
		\_SB.MWAK (Arg0)
	}

	Return (Package (){ 0, 0 })
}
