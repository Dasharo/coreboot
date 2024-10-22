/* SPDX-License-Identifier: GPL-2.0-only */

Device(HDA)
{
	Name(_ADR,0x001b0000)

	OperationRegion (PMEB, PCI_Config, 0x74, 0x04)
	Field (PMEB, WordAcc, NoLock, Preserve)
	{
		,	8,
		PMEE,	1,
		,	6,
		PMES,	1
	}
}
