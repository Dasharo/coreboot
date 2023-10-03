/* SPDX-License-Identifier: GPL-2.0-only */

/* Included in each PCIe Root Port device */

OperationRegion (RPCS, PCI_Config, 0x00, 0xFF)
Field (RPCS, AnyAcc, NoLock, Preserve)
{
	Offset (0x10),
	L0SE, 1,
	Offset (0x4c),	// Link Capabilities
	, 24,
	RPPN, 8,	// Root Port Number
	Offset (0x5A),
	, 3,
	PDCX, 1,
	, 2,
	PDSX, 1,
	Offset (0x60),	// RSTS - Root Status Register
	, 16,
	PSPX, 1,	// PME Status
	Offset (0xDB),	// MPC - Miscellaneous Port Configuration Register
	, 6,
	HPEX, 1,	// Hot Plug SCI Enable
	PMEX, 1,	// Power Management SCI Enable
}

Field (RPCS, AnyAcc, NoLock, WriteAsZeros)
{
	Offset (0xDF),
	, 6,
	HPCS, 1,	// Hot Plug SCI Status
	PMCS, 1,	// Power Management SCI Status
}

Method (HPME, 0, Serialized) {
	If (PMCS == 1) {
		Notify (PXSX, 0x2)
		PMCS = 1 // clear rootport's PME SCI status
		PSPX = 1 // consume one pending PME notification to prevent it from blocking the queue
	}
}

Method (HPEV, 0, Serialized) {
	Sleep (0x64)
	If (PDCX == 1)
	{
		PDCX = 1
		HPCS = 1
		If (PDSX == 0)
		{
			L0SE = 0
		}
		Return (1)
	}
	Else
	{
		HPCS = 1
		Return (0)
	}
}

Name (_HPP, Package (0x04) {
	0x08,
	0x40,
	1,
	0
})

Device(PXSX)
{
	Name (_ADR, 0x00000000)
	Name (_PRW, Package() { 9, 4 })
}
