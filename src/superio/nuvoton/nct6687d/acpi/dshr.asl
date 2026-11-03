/* SPDX-License-Identifier: GPL-2.0-only */

#include <ec/dasharo/ec/driver.h>

/*
 * Dasharo ACPI interface (DSHR0001) backed by the NCT6687D hardware monitor.
 *
 * Registers are accessed through EC port 0 with the same page/index/data
 * protocol as Linux drivers/hwmon/nct6683.c. Port 0 is meant for firmware and
 * is only used by coreboot during boot. Port 1 is left to the OS: Linux
 * nct6683 refuses to load if an ACPI OperationRegion overlaps it. The channels
 * are not hardcoded, they are discovered from the monitor configuration:
 * - CPU temperatures: monitors sourced from PECI or from the PCH CPU reading
 * - Board temperatures: monitors sourced from the local sensor, thermal
 *   diodes or thermistors
 * - Fans: enabled fan inputs. Inputs set in NCT6687D_DSHR_CPU_FANS (bit 0 =
 *   FANIN1) are reported as CPU fans, the remaining ones as chassis fans.
 *
 * Controllable through preprocessor defines:
 * NCT6687D_EC_BASE		I/O base of the EC (LDN 0xb), required
 * NCT6687D_DSHR_CPU_FANS	Bitmask of fan inputs that cool the CPU
 */

#ifndef NCT6687D_EC_BASE
#error "NCT6687D_EC_BASE must be defined"
#endif

#ifndef NCT6687D_DSHR_CPU_FANS
#define NCT6687D_DSHR_CPU_FANS		0x01
#endif

#define NCT6687D_NUM_MON		32
#define NCT6687D_NUM_FANIN		16

/* Page 1 registers */
#define NCT6687D_REG_MON(x)		(0x00 + 2 * (x))	/* MSB: signed degrees C */
#define NCT6687D_REG_FAN_RPM(x)		(0x40 + 2 * (x))	/* 16-bit RPM, MSB first */
#define NCT6687D_REG_MON_CFG(x)		(0xa0 + (x))		/* bits 6:0 = source */
#define NCT6687D_REG_FANIN_CFG(x)	(0xc0 + (x))		/* bit 7 = enabled */

Device (DASH)
{
	Name (_HID, "DSHR0001")
	Name (_UID, 0)
	// Hide the device so that Windows does not warn about a missing driver.
	Name (_STA, 0xB)

	// Version of the ACPI interface
	Name (DSVR, 1)

	OperationRegion (ECP0, SystemIO, NCT6687D_EC_BASE, 0x03)
	Field (ECP0, ByteAcc, NoLock, Preserve)
	{
		PAGE, 8,
		INDX, 8,
		DATA, 8,
	}

	// Read register Arg1 from page Arg0
	Method (RREG, 2, Serialized)
	{
		PAGE = 0xff	// unlock
		PAGE = Arg0
		INDX = Arg1
		Local0 = DATA
		PAGE = 0xff
		Return (Local0)
	}

	// Get the temperature group of monitor Arg0, or 0xff if not a temperature
	Method (MGRP, 1, Serialized)
	{
		Local0 = RREG (1, NCT6687D_REG_MON_CFG (Arg0)) & 0x7f

		// PECI agents 0-3, domains 0-1, or CPU temperature read from PCH
		If (((Local0 >= 0x20) && (Local0 <= 0x27)) || (Local0 == 0x30)) {
			Return (DASHARO_TEMPERATURE_CPU_PACKAGE)
		}
		// Local sensor, thermal diodes and thermistors
		If ((Local0 >= 0x01) && (Local0 <= 0x18)) {
			Return (DASHARO_TEMPERATURE_BOARD)
		}
		Return (0xff)
	}

	// Get the fan group of fan input Arg0, or 0xff if the input is disabled
	Method (FGRP, 1, Serialized)
	{
		If (!(RREG (1, NCT6687D_REG_FANIN_CFG (Arg0)) & 0x80)) {
			Return (0xff)
		}
		If ((1 << Arg0) & NCT6687D_DSHR_CPU_FANS) {
			Return (DASHARO_FAN_CPU)
		}
		Return (DASHARO_FAN_CHASSIS)
	}

	/*
	 * Find the Arg2-th monitor (Arg0 = 0) or fan input (Arg0 = 1) that
	 * belongs to group Arg1. Returns its number, or 0xff if not found.
	 * With Arg2 = 0xff, returns the number of matching channels instead.
	 */
	Method (FIND, 3, Serialized)
	{
		If (Arg0 == 0) {
			Local2 = NCT6687D_NUM_MON
		} Else {
			Local2 = NCT6687D_NUM_FANIN
		}

		Local1 = 0	// Matches so far
		Local0 = 0
		While (Local0 < Local2) {
			If (Arg0 == 0) {
				Local3 = MGRP (Local0)
			} Else {
				Local3 = FGRP (Local0)
			}
			If (Local3 == Arg1) {
				If (Local1 == Arg2) {
					Return (Local0)
				}
				Local1++
			}
			Local0++
		}

		If (Arg2 == 0xff) {
			Return (Local1)
		}
		Return (0xff)
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
					Case (Package () {
						DASHARO_TEMPERATURE_CPU_PACKAGE,
						DASHARO_TEMPERATURE_BOARD })
					{
						Return (FIND (0, Arg1, 0xff))
					}
					Default { Return (0) }
				}
			}
			Case (DASHARO_FEATURE_FAN_TACH)
			{
				Switch (ToInteger (Arg1))
				{
					Case (Package () { DASHARO_FAN_CPU, DASHARO_FAN_CHASSIS })
					{
						Return (FIND (1, Arg1, 0xff))
					}
					Default { Return (0) }
				}
			}
			Default { Return (0) }
		}
	}

	// Get temperature by capability and index
	Method (GTMP, 2, Serialized)
	{
		Local0 = FIND (0, Arg0, Arg1)
		If (Local0 == 0xff) {
			Return (0)
		}

		Local1 = RREG (1, NCT6687D_REG_MON (Local0))
		// Negative temperatures are reported as 0
		If (Local1 & 0x80) {
			Return (0)
		}
		Return (Local1)
	}

	// Get fan tach reading by cap and index
	Method (GFTH, 2, Serialized)
	{
		Local0 = FIND (1, Arg0, Arg1)
		If (Local0 == 0xff) {
			Return (0)
		}

		Local1 = RREG (1, NCT6687D_REG_FAN_RPM (Local0)) << 8
		Local1 |= RREG (1, NCT6687D_REG_FAN_RPM (Local0) + 1)
		Return (Local1)
	}
}
