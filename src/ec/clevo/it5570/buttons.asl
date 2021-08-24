/* Initially copied from System76: src/mainboard/system76/cml-u/acpi */
/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

Device (PWRB)
{
	Name (_HID, EisaId ("PNP0C0C"))
	Name (_PRW, Package () { EC_GPE_PWRB, 3 })
}

Device (SLPB)
{
	Name (_HID, EisaId ("PNP0C0E"))
	Name (_PRW, Package () { EC_GPE_SLPB, 3 })
}
