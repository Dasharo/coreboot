/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/gpio_defs.h>

Scope (\_SB.PCI0.I2C4)
{
	Device (PD01)
	{
		Name (_HID, "INT3515")
		Name (_UID, One)
		Name (_DDN, "TPS65994 USB-C PD Controller")
		Name (_STA, 0x0F)
		Name (_CRS, ResourceTemplate ()
		{
			I2cSerialBusV2 (0x0023, ControllerInitiated, 0x00061A80,
					AddressingMode7Bit, "\\_SB.PCI0.I2C4",
					0x00, ResourceConsumer, , Exclusive, )
			I2cSerialBusV2 (0x0027, ControllerInitiated, 0x00061A80,
					AddressingMode7Bit, "\\_SB.PCI0.I2C4",
					0x00, ResourceConsumer, , Exclusive)
			Interrupt (ResourceConsumer, Level, ActiveLow)
			{
				GPP_F13_IRQ
			}
		})
	}
}
