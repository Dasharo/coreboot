/* SPDX-License-Identifier: GPL-2.0-only */

Device(SEC0)
{
	Name(_ADR,0x001a0000)

	OperationRegion (PMEB, PCI_Config, 0x84, 0x04)
	Field (PMEB, WordAcc, NoLock, Preserve)
	{
		,	8,
		PMEE,	1,
		,	6,
		PMES,	1
	}
}
