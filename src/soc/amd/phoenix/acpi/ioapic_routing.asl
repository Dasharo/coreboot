/* SPDX-License-Identifier: GPL-2.0-only */

Scope (PCI0) {
	OperationRegion (NAPC, PCI_Config, 0xB8, 0x08)
	Field (NAPC, DWordAcc, NoLock, Preserve)
	{
		NAPX,   32,
		NAPD,   32
	}
}

/*
 * Clears IoapicSbFeatureEn on GNB IOAPIC to switch routing to IOAPIC.
 */
Mutex (NAPM, 0x00)
Method (NAPE, 0, NotSerialized)
{
	If (PICM == 0)
	{
		Return
	}

	\_SB.DSPI()

	Acquire (NAPM, 0xFFFF)
	\_SB.PCI0.NAPX = 0x14300000
	Local0 = \_SB.PCI0.NAPD
	Local0 &= 0xFFFFFFEF
	\_SB.PCI0.NAPD = Local0
	Release (NAPM)
}
