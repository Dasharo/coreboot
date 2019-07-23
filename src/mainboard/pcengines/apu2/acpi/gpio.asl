/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

Scope (\_SB.PCI0)
{
	Device (GPIO) {
		Name (_HID, "AMD0030")
		Name (_CID, "AMD0030")
		Name (_UID, 0)
		Name (_DDN, "GPIO Controller")

		Name (_CRS, ResourceTemplate () {
			Interrupt(ResourceConsumer, Level, ActiveLow,
				  Shared, , , ) {7}
			Memory32Fixed(ReadWrite, 0xFED81500, 0x300)
		})

		Method (_STA, 0x0, NotSerialized) {
			Return (0x0F)
		}
	}
}
