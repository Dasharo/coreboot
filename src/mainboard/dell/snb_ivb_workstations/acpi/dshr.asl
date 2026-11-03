/* SPDX-License-Identifier: GPL-2.0-only */

#include <ec/dasharo/ec/driver.h>

/*
 * Dasharo ACPI interface (DSHR0001) backed by the SCH5545 hardware monitor.
 *
 * The HWM registers are not directly accessible, they are "virtual registers"
 * accessed through the EC mailbox of the Embedded Memory Interface. The
 * protocol and the register layout are the same as on SCH5627, see Linux
 * drivers/hwmon/sch56xx-common.c and drivers/hwmon/sch5627.c.
 */

/* HWM virtual registers */
#define SCH5545_HWM_TEMP_CPU		0x2b	/* MSB, degrees C + 64 */
#define SCH5545_HWM_TEMP_BOARD		0x26	/* MSB, degrees C + 64 */
#define SCH5545_HWM_FAN_CPU		0x2c	/* 16-bit tach count, LSB first */
#define SCH5545_HWM_FAN_CHASSIS		0x2e	/* 16-bit tach count, LSB first */

#define SCH5545_VREG_READ		0x02
#define SCH5545_VREG_FAIL		0xffff

Device (DASH)
{
	Name (_HID, "DSHR0001")
	Name (_UID, 0)
	// Hide the device so that Windows does not warn about a missing driver.
	Name (_STA, 0xB)

	// Version of the ACPI interface
	Name (DSVR, 1)

	OperationRegion (EMIR, SystemIO, SCH5545_EMI_BASE, 0x10)
	Field (EMIR, ByteAcc, NoLock, Preserve)
	{
		H2EC, 8,	// Host-to-EC mailbox
		EC2H, 8,	// EC-to-Host mailbox
		ECAL, 8,	// EC address LSB (bits 1:0 = access type)
		ECAH, 8,	// EC address MSB
		ECD0, 8,	// EC data bytes 0-3
		ECD1, 8,
		ECD2, 8,
		ECD3, 8,
		ISRL, 8,	// Interrupt source LSB
	}

	/*
	 * Read one byte HWM virtual register Arg0.
	 * Returns SCH5545_VREG_FAIL if the EC does not respond.
	 * Serialized, as this is the only user of the EMI mailbox at runtime.
	 */
	Method (VRRD, 1, Serialized)
	{
		// Write-clear the EC-to-Host mailbox
		Local0 = EC2H
		EC2H = Local0

		// Request packet at the start of mailbox region 1 (0x8000)
		ECAL = 0x00
		ECAH = 0x80
		ECD0 = SCH5545_VREG_READ
		ECD1 = 0x01		// One 8-bit entry
		// Point at the first data entry (0x8004)
		ECAL = 0x04
		ECD2 = Arg0 & 0xff
		ECD3 = (Arg0 >> 8) & 0xff

		// Execute the command
		H2EC = 0x01

		/*
		 * The EC responds within 15 ms. Poll for completion like Linux:
		 * 64 busy polls, then up to 32 more with 1 ms sleeps.
		 */
		Local1 = 0
		While (Local1 < 96) {
			If (Local1 >= 64) {
				Sleep (1)
			}
			Local0 = ISRL
			If (Local0 != 0) {
				ISRL = Local0
			}
			If (Local0 & 0x01) {
				Break
			}
			Local1++
		}
		If (Local1 >= 96) {
			Return (SCH5545_VREG_FAIL)
		}

		Local1 = 0
		While (Local1 < 64) {
			If (EC2H == 0x01) {
				Return (ECD0)
			}
			Local1++
		}

		Return (SCH5545_VREG_FAIL)
	}

	// Read a temperature register, in degrees C
	Method (RTMP, 1, Serialized)
	{
		Local0 = VRRD (Arg0)
		If ((Local0 == SCH5545_VREG_FAIL) || (Local0 < 64)) {
			Return (0)
		}
		Return (Local0 - 64)
	}

	// Read a fan tach register, in RPM
	Method (RFAN, 1, Serialized)
	{
		// Read LSB first, this latches the matching MSB
		Local0 = VRRD (Arg0)
		If (Local0 == SCH5545_VREG_FAIL) {
			Return (0)
		}
		Local1 = VRRD (Arg0 + 1)
		If (Local1 == SCH5545_VREG_FAIL) {
			Return (0)
		}

		Local0 |= Local1 << 8
		// 0xffff means the fan is stopped or not connected
		If ((Local0 == 0) || (Local0 == 0xffff)) {
			Return (0)
		}
		Return (5400540 / Local0)
	}

	// Get platform features
	Method (GFTR, 0, Serialized)
	{
		Return (
			(1 << DASHARO_FEATURE_TEMPERATURE) |
			(1 << DASHARO_FEATURE_FAN_TACH))
	}

	// Get feature capabilities
	Method (GFCP, 2, Serialized)
	{
		Switch (ToInteger (Arg0))
		{
			Case (DASHARO_FEATURE_TEMPERATURE)
			{
				Switch (ToInteger (Arg1))
				{
					Case (DASHARO_TEMPERATURE_CPU_PACKAGE) { Return (1) }
					Case (DASHARO_TEMPERATURE_BOARD) { Return (1) }
					Default { Return (0) }
				}
			}
			Case (DASHARO_FEATURE_FAN_TACH)
			{
				Switch (ToInteger (Arg1))
				{
					Case (DASHARO_FAN_CPU) { Return (1) }
					Case (DASHARO_FAN_CHASSIS) { Return (1) }
					Default { Return (0) }
				}
			}
			Default { Return (0) }
		}
	}

	// Get temperature by capability and index
	Method (GTMP, 2, Serialized)
	{
		If (Arg1 != 0) { Return (0) }

		Switch (ToInteger (Arg0))
		{
			Case (DASHARO_TEMPERATURE_CPU_PACKAGE)
			{
				Return (RTMP (SCH5545_HWM_TEMP_CPU))
			}
			Case (DASHARO_TEMPERATURE_BOARD)
			{
				Return (RTMP (SCH5545_HWM_TEMP_BOARD))
			}
			Default { Return (0) }
		}
	}

	// Get fan tach reading by cap and index
	Method (GFTH, 2, Serialized)
	{
		If (Arg1 != 0) { Return (0) }

		Switch (ToInteger (Arg0))
		{
			Case (DASHARO_FAN_CPU)
			{
				Return (RFAN (SCH5545_HWM_FAN_CPU))
			}
			Case (DASHARO_FAN_CHASSIS)
			{
				Return (RFAN (SCH5545_HWM_FAN_CHASSIS))
			}
			Default { Return (0) }
		}
	}
}
