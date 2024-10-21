/* SPDX-License-Identifier: GPL-2.0-only */

Device(SATA)
{
	Name(_ADR,0x00130000)

	OperationRegion (PMEB, PCI_Config, 0x74, 0x04)
	Field (PMEB, WordAcc, NoLock, Preserve)
	{
		,	8,
		PMEE,	1,
		,	6,
		PMES,	1
	}
}
