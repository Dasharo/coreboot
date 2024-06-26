/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/hpet.h>

/* Intel LPC Bus Device  - 0:1f.0 */

Device (LPCB)
{
	Name(_ADR, 0x001f0000)

	#include "irqlinks.asl"

	#include "acpi/ec.asl"

	Device (DMAC)		/* DMA Controller */
	{
		Name(_HID, EISAID("PNP0200"))
		Name(_CRS, ResourceTemplate()
		{
			IO (Decode16, 0x00, 0x00, 0x01, 0x20)
			IO (Decode16, 0x81, 0x81, 0x01, 0x11)
			IO (Decode16, 0x93, 0x93, 0x01, 0x0d)
			IO (Decode16, 0xc0, 0xc0, 0x01, 0x20)
			DMA (Compatibility, NotBusMaster, Transfer8_16) { 4 }
		})
	}

	Device (FWH)		/* Firmware Hub */
	{
		Name (_HID, EISAID("INT0800"))
		Name (_CRS, ResourceTemplate()
		{
			Memory32Fixed(ReadOnly, 0xff000000, 0x01000000)
		})
	}

	Device (HPET)
	{
		Name (_HID, EISAID("PNP0103"))
		Name (_CID, 0x010CD041)

		Method (_STA, 0)	/* Device Status */
		{
			Return (0xf)	/* Enable and show device */
		}

		Name(_CRS, ResourceTemplate()
		{
			Memory32Fixed(ReadOnly, HPET_BASE_ADDRESS, 0x400)
		})
	}

	Device(PIC)	/* 8259 Interrupt Controller */
	{
		Name(_HID,EISAID("PNP0000"))
		Name(_CRS, ResourceTemplate()
		{
			IO (Decode16, 0x20, 0x20, 0x01, 0x02)
			IO (Decode16, 0x24, 0x24, 0x01, 0x02)
			IO (Decode16, 0x28, 0x28, 0x01, 0x02)
			IO (Decode16, 0x2c, 0x2c, 0x01, 0x02)
			IO (Decode16, 0x30, 0x30, 0x01, 0x02)
			IO (Decode16, 0x34, 0x34, 0x01, 0x02)
			IO (Decode16, 0x38, 0x38, 0x01, 0x02)
			IO (Decode16, 0x3c, 0x3c, 0x01, 0x02)
			IO (Decode16, 0xa0, 0xa0, 0x01, 0x02)
			IO (Decode16, 0xa4, 0xa4, 0x01, 0x02)
			IO (Decode16, 0xa8, 0xa8, 0x01, 0x02)
			IO (Decode16, 0xac, 0xac, 0x01, 0x02)
			IO (Decode16, 0xb0, 0xb0, 0x01, 0x02)
			IO (Decode16, 0xb4, 0xb4, 0x01, 0x02)
			IO (Decode16, 0xb8, 0xb8, 0x01, 0x02)
			IO (Decode16, 0xbc, 0xbc, 0x01, 0x02)
			IO (Decode16, 0x4d0, 0x4d0, 0x01, 0x02)
			IRQNoFlags () { 2 }
		})
	}

	Device(LDRC)	/* LPC device: Resource consumption */
	{
		Name (_HID, EISAID("PNP0C02"))
		Name (_UID, 2)

		Name (RBUF, ResourceTemplate()
		{
			IO (Decode16, 0x61, 0x61, 0x1, 0x01) /* NMI Status */
			IO (Decode16, 0x63, 0x63, 0x1, 0x01) /* CPU Reserved */
			IO (Decode16, 0x65, 0x65, 0x1, 0x01) /* CPU Reserved */
			IO (Decode16, 0x67, 0x67, 0x1, 0x01) /* CPU Reserved */
			IO (Decode16, 0x80, 0x80, 0x1, 0x01) /* Port 80 Post */
			IO (Decode16, 0x92, 0x92, 0x1, 0x01) /* CPU Reserved */
			IO (Decode16, 0xb2, 0xb2, 0x1, 0x02) /* SWSMI */
		})

		Method (_CRS, 0, NotSerialized)
		{
			Return (RBUF)
		}
	}

	Device (RTC)	/* Real Time Clock */
	{
		Name (_HID, EISAID("PNP0B00"))
		Name (_CRS, ResourceTemplate()
		{
			IO (Decode16, 0x70, 0x70, 1, 8)
		})
	}

	Device (TIMR)	/* Intel 8254 timer */
	{
		Name(_HID, EISAID("PNP0100"))
		Name(_CRS, ResourceTemplate()
		{
			IO (Decode16, 0x40, 0x40, 0x01, 0x04)
			IO (Decode16, 0x50, 0x50, 0x10, 0x04)
			IRQNoFlags() {0}
		})
	}

#if CONFIG(ENABLE_BUILTIN_COM1)
	Device (COM1) {
		Name (_HID, EISAID ("PNP0501"))
		Name (_UID, 1)

		Method (_STA, 0, NotSerialized) {
			Return (0x0F)
		}

		Name (_CRS, ResourceTemplate ()
		{
			IO (Decode16, 0x03F8, 0x3F8, 0x08, 0x08)
			IRQNoFlags () {4}
		})
	}
#endif

	/* Include mainboard's superio.asl file. */
	#include "acpi/superio.asl"
}
