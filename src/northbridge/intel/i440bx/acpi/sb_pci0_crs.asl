/* SPDX-License-Identifier: GPL-2.0-only */

/* i440bx Northbridge resources that sits on \_SB.PCI0 */
Device (NB)
{
	Name(_ADR, 0x00000000)
	OperationRegion(PCIC, PCI_Config, 0x00, 0x100)
	Field (PCIC, ByteAcc, NoLock, Preserve)
	{
		Offset (0x67),	// DRB7
		DRB7,	8,
		Offset (0x7A),	// PMCR
		PMCR,	8
	}
	Method(TOM1, 0) {
		/* Multiply by 8MB to get TOM */
		Return(DRB7 << 23)
	}
}

Method(_CRS, 0) {
	Name(TMP, ResourceTemplate() {
		WordBusNumber(ResourceProducer, MinFixed, MaxFixed, PosDecode,
			0x0000,             // Granularity
			0x0000,             // Range Minimum
			0x00FF,             // Range Maximum
			0x0000,             // Translation Offset
			0x0100,             // Length
			,,
		)
		IO(Decode16, 0x0CF8, 0x0CF8, 1, 8)

		WORDIO(ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
			0x0000,			/* address granularity */
			0x0000,			/* range minimum */
			0x0CF7,			/* range maximum */
			0x0000,			/* translation */
			0x0CF8			/* length */
		)

		WORDIO(ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
			0x0000,			/* address granularity */
			0x0D00,			/* range minimum */
			0xFFFF,			/* range maximum */
			0x0000,			/* translation */
			0xF300			/* length */
		)

		/* memory space for PCI BARs below 4GB */
		Memory32Fixed(ReadOnly, 0x00000000, 0x00000000, MMIO)
	})
	CreateDWordField(TMP, MMIO._BAS, MM1B)
	CreateDWordField(TMP, MMIO._LEN, MM1L)
	/*
	 * Declare memory between TOM1 and 4GB as available
	 * for PCI MMIO.
	 *
	 * Use ShiftLeft to avoid 64bit constant (for XP).
	 * This will work even if the OS does 32bit arithmetic, as
	 * 32bit (0x00000000 - TOM1) will wrap and give the same
	 * result as 64bit (0x100000000 - TOM1).
	 */
	MM1B = \_SB.PCI0.NB.TOM1
	Local0 = 0x10000000 << 4
	Local0 -= CONFIG_ROM_SIZE
	MM1L = Local0 - MM1B

	Return(TMP)
}
