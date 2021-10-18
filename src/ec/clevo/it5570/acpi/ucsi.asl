/* SPDX-License-Identifier: GPL-2.0-only */

Device (UCSI)
{
	Name (_HID, EisaId ("USBC000"))
	Name (_CID, EisaId ("PNP0CA0"))
	Name (_UID, One)
	Name (_DDN, "USB Type-C")
	Name (_STA, 0xf)

	/* Shared memory fields are defined in the SSDT */
	External (\_SB.PCI0.LPCB.EC0.UCSI.VER0, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.VER1, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CCI0, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CCI1, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CCI2, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CCI3, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CTL0, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CTL1, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CTL2, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CTL3, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CTL4, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CTL5, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CTL6, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.CTL7, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI0, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI1, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI2, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI3, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI4, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI5, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI6, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI7, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI8, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGI9, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGIA, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGIB, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGIC, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGID, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGIE, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGIF, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO0, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO1, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO2, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO3, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO4, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO5, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO6, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO7, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO8, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGO9, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGOA, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGOB, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGOC, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGOD, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGOE, FieldUnitObj)
	External (\_SB.PCI0.LPCB.EC0.UCSI.MGOF, FieldUnitObj)

	Method (INIT)
	{
		/* Read UCSI version from EC into shared memory */
		VER0 = \_SB.PCI0.LPCB.EC0.VER0
		VER1 = \_SB.PCI0.LPCB.EC0.VER1
	}

	Method (_DSM, 4, Serialized)
	{
		If (Arg0 != ToUUID ("6f8398c2-7ca4-11e4-ad36-631042b5008f")) {
			Return (Buffer (One) { Zero })
		}

		Switch (ToInteger (Arg2))
		{
			Case (Zero)
			{
				Return (Buffer (One) { 0x0F })
			}
			Case (One)
			{
				/* Write Message Out */
				\_SB.PCI0.LPCB.EC0.MGO0 = MGO0
				\_SB.PCI0.LPCB.EC0.MGO1 = MGO1
				\_SB.PCI0.LPCB.EC0.MGO2 = MGO2
				\_SB.PCI0.LPCB.EC0.MGO3 = MGO3
				\_SB.PCI0.LPCB.EC0.MGO4 = MGO4
				\_SB.PCI0.LPCB.EC0.MGO5 = MGO5
				\_SB.PCI0.LPCB.EC0.MGO6 = MGO6
				\_SB.PCI0.LPCB.EC0.MGO7 = MGO7
				\_SB.PCI0.LPCB.EC0.MGO8 = MGO8
				\_SB.PCI0.LPCB.EC0.MGO9 = MGO9
				\_SB.PCI0.LPCB.EC0.MGOA = MGOA
				\_SB.PCI0.LPCB.EC0.MGOB = MGOB
				\_SB.PCI0.LPCB.EC0.MGOC = MGOC
				\_SB.PCI0.LPCB.EC0.MGOD = MGOD
				\_SB.PCI0.LPCB.EC0.MGOE = MGOE
				\_SB.PCI0.LPCB.EC0.MGOF = MGOF

				/* Write Control */
				\_SB.PCI0.LPCB.EC0.CTL0 = CTL0
				\_SB.PCI0.LPCB.EC0.CTL1 = CTL1
				\_SB.PCI0.LPCB.EC0.CTL2 = CTL2
				\_SB.PCI0.LPCB.EC0.CTL3 = CTL3
				\_SB.PCI0.LPCB.EC0.CTL4 = CTL4
				\_SB.PCI0.LPCB.EC0.CTL5 = CTL5
				\_SB.PCI0.LPCB.EC0.CTL6 = CTL6
				\_SB.PCI0.LPCB.EC0.CTL7 = CTL7

				/* Start EC Command */
				\_SB.PCI0.LPCB.EC0.FDAT = One
				\_SB.PCI0.LPCB.EC0.FCMD = 0xE0
			}
			Case (2)
			{
				/* Read Message In */
				MGI0 = \_SB.PCI0.LPCB.EC0.MGI0
				MGI1 = \_SB.PCI0.LPCB.EC0.MGI1
				MGI2 = \_SB.PCI0.LPCB.EC0.MGI2
				MGI3 = \_SB.PCI0.LPCB.EC0.MGI3
				MGI4 = \_SB.PCI0.LPCB.EC0.MGI4
				MGI5 = \_SB.PCI0.LPCB.EC0.MGI5
				MGI6 = \_SB.PCI0.LPCB.EC0.MGI6
				MGI7 = \_SB.PCI0.LPCB.EC0.MGI7
				MGI8 = \_SB.PCI0.LPCB.EC0.MGI8
				MGI9 = \_SB.PCI0.LPCB.EC0.MGI9
				MGIA = \_SB.PCI0.LPCB.EC0.MGIA
				MGIB = \_SB.PCI0.LPCB.EC0.MGIB
				MGIC = \_SB.PCI0.LPCB.EC0.MGIC
				MGID = \_SB.PCI0.LPCB.EC0.MGID
				MGIE = \_SB.PCI0.LPCB.EC0.MGIE
				MGIF = \_SB.PCI0.LPCB.EC0.MGIF

				/* Read Status */
				CCI0 = \_SB.PCI0.LPCB.EC0.CCI0
				CCI1 = \_SB.PCI0.LPCB.EC0.CCI1
				CCI2 = \_SB.PCI0.LPCB.EC0.CCI2
				CCI3 = \_SB.PCI0.LPCB.EC0.CCI3
			}
		}
		Return (Buffer (One) { Zero })
	}
}
