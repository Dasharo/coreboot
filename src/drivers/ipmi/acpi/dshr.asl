/* SPDX-License-Identifier: GPL-2.0-only */

#include <ec/dasharo/ec/driver.h>

/*
 * Dasharo ACPI interface (DSHR0001) backed by BMC sensors over IPMI.
 *
 * The BMC is not accessed directly. Requests go through an IPMI operation
 * region (ACPI address space 7), so the OS IPMI driver performs them and
 * serializes them with its other users (Linux: acpi_ipmi on top of ipmi_si).
 * Until the OS installs the region handler (_REG), all readings are 0.
 *
 * Readings are converted with the factors from the Get Sensor Reading Factors
 * command, so no SDR data is hardcoded. Linear, unsigned analog sensors are
 * assumed, which is what temperature and fan sensors normally are.
 *
 * The board defines the IPMI sensor numbers of each channel group as comma
 * separated lists. Undefined lists are empty.
 * DSHR_IPMI_CPU_TEMPS		CPU package temperatures
 * DSHR_IPMI_BOARD_TEMPS	Board temperatures
 * DSHR_IPMI_CHASSIS_TEMPS	Chassis temperatures, e.g. air outlet
 * DSHR_IPMI_CPU_FANS		CPU fans
 * DSHR_IPMI_CHASSIS_FANS	Chassis fans
 */

#ifndef DSHR_IPMI_CPU_TEMPS
#define DSHR_IPMI_CPU_TEMPS
#endif
#ifndef DSHR_IPMI_BOARD_TEMPS
#define DSHR_IPMI_BOARD_TEMPS
#endif
#ifndef DSHR_IPMI_CHASSIS_TEMPS
#define DSHR_IPMI_CHASSIS_TEMPS
#endif
#ifndef DSHR_IPMI_CPU_FANS
#define DSHR_IPMI_CPU_FANS
#endif
#ifndef DSHR_IPMI_CHASSIS_FANS
#define DSHR_IPMI_CHASSIS_FANS
#endif

#define DSHR_IPMI_NO_READING	0xffffffff
#define DSHR_IPMI_SIGN64	0x8000000000000000

