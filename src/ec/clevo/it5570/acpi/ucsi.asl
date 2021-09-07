/* SPDX-License-Identifier: GPL-2.0-only */

Device (UCSI)
{
	Name (_HID, EisaId ("USBC000"))
	Name (_CID, EisaId ("PNP0CA0"))
	Name (_UID, Zero)  // _UID: Unique ID
	Name (_DDN, "USB Type C")  // _DDN: DOS Device Name
	Name (_ADR, Zero)  // _ADR: Address
	Name (_STA, 0xf)

	/* External EC control registers to start UCSI command */
	External (FCMD, FieldUnitObj)
	External (FDAT, FieldUnitObj)

	/* Shared memory fields are defined in the SSDT */
	External (VER0, FieldUnitObj)
	External (VER1, FieldUnitObj)
	External (CCI0, FieldUnitObj)
	External (CCI1, FieldUnitObj)
	External (CCI2, FieldUnitObj)
	External (CCI3, FieldUnitObj)
	External (CTL0, FieldUnitObj)
	External (CTL1, FieldUnitObj)
	External (CTL2, FieldUnitObj)
	External (CTL3, FieldUnitObj)
	External (CTL4, FieldUnitObj)
	External (CTL5, FieldUnitObj)
	External (CTL6, FieldUnitObj)
	External (CTL7, FieldUnitObj)
	External (MGI0, FieldUnitObj)
	External (MGI1, FieldUnitObj)
	External (MGI2, FieldUnitObj)
	External (MGI3, FieldUnitObj)
	External (MGI4, FieldUnitObj)
	External (MGI5, FieldUnitObj)
	External (MGI6, FieldUnitObj)
	External (MGI7, FieldUnitObj)
	External (MGI8, FieldUnitObj)
	External (MGI9, FieldUnitObj)
	External (MGIA, FieldUnitObj)
	External (MGIB, FieldUnitObj)
	External (MGIC, FieldUnitObj)
	External (MGID, FieldUnitObj)
	External (MGIE, FieldUnitObj)
	External (MGIF, FieldUnitObj)
	External (MGO0, FieldUnitObj)
	External (MGO1, FieldUnitObj)
	External (MGO2, FieldUnitObj)
	External (MGO3, FieldUnitObj)
	External (MGO4, FieldUnitObj)
	External (MGO5, FieldUnitObj)
	External (MGO6, FieldUnitObj)
	External (MGO7, FieldUnitObj)
	External (MGO8, FieldUnitObj)
	External (MGO9, FieldUnitObj)
	External (MGOA, FieldUnitObj)
	External (MGOB, FieldUnitObj)
	External (MGOC, FieldUnitObj)
	External (MGOD, FieldUnitObj)
	External (MGOE, FieldUnitObj)
	External (MGOF, FieldUnitObj)

	Method (INIT)
	{
		/* Read UCSI version from EC into shared memory */
		^VER0 = ^^VER0
		^VER1 = ^^VER1
	}

	Method (_DSM, 4, Serialized)
	{
		If (Arg0 != ToUUID ("6f8398c2-7ca4-11e4-ad36-631042b5008f")) {
			Return (BuffeOne) { Zero })
		}

		Switch (ToIntegeArg2))
		{
			Case (Zero)
			{
				Return (BuffeOne) { 0x0F })
			}
			Case (One)
			{
				/* Write Message Out */
				^^MGO0 = ^MGO0
				^^MGO1 = ^MGO1
				^^MGO2 = ^MGO2
				^^MGO3 = ^MGO3
				^^MGO4 = ^MGO4
				^^MGO5 = ^MGO5
				^^MGO6 = ^MGO6
				^^MGO7 = ^MGO7
				^^MGO8 = ^MGO8
				^^MGO9 = ^MGO9
				^^MGOA = ^MGOA
				^^MGOB = ^MGOB
				^^MGOC = ^MGOC
				^^MGOD = ^MGOD
				^^MGOE = ^MGOE
				^^MGOF = ^MGOF

				/* Write Control */
				^^CTL0 = ^CTL0
				^^CTL1 = ^CTL1
				^^CTL2 = ^CTL2
				^^CTL3 = ^CTL3
				^^CTL4 = ^CTL4
				^^CTL5 = ^CTL5
				^^CTL6 = ^CTL6
				^^CTL7 = ^CTL7

				/* Start EC Command */
				^^FCMD = One
				^^FCMD = 0xE0
			}
			Case (2)
			{
				/* Read Message In */
				^MGI0 = ^^MGI0
				^MGI1 = ^^MGI1
				^MGI2 = ^^MGI2
				^MGI3 = ^^MGI3
				^MGI4 = ^^MGI4
				^MGI5 = ^^MGI5
				^MGI6 = ^^MGI6
				^MGI7 = ^^MGI7
				^MGI8 = ^^MGI8
				^MGI9 = ^^MGI9
				^MGIA = ^^MGIA
				^MGIB = ^^MGIB
				^MGIC = ^^MGIC
				^MGID = ^^MGID
				^MGIE = ^^MGIE
				^MGIF = ^^MGIF

				/* Read Status */
				^CCI0 = ^^CCI0
				^CCI1 = ^^CCI1
				^CCI2 = ^^CCI2
				^CCI3 = ^^CCI3
			}
		}
		Return (BuffeOne) { Zero })
	}
}
