/* SPDX-License-Identifier: GPL-2.0-only */

Scope (\) {
	/* PIC IRQ mapping registers, C00h-C01h */
	OperationRegion (PRQM, SystemIO, 0x00000C00, 0x00000002)
	Field (PRQM, ByteAcc, NoLock, Preserve) {
		PRQI, 0x00000008,
		PRQD, 0x00000008,	/* Offset: 1h */
	}
	IndexField (PRQI, PRQD, ByteAcc, NoLock, Preserve) {
		PINA, 0x00000008,	/* Index 0 */
		PINB, 0x00000008,	/* Index 1 */
		PINC, 0x00000008,	/* Index 2 */
		PIND, 0x00000008,	/* Index 3 */
		AINT, 0x00000008,	/* Index 4 */
		SINT, 0x00000008,	/* Index 5 */
		    , 0x00000008,	/* Index 6 */
		AAUD, 0x00000008,	/* Index 7 */
		AMOD, 0x00000008,	/* Index 8 */
		PINE, 0x00000008,	/* Index 9 */
		PINF, 0x00000008,	/* Index A */
		PING, 0x00000008,	/* Index B */
		PINH, 0x00000008,	/* Index C */
	}

	/* PCI Error control register */
	OperationRegion (PERC, SystemIO, 0x00000C14, 0x00000001)
	Field (PERC, ByteAcc, NoLock, Preserve) {
		SENS, 0x00000001,
		PENS, 0x00000001,
		SENE, 0x00000001,
		PENE, 0x00000001,
	}

	Scope (\_SB) {
		Method (CIRQ, 0x00, NotSerialized) {
			Store (0, PINA)
			Store (0, PINB)
			Store (0, PINC)
			Store (0, PIND)
			Store (0, PINE)
			Store (0, PINF)
			Store (0, PING)
			Store (0, PINH)
		}

		/* set "A", 8259 interrupts */
		Name (PRSA, ResourceTemplate () {
			IRQ (Level, ActiveLow, Exclusive) {4, 7, 10, 11, 12, 14, 15}
		})

		Method (CRSA, 1, Serialized) {
			Store (ResourceTemplate () {
				IRQ (Level, ActiveLow, Shared) {15}
			}, Local0)
			CreateWordField (Local0, 1, LIRQ)
			ShiftLeft (1, Arg0, LIRQ)
			Return (Local0)
		}

		Method (SRSA, 1, Serialized) {
			CreateWordField (Arg0, 1, LIRQ)
			FindSetRightBit (LIRQ, Local0)
			if (Local0) {
				Decrement (Local0)
			}
			Return (Local0)
		}

		Device (LNKA) {
			Name (_HID, EisaId ("PNP0C0F"))
			Name (_UID, 1)
			Method (_STA, 0) {
				If (PINA) {
					Return (0x0B) /* LNKA is invisible */
				} Else {
					Return (0x09) /* LNKA is disabled */
				}
			}
			Method (_DIS, 0) {
				Store (0, PINA)
			}
			Method (_PRS, 0) {
				Return (PRSA)
			}
			Method (_CRS, 0, Serialized) {
				Return (CRSA (PINA))
			}
			Method (_SRS, 1, Serialized) {
				Store (SRSA (Arg0), PINA)
			}
		}

		Device (LNKB) {
			Name (_HID, EisaId ("PNP0C0F"))
			Name (_UID, 2)
			Method (_STA, 0) {
				If (PINB) {
					Return (0x0B) /* LNKB is invisible */
				} Else {
					Return (0x09) /* LNKB is disabled */
				}
			}
			Method (_DIS, 0) {
				Store (0, PINB)
			}
			Method (_PRS, 0) {
				Return (PRSA)
			}
			Method (_CRS, 0, Serialized) {
				Return (CRSA (PINB))
			}
			Method (_SRS, 1, Serialized) {
				Store (SRSA (Arg0), PINB)
			}
		}

		Device (LNKC) {
			Name (_HID, EisaId ("PNP0C0F"))
			Name (_UID, 3)
			Method (_STA, 0) {
				If (PINC) {
					Return (0x0B) /* LNKC is invisible */
				} Else {
					Return (0x09) /* LNKC is disabled */
				}
			}
			Method (_DIS, 0) {
				Store (0, PINC)
			}
			Method (_PRS, 0) {
				Return (PRSA)
			}
			Method (_CRS, 0, Serialized) {
				Return (CRSA (PINC))
			}
			Method (_SRS, 1, Serialized) {
				Store (SRSA (Arg0), PINC)
			}
		}

		Device (LNKD) {
			Name (_HID, EisaId ("PNP0C0F"))
			Name (_UID, 4)
			Method (_STA, 0) {
				If (PIND) {
					Return (0x0B) /* LNKD is invisible */
				} Else {
					Return (0x09) /* LNKD is disabled */
				}
			}
			Method (_DIS, 0) {
				Store (0, PIND)
			}
			Method (_PRS, 0) {
				Return (PRSA)
			}
			Method (_CRS, 0, Serialized) {
				Return (CRSA (PIND))
			}
			Method (_SRS, 1, Serialized) {
				Store (SRSA (Arg0), PIND)
			}
		}

		Device (LNKE) {
			Name (_HID, EisaId ("PNP0C0F"))
			Name (_UID, 5)
			Method (_STA, 0) {
				If (PINE) {
					Return (0x0B) /* LNKE is invisible */
				} Else {
					Return (0x09) /* LNKE is disabled */
				}
			}
			Method (_DIS, 0) {
				Store (0, PINE)
			}
			Method (_PRS, 0) {
				Return (PRSA)
			}
			Method (_CRS, 0, Serialized) {
				Return (CRSA (PINE))
			}
			Method (_SRS, 1, Serialized) {
				Store (SRSA (Arg0), PINE)
			}
		}

		Device (LNKF) {
			Name (_HID, EisaId ("PNP0C0F"))
			Name (_UID, 6)
			Method (_STA, 0) {
				If (PINF) {
					Return (0x0B) /* LNKF is invisible */
				} Else {
					Return (0x09) /* LNKF is disabled */
				}
			}
			Method (_DIS, 0) {
				Store (0, PINF)
			}
			Method (_PRS, 0) {
				Return (PRSA)
			}
			Method (_CRS, 0, Serialized) {
				Return (CRSA (PINF))
			}
			Method (_SRS, 1, Serialized) {
				Store (SRSA (Arg0), PINF)
			}
		}

		Device (LNKG) {
			Name (_HID, EisaId ("PNP0C0F"))
			Name (_UID, 7)
			Method (_STA, 0) {
				If (PING) {
					Return (0x0B) /* LNKG is invisible */
				} Else {
					Return (0x09) /* LNKG is disabled */
				}
			}
			Method (_DIS, 0) {
				Store (0, PING)
			}
			Method (_PRS, 0) {
				Return (PRSA)
			}
			Method (_CRS, 0, Serialized) {
				Return (CRSA (PING))
			}
			Method (_SRS, 1, Serialized) {
				Store (SRSA (Arg0), PING)
			}
		}

		Device (LNKH) {
			Name (_HID, EisaId ("PNP0C0F"))
			Name (_UID, 8)
			Method (_STA, 0) {
				If (PINH) {
					Return (0x0B) /* LNKH is invisible */
				} Else {
					Return (0x09) /* LNKH is disabled */
				}
			}
			Method (_DIS, 0) {
				Store (0, PINH)
			}
			Method (_PRS, 0) {
				Return (PRSA)
			}
			Method (_CRS, 0, Serialized) {
				Return (CRSA (PINH))
			}
			Method (_SRS, 1, Serialized) {
				Store (SRSA (Arg0), PINH)
			}
		}

	}	/* End Scope (_SB) */

}	/* End Scope (/) */
