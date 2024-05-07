/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2019 PC Engines Gmbh
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

Scope (\_SB.PCI0.LIBR) {

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

		Name (_PRS, ResourceTemplate ()
		{
			StartDependentFn (0, 0) {
				IO (Decode16, 0x03F8, 0x3F8, 0x08, 0x08)
				IRQNoFlags () {4}
			}
			EndDependentFn ()
		})
	}

	Device (COM2) {
		Name (_HID, EISAID ("PNP0501"))
		Name (_UID, 2)

		Method (_STA, 0, NotSerialized) {
			Return (0x0F)
		}

		Name (_CRS, ResourceTemplate ()
		{
			IO (Decode16, 0x02F8, 0x2F8, 0x08, 0x08)
			IRQNoFlags () {3}
		})

		Name (_PRS, ResourceTemplate ()
		{
			StartDependentFn (0, 0) {
				IO (Decode16, 0x02F8, 0x2F8, 0x08, 0x08)
				IRQNoFlags () {3}
			}
			EndDependentFn ()
		})
	}
}