/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/iomap.h>
#include <soc/irq.h>

/* SouthCluster GPIO */
Device (GPSC)
{
	Name (_HID, "INT33FC")
	Name (_CID, "INT33FC")
	Name (_UID, 1)

	Name (RBUF, ResourceTemplate()
	{
		Memory32Fixed (ReadWrite, 0, 0x1000, RMEM)
		Interrupt (ResourceConsumer, Level, ActiveLow, Shared,,,)
		{
			GPIO_SC_IRQ
		}
	})

	Method (_CRS)
	{
		CreateDwordField (^RBUF, ^RMEM._BAS, RBAS)
		RBAS = IO_BASE_ADDRESS + IO_BASE_OFFSET_GPSCORE
		Return (^RBUF)
	}

	Method (_HRV, 0, NotSerialized) { Return (0x06) }

	Method (_STA)
	{
		Return (0xF)
	}
}

/* NorthCluster GPIO */
Device (GPNC)
{
	Name (_HID, "INT33FC")
	Name (_CID, "INT33FC")
	Name (_UID, 2)

	Name (RBUF, ResourceTemplate()
	{
		Memory32Fixed (ReadWrite, 0, 0x1000, RMEM)
		Interrupt (ResourceConsumer, Level, ActiveLow, Shared,,,)
		{
			GPIO_NC_IRQ
		}
	})

	Method (_CRS)
	{
		CreateDwordField (^RBUF, ^RMEM._BAS, RBAS)
		RBAS = IO_BASE_ADDRESS + IO_BASE_OFFSET_GPNCORE
		Return (^RBUF)
	}

	Method (_HRV, 0, NotSerialized) { Return (0x06) }

	Method (_STA)
	{
		Return (0xF)
	}
}

/* SUS GPIO */
Device (GPSS)
{
	Name (_HID, "INT33FC")
	Name (_CID, "INT33FC")
	Name (_UID, 3)

	Name (RBUF, ResourceTemplate()
	{
		Memory32Fixed (ReadWrite, 0, 0x1000, RMEM)
		Interrupt (ResourceConsumer, Level, ActiveLow, Shared,,,)
		{
			GPIO_SUS_IRQ
		}
	})

	Method (_CRS)
	{
		CreateDwordField (^RBUF, ^RMEM._BAS, RBAS)
		RBAS = IO_BASE_ADDRESS + IO_BASE_OFFSET_GPSSUS
		Return (^RBUF)
	}

	Method (_HRV, 0, NotSerialized) { Return (0x06) }

	Method (_STA)
	{
		Return (0xF)
	}
}

Device (GPED)
{
	Name (_HID, "INT0002")
	Name (_CID, "INT0002")
	Name (_DDN, "Virtual GPIO controller")
	Name (_UID, 1)

	Name (RBUF, ResourceTemplate ()
	{
		Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 9 }
	})

	Method (_CRS, 0, Serialized)
	{
		Return (RBUF)
	}

	Method (_STA, 0, NotSerialized)
	{
		Return (0)
	}

	Name (GBUF, ResourceTemplate ()
	{
		GpioInt (Edge, ActiveHigh, ExclusiveAndWake, PullDown, 0x0000,
			"\\_SB.GPED", 0x00, ResourceConsumer,,) { 2 }
	})

	Method (_AEI, 0, Serialized)
	{
		Return (GBUF)
	}

	Method (_E02, 0, NotSerialized)
	{
		If (PWBS)
		{
			PWBS = 1
		}

		If (PMEB)
		{
			PMEB = 1
		}

		If (\_SB.PCI0.SATA.PMES)
		{
			\_SB.PCI0.SATA.PMES = 1
			Notify (\_SB.PCI0.SATA, 0x02)
		}

		If (\_SB.PCI0.EM45.PMES && (\C0EN == 0))
		{
			\_SB.PCI0.EM45.PMES = 1
			Notify (\_SB.PCI0.EM45, 0x02)
		}

		If (\_SB.PCI0.HDA.PMES)
		{
			\_SB.PCI0.HDA.PMES = 1
			Notify (\_SB.PCI0.HDA, 0x02)
		}

		If (\_SB.PCI0.XHCI.PMES)
		{
			\_SB.PCI0.XHCI.PMES = 1
			Notify (\_SB.PCI0.XHCI, 0x02)
		}

		If (\_SB.PCI0.SEC0.PMES == 1)
		{
			\_SB.PCI0.SEC0.PMES |= 0
			Notify (\_SB.PCI0.SEC0, 0x02)
		}
	}
}
