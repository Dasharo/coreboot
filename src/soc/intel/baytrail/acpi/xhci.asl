/* SPDX-License-Identifier: GPL-2.0-only */

Device (XHCI)
{
	Name (_ADR, 0x00140000)
	Name (_PRW, Package () { 0x0d, 3 })
	Name (_S3D, 3) /* Highest D state in S3 state */

	Method(_PSW,1)
	{
		If (PMES & PMEE) {
			PMEE = 0
			PMES = 1
		}
	}

	OperationRegion (PMEB, PCI_Config, 0x74, 0x04)
	Field (PMEB, WordAcc, NoLock, Preserve)
	{
		,	8,
		PMEE,	1,
		,	6,
		PMES,	1
	}

	Device (RHUB)
	{
		Name (_ADR, 0x00000000)

		Name (PCKG, Package (0x01) {
			Buffer (0x10) {}
		})

		// GPLD: Generate Port Location Data (PLD)
		Method (GPLD, 1, Serialized) {

			// REV: Revision 0x02 for ACPI 5.0
			CreateField (DerefOf (PCKG[0]), 0, 0x07, REV)
			REV = 0x02

			// VISI: Port visibility to user per port
			CreateField (DerefOf (PCKG[0]), 0x40, 1, VISI)
			VISI = Arg0
			Return (PCKG)
		}

		Device (PRT1) { Name (_ADR, 1) }
		Device (PRT2) { Name (_ADR, 2) }
		Device (PRT3) { Name (_ADR, 3) }
		Device (PRT4) { Name (_ADR, 4) }
		Device (SSP1) { Name (_ADR, 7) }
	}
}
