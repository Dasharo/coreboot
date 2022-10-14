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

Scope (\)
{
	OperationRegion (PMIO, SystemIO, 0x0400, 0x24)
	Field (PMIO, ByteAcc, NoLock, WriteAsZeros)
	{
		Offset (0x20),
			,   5,
		SCIS,   1,
	}
}

Scope (\_GPE)
{
	Method (_L05)
	{
		If (\_SB.PCI0.GFX0.GSSE)
		{
			SCIS = One
		}
	}
}
