/* SPDX-License-Identifier: GPL-2.0-or-later */

Scope (\_SB.PCI0) {
	/* This device triggers automatic drivers and MSI utilities installation on Windows */
	Device (MSIV) {
		Name (_HID, "MBAD0002")
		Name (_UID, 1)
		Method (_STA, 0, NotSerialized){
			Return (1)
		}
	}
}
