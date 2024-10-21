/* SPDX-License-Identifier: GPL-2.0-only */

Device (OTG)
{
	Name (_HID, "80860F37")
	Name (_CID, "80860F37")
	Name (_UID, 1)
	Name (_DDN, "USB OTG Controller")

	Name (RBUF, ResourceTemplate()
	{
		Memory32Fixed (ReadWrite, 0, 0x00200000, BAR0)
		Memory32Fixed (ReadWrite, 0, 0x00001000, BAR1)
	})

	Method (_CRS)
	{
		/* Update BAR0 from NVS */
		CreateDwordField (^RBUF, ^BAR0._BAS, BAS0)
		BAS0 = \UOB0

		/* Update BAR1 from NVS */
		CreateDwordField (^RBUF, ^BAR1._BAS, BAS1)
		BAS1 = \UOB1

		Return (^RBUF)
	}

	Method (_STA)
	{
		If (\UOEN == 1) {
			Return (0xF)
		} Else {
			Return (0x0)
		}
	}
}
