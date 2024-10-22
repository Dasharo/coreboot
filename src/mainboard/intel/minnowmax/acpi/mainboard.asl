/* SPDX-License-Identifier: GPL-2.0-only */

Scope (\_SB.PCI0.LPEA)
{
	Name (GBUF, ResourceTemplate ()
	{
	})
	Method (_DIS, 0x0, NotSerialized)
	{
		//Add a dummy disable function
	}
}

Scope (\_SB) {
	Device (PWRB)
	{
		Name(_HID, EisaId("PNP0C0C"))
	}
}

Scope(\_GPE)
{
	/* PMC_WAKE_PCIE0 connected to LAN on RP3 */
	Method (_L03)
	{
		PWS0 = 1
		Notify (\_SB.PCI0.RP03, 0x02)
	}
}
