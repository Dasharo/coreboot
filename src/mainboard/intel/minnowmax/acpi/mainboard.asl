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
