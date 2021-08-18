/* Initially copied from System76: src/mainboard/system76/cml-u/acpi */
/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

// GPP_E2 SCI
Method (_L02, 0, Serialized) {
	Debug = Concatenate("GPE_L02: ", ToHexString(\_SB.PCI0.LPCB.EC0.WFNO))
	If (\_SB.PCI0.LPCB.EC0.ECOK) {
		If (\_SB.PCI0.LPCB.EC0.WFNO == One) {
			Notify(\_SB.LID0, 0x80)
		}
	}
}
