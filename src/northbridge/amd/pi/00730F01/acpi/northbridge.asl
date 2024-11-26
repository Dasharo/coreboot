/* SPDX-License-Identifier: GPL-2.0-only */

/* Note: Only need HID on Primary Bus */
External (TOM1)
External (TOM2)
Name(_HID, EISAID("PNP0A08"))	/* PCI Express Root Bridge */
Name(_CID, EISAID("PNP0A03"))	/* PCI Root Bridge */

OperationRegion (NAPC, PCI_Config, 0xF8, 0x08)
Field (NAPC, DWordAcc, NoLock, Preserve)
{
	NAPX,   32,	/* Northbridge IOAPIC Index */
	NAPD,   32	/* Northbridge IOAPIC Data */
}

IndexField (NAPX, NAPD, DWordAcc, NoLock, Preserve)
{
	NAFC,   32	/* Northbridge IOAPIC Feature Control */
}

Mutex (NAPM, 0x00)
Method (NAPE, 1, NotSerialized)
{
	Acquire (NAPM, 0xFFFF)

	Local0 = NAFC
	Local0 &= 0xFFFFFFEF /* Clear IoapicSbFeatureEn for APIC mode */

	/* Set IoapicSbFeatureEn if OS chose to use PIC mode */
	If (Arg0 == 0) {
		Local0 |= (1 << 4)
	}

	NAFC = Local0

	Release (NAPM)
}

/* Describe the Northbridge devices */

Method(_BBN, 0, NotSerialized)	/* Bus number = 0 */
{
	Return(0)
}

Method(_STA, 0, NotSerialized)
{
	Return(0x0f)	/* Status is visible */
}

Method(_PRT,0, NotSerialized)
{
	If(PICM)
	{
		Return(APR0)	/* APIC mode */
	}
	Return (PR0)		/* PIC Mode */
}

Device(AMRT) {
	Name(_ADR, 0x00000000)
} /* end AMRT */

Device(IOMU) {
	Name(_ADR, 0x00000002)

	Method(_PRT,0, NotSerialized)
	{
		If(PICM)
		{
			Return(APR0)	/* APIC mode */
		}
		Return (PR0)		/* PIC Mode */
	}
} /* end AMRT */

/* Gpp 0 */
Device(PBR4) {
	Name(_ADR, 0x00020001)
	Name(_PRW, Package() {0x18, 4})
	Method(_PRT,0) {
		If(PICM) { Return(APS4) }	/* APIC mode */
		Return (PS4)			/* PIC Mode */
	} /* end _PRT */
} /* end PBR4 */

/* Gpp 1 */
Device(PBR5) {
	Name(_ADR, 0x00020002)
	Name(_PRW, Package() {0x18, 4})
	Method(_PRT,0) {
		If(PICM) { Return(APS5) }	/* APIC mode */
		Return (PS5)			/* PIC Mode */
	} /* end _PRT */
} /* end PBR5 */

/* Gpp 2 */
Device(PBR6) {
	Name(_ADR, 0x00020003)
	Name(_PRW, Package() {0x18, 4})
	Method(_PRT,0) {
		If(PICM) { Return(APS6) }	/* APIC mode */
		Return (PS6)			/* PIC Mode */
	} /* end _PRT */
} /* end PBR6 */

/* Gpp 3 */
Device(PBR7) {
	Name(_ADR, 0x00020004)
	Name(_PRW, Package() {0x18, 4})
	Method(_PRT,0) {
		If(PICM) { Return(APS7) }	/* APIC mode */
		Return (PS7)			/* PIC Mode */
	} /* end _PRT */
} /* end PBR7 */

/* Gpp 4 */
Device(PBR8) {
	Name(_ADR, 0x00020005)
	Name(_PRW, Package() {0x18, 4})
	Method(_PRT,0) {
		If(PICM) { Return(APS8) }	/* APIC mode */
		Return (PS8)			/* PIC Mode */
	} /* end _PRT */
} /* end PBR8 */

Device(K10M) {
	Name (_ADR, 0x00180003)
	#include <soc/amd/common/acpi/thermal_zone.asl>
}