Device (DASH)
{
	Name (_HID, "DSHR0001")
	Name (_UID, 0)
	// Hide the device so that Windows does not warn about a missing driver.
	Name (_STA, 0xB)

	// Version of the ACPI interface
	Name (DSVR, 1)

	Name (CTMP, Package () { DSHR_IPMI_CPU_TEMPS })
	Name (BTMP, Package () { DSHR_IPMI_BOARD_TEMPS })
	Name (STMP, Package () { DSHR_IPMI_CHASSIS_TEMPS })
	Name (CFAN, Package () { DSHR_IPMI_CPU_FANS })
	Name (SFAN, Package () { DSHR_IPMI_CHASSIS_FANS })

	// Set when the OS has installed the IPMI operation region handler
	Name (AVBL, 0)
	Method (_REG, 2, NotSerialized)
	{
		If (Arg0 == 7) {
			AVBL = Arg1
		}
	}

	// Sensor/Event NetFn (0x04), the offset is the command
	OperationRegion (IPMR, IPMI, 0x0400, 0x100)
	Field (IPMR, BufferAcc, NoLock, Preserve)
	{
		AccessAs (BufferAcc, 0x01),
		Offset (0x23),
		GSRF, 8,	// Get Sensor Reading Factors
		Offset (0x2d),
		GSRD, 8,	// Get Sensor Reading
	}

	// Get the raw reading of sensor Arg0, or DSHR_IPMI_NO_READING
	Method (RRAW, 1, Serialized)
	{
		If (!AVBL) {
			Return (DSHR_IPMI_NO_READING)
		}

		Name (BUFF, Buffer (66) {})
		CreateByteField (BUFF, 0x00, STAT)
		CreateByteField (BUFF, 0x01, LENG)
		CreateByteField (BUFF, 0x02, CCOD)	// Request: sensor number
		CreateByteField (BUFF, 0x03, READ)
		CreateByteField (BUFF, 0x04, FLAG)

		LENG = 1
		CCOD = Arg0
		Store (Store (BUFF, GSRD), BUFF)

		If ((STAT != 0) || (LENG < 3) || (CCOD != 0)) {
			Return (DSHR_IPMI_NO_READING)
		}
		// Reading/state unavailable
		If (FLAG & 0x20) {
			Return (DSHR_IPMI_NO_READING)
		}
		Return (READ)
	}

	// 10 ^ Arg0
	Method (POWT, 1, NotSerialized)
	{
		Local0 = 1
		While (Arg0 > 0) {
			Local0 *= 10
			Arg0--
		}
		Return (Local0)
	}

	// Signed Arg0 * 10 ^ (signed Arg1), truncated towards 0
	Method (SCAL, 2, NotSerialized)
	{
		Local0 = 0
		If (Arg0 & DSHR_IPMI_SIGN64) {
			Local0 = 1
			Arg0 = 0 - Arg0
		}
		If (Arg1 & DSHR_IPMI_SIGN64) {
			Arg0 /= POWT (0 - Arg1)
		} Else {
			Arg0 *= POWT (Arg1)
		}
		If (Local0) {
			Arg0 = 0 - Arg0
		}
		Return (Arg0)
	}

	// Convert raw reading Arg1 of sensor Arg0 to units, negative values give 0
	Method (CONV, 2, Serialized)
	{
		Name (BUFF, Buffer (66) {})
		CreateByteField (BUFF, 0x00, STAT)
		CreateByteField (BUFF, 0x01, LENG)
		CreateByteField (BUFF, 0x02, CCOD)	// Request: sensor number
		CreateByteField (BUFF, 0x03, NEXT)	// Request: reading
		CreateByteField (BUFF, 0x04, MLSB)
		CreateByteField (BUFF, 0x05, MMSB)	// [7:6] M MSBs
		CreateByteField (BUFF, 0x06, BLSB)
		CreateByteField (BUFF, 0x07, BMSB)	// [7:6] B MSBs
		CreateByteField (BUFF, 0x09, EXPS)	// [7:4] R exp, [3:0] B exp

		LENG = 2
		CCOD = Arg0
		NEXT = Arg1
		Store (Store (BUFF, GSRF), BUFF)

		If ((STAT != 0) || (LENG < 8) || (CCOD != 0)) {
			Return (0)
		}

		// 10-bit two's complement M and B, 4-bit two's complement exponents
		Local0 = MLSB | ((MMSB & 0xc0) << 2)
		If (Local0 & 0x200) {
			Local0 |= 0xfffffffffffffc00
		}
		Local1 = BLSB | ((BMSB & 0xc0) << 2)
		If (Local1 & 0x200) {
			Local1 |= 0xfffffffffffffc00
		}
		Local2 = EXPS >> 4
		If (Local2 & 0x08) {
			Local2 |= 0xfffffffffffffff0
		}
		Local3 = EXPS & 0x0f
		If (Local3 & 0x08) {
			Local3 |= 0xfffffffffffffff0
		}

		// y = (M * x + B * 10^Bexp) * 10^Rexp
		Local4 = SCAL (Local0 * Arg1, Local2) + SCAL (Local1, Local2 + Local3)
		If (Local4 & DSHR_IPMI_SIGN64) {
			Return (0)
		}
		Return (Local4)
	}

	// Read sensor Arg0 in units, 0 if not available
	Method (RSNS, 1, Serialized)
	{
		Local0 = RRAW (Arg0)
		If (Local0 == DSHR_IPMI_NO_READING) {
			Return (0)
		}
		Return (CONV (Arg0, Local0))
	}

	// Read entry Arg1 of sensor list Arg0
	Method (RLST, 2, Serialized)
	{
		If (Arg1 >= SizeOf (Arg0)) {
			Return (0)
		}
		Return (RSNS (DerefOf (Arg0[Arg1])))
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
					Case (DASHARO_TEMPERATURE_CPU_PACKAGE) { Return (SizeOf (CTMP)) }
					Case (DASHARO_TEMPERATURE_BOARD) { Return (SizeOf (BTMP)) }
					Case (DASHARO_TEMPERATURE_CHASSIS) { Return (SizeOf (STMP)) }
					Default { Return (0) }
				}
			}
			Case (DASHARO_FEATURE_FAN_TACH)
			{
				Switch (ToInteger (Arg1))
				{
					Case (DASHARO_FAN_CPU) { Return (SizeOf (CFAN)) }
					Case (DASHARO_FAN_CHASSIS) { Return (SizeOf (SFAN)) }
					Default { Return (0) }
				}
			}
			Default { Return (0) }
		}
	}

	// Get temperature by capability and index
	Method (GTMP, 2, Serialized)
	{
		Switch (ToInteger (Arg0))
		{
			Case (DASHARO_TEMPERATURE_CPU_PACKAGE) { Return (RLST (CTMP, Arg1)) }
			Case (DASHARO_TEMPERATURE_BOARD) { Return (RLST (BTMP, Arg1)) }
			Case (DASHARO_TEMPERATURE_CHASSIS) { Return (RLST (STMP, Arg1)) }
			Default { Return (0) }
		}
	}

	// Get fan tach reading by cap and index
	Method (GFTH, 2, Serialized)
	{
		Switch (ToInteger (Arg0))
		{
			Case (DASHARO_FAN_CPU) { Return (RLST (CFAN, Arg1)) }
			Case (DASHARO_FAN_CHASSIS) { Return (RLST (SFAN, Arg1)) }
			Default { Return (0) }
		}
	}
}
