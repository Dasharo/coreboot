/*
 * This file is part of the coreboot project.
 *
 * Based on the example of Mika Westerberg: https://lwn.net/Articles/612062/
 *
 * Copyright (C) 2015 Tobias Diedrich, Mika Westerberg
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
	Device (BTNS)
	{
		Name (_HID, "PRP0001")

		Name (_CRS, ResourceTemplate () {
			GpioInt (Edge, ActiveLow, Shared, PullUp, ,
				 "\\_SB.PCI0.GPIO") {
			#if CONFIG(BOARD_PCENGINES_APU5)
				9
			#else
				89
			#endif
			}
		})

		Name (_DSD, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {"compatible", "gpio-keys"},
				Package () {"autorepeat", 1}
			}
		})

		Device (BTN1)
		{
			Name (_HID, "PRP0001")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
					Package () {"linux,code", 257},
					Package () {"linux,input-type", 1},
					Package () {"debounce-interval", 100},
					Package () {"label", "switch1"},
					Package () {"interrupts", 7},
					Package () {"gpios", Package ()
							{^^BTNS, 0, 0, 1}}
				}
			})
		}
	}
}
