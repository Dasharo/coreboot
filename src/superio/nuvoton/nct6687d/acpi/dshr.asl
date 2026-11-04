/* SPDX-License-Identifier: GPL-2.0-only */

#include <ec/dasharo/ec/driver.h>
#include <superio/nuvoton/nct6687d/nct6687d.h>
#include <superio/nuvoton/nct6687d/nct6687d_hwm.h>

/*
 * Dasharo ACPI interface (DSHR0001) backed by the NCT6687D hardware monitor.
 * Included by superio.asl inside the SuperIO device when NCT6687D_SHOW_DSHR is
 * defined, so that the configuration mode methods and fields can be used.
 *
 * The EC I/O base is read from LDN 0xb on every call, as the OS may move it.
 * Registers are accessed through EC port 0 with the same page/index/data
 * protocol as Linux drivers/hwmon/nct6683.c. Port 0 is meant for firmware and
 * is only used by coreboot during boot. Port 1 is left to the OS driver (Linux
 * nct6683), so the two never share the page/index/data registers. The channels
 * are not hardcoded, they are discovered from the monitor configuration:
 * - CPU temperatures: monitors sourced from PECI, AMD SB-TSI or from the PCH
 *   CPU reading
 * - Board temperatures: monitors sourced from the local sensor, thermal
 *   diodes, thermistors, the PCH chip and MCH readings or SMBus thermal
 *   sensors
 * - Fans: enabled fan inputs. Inputs set in NCT6687D_DSHR_CPU_FANS (bit 0 =
 *   FANIN1) are reported as CPU fans, the remaining ones as chassis fans.
 *   The same channels report the current output duty (0-255) of fans 1-8.
 *
 * Controllable through preprocessor defines:
 * NCT6687D_DSHR_CPU_FANS	Bitmask of fan inputs that cool the CPU
 */

#ifndef NCT6687D_DSHR_CPU_FANS
#define NCT6687D_DSHR_CPU_FANS		0x01
#endif

#define NCT6687D_NUM_MON		32
#define NCT6687D_NUM_FANIN		16

