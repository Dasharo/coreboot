/* SPDX-License-Identifier: GPL-2.0-or-later */

Scope (\_SB.GPNC)
{
	Name (_AEI, ResourceTemplate ()
	{
		GpioInt (Edge, ActiveLow, ExclusiveAndWake, PullNone,,
			"\\_SB.GPNC") { BOARD_SCI_GPIO_INDEX }
	})

	Method (_E0F, 0, NotSerialized)  // _Exx: Edge-Triggered GPE
	{
	}
}
