/* SPDX-License-Identifier: GPL-2.0-or-later */

External(\TBTS, MethodObj)

Scope (_GPE) {
	/* Empty PCI_EXP_STS handler */
	Method (_L69, 0, Serialized) { }
}

Scope (\_SB.PCI0) {
	/* This device triggers automatic drivers and MSI utilities installation on Windows */
	Device (MSIV) {
		Name (_HID, "MBID0001")
		Name (_UID, One)
		Method (_STA, 0, NotSerialized){
			Return (One)
		}
	}
}

Scope (\_SB) {
	Method (MPTS, 1) {
		If (Arg0 == 3 || Arg0 == 4) {
			If (CondRefOf (\TBTS)) {
				\TBTS()
			}
		}
	}
}