Device (DASH)
{
	Name (_HID, "DSHR0001")
	Name (_UID, 0)
	// Hide the device so that Windows does not warn about a missing driver.
	Name (_STA, 0xB)

	// Version of the ACPI interface
	Name (DSVR, 1)

	// Get the EC I/O base, or 0 if the EC is disabled
	Method (ECBA, 0, Serialized)
	{
		Local0 = 0
		ENTER_CONFIG_MODE (NCT6687D_EC)
		If (PNP_DEVICE_ACTIVE) {
			Local0 = (PNP_IO0_HIGH_BYTE << 8) | PNP_IO0_LOW_BYTE
		}
		EXIT_CONFIG_MODE ()
		Return (Local0)
	}

	// Read register Arg1, encoded as EC_PAGE_REG (page, index), at EC base Arg0
	Method (RREG, 2, Serialized)
	{
		OperationRegion (ECP0, SystemIO, Arg0, 0x03)
		Field (ECP0, ByteAcc, NoLock, Preserve)
		{
			PAGE, 8,
			INDX, 8,
			DATA, 8,
		}

		PAGE = 0xff	// unlock
		PAGE = Arg1 >> 8
		INDX = Arg1 & 0xff
		Local0 = DATA
		PAGE = 0xff
		Return (Local0)
	}

	// Get the temperature group of monitor Arg1, or 0xff if not a temperature
	Method (MGRP, 2, Serialized)
	{
		Local0 = RREG (Arg0, SENSOR_CFG_REG (Arg1)) & SENSOR_SRC_SEL_MASK

		// PECI agents 0-3, domains 0-1, CPU temperature read from PCH,
		// or AMD SB-TSI at addresses 0x90-0x9d
		If (((Local0 >= 0x20) && (Local0 <= 0x27)) || (Local0 == 0x30) ||
		    ((Local0 >= 0x42) && (Local0 <= 0x49))) {
			Return (DASHARO_TEMPERATURE_CPU_PACKAGE)
		}
		// Local sensor, thermal diodes, thermistors, PCH chip and MCH
		// temperatures read from PCH, and SMBus thermal sensors
		If (((Local0 >= 0x01) && (Local0 <= 0x18)) ||
		    (Local0 == 0x31) || (Local0 == 0x33) ||
		    ((Local0 >= 0x38) && (Local0 <= 0x3d))) {
			Return (DASHARO_TEMPERATURE_BOARD)
		}
		Return (0xff)
	}

	// Get the fan group of fan input Arg1 (0-based), or 0xff if the input is disabled
	Method (FGRP, 2, Serialized)
	{
		If (!(RREG (Arg0, FANIN_CFG_REG (Arg1 + 1)) & FANIN_MONITOR_EN)) {
			Return (0xff)
		}
		If ((1 << Arg1) & NCT6687D_DSHR_CPU_FANS) {
			Return (DASHARO_FAN_CPU)
		}
		Return (DASHARO_FAN_CHASSIS)
	}

	/*
	 * Find the Arg3-th monitor (Arg1 = 0) or fan input (Arg1 = 1) that
	 * belongs to group Arg2, at EC base Arg0. Returns its number, or 0xff if
	 * not found. With Arg3 = 0xff, returns the number of matching channels.
	 */
	Method (FIND, 4, Serialized)
	{
		If (Arg0 == 0) {
			Return (0xff)
		}
		If (Arg1 == 0) {
			Local2 = NCT6687D_NUM_MON
		} Else {
			Local2 = NCT6687D_NUM_FANIN
		}

		Local1 = 0	// Matches so far
		Local0 = 0
		While (Local0 < Local2) {
			If (Arg1 == 0) {
				Local3 = MGRP (Arg0, Local0)
			} Else {
				Local3 = FGRP (Arg0, Local0)
			}
			If (Local3 == Arg2) {
				If (Local1 == Arg3) {
					Return (Local0)
				}
				Local1++
			}
			Local0++
		}

		If (Arg3 == 0xff) {
			Return (Local1)
		}
		Return (0xff)
	}

	// Get platform features
	Method (GFTR, 0, Serialized)
	{
		Return (
			(1 << DASHARO_FEATURE_TEMPERATURE) |
			(1 << DASHARO_FEATURE_FAN_PWM) |
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
						Return (FIND (ECBA (), 0, Arg1, 0xff))
					}
					Default { Return (0) }
				}
			}
			Case (Package () { DASHARO_FEATURE_FAN_PWM, DASHARO_FEATURE_FAN_TACH })
			{
				Switch (ToInteger (Arg1))
				{
					Case (Package () { DASHARO_FAN_CPU, DASHARO_FAN_CHASSIS })
					{
						Return (FIND (ECBA (), 1, Arg1, 0xff))
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
		Local2 = ECBA ()
		Local0 = FIND (Local2, 0, Arg0, Arg1)
		If (Local0 == 0xff) {
			Return (0)
		}

		Local1 = RREG (Local2, SENSOR_READ_HI_REG (Local0))
		// Negative temperatures are reported as 0
		If (Local1 & 0x80) {
			Return (0)
		}
		Return (Local1)
	}

	// Get fan tach reading by cap and index
	Method (GFTH, 2, Serialized)
	{
		Local2 = ECBA ()
		Local0 = FIND (Local2, 1, Arg0, Arg1)
		If (Local0 == 0xff) {
			Return (0)
		}

		Local1 = RREG (Local2, FAN_RPM_READ_HI_REG (Local0 + 1)) << 8
		Local1 |= RREG (Local2, FAN_RPM_READ_LO_REG (Local0 + 1))
		Return (Local1)
	}

	// Get fan duty cycle by cap and index
	Method (GFDC, 2, Serialized)
	{
		Local2 = ECBA ()
		Local0 = FIND (Local2, 1, Arg0, Arg1)
		// Duty of fans 9 and up is not reported
		If (Local0 >= 8) {
			Return (0)
		}

		Return (RREG (Local2, FAN_DUTY_PAGE1_REG (Local0 + 1)))
	}
}
