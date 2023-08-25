/* SPDX-License-Identifier: GPL-2.0-only */

Scope (\_SB.PCI0.LPCB) {

	/* OC Watchdog */
	Device(CWDT)
	{
#if CONFIG(SOC_INTEL_RAPTORLAKE)
	Name(_HID,"INTC109C")
#else
	Name(_HID,"INTC1099")
#endif
		Name(_CID,EISAID("PNP0C02"))

		Name (RBUF, ResourceTemplate () {
			IO(Decode16, 0, 0, 0x4, 0x4, OCWD)
		})

		Method (_CRS, 0x0, NotSerialized)
		{
			CreateWordField(^RBUF, ^OCWD._MIN, OMIN)
			CreateWordField(^RBUF, ^OCWD._MAX, OMAX)

			OMIN = ACPI_BASE_ADDRESS + 0x54
			OMAX = ACPI_BASE_ADDRESS + 0x54

			Return (RBUF)
		}
	}
}
