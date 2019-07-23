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

Scope (\_SB.PCI0.SBUS)
{
	Device (LEDS)
	{
		Name (_HID, "AMD0030")

		Name (_CRS, ResourceTemplate () {
			/* LEDS GPIO*/
			GpioIo (Exclusive, PullNone, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.SBUS.GPIO", 0, ResourceConsumer) { 0x110, 0x114, 0x118 }
		})

		Name (_DSD, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {"compatible", Package () {"gpio-leds"}},
			}
		})

		Device (LED1)
		{
			Name (_HID, "AMD0030")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER ":green:led1"},
					Package () {"gpios", Package () {^^LEDS, 0, 0, 1 /* low-active */}},
					Package () {"default-state", "keep"},
				} }) }

		Device (LED2)
		{
			Name (_HID, "AMD0030")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER ":green:led2"},
					Package () {"gpios", Package () {^^LEDS, 0, 1, 1 /* low-active */}},
					Package () {"default-state", "keep"},
				}
			})
		}

		Device (LED3)
		{
			Name (_HID, "AMD0030")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"green:led3"},
					Package () {"gpios", Package () {^^LEDS, 0, 2, 1 /* low-active */}},
					Package () {"default-state", "keep"},
				}
			})
		}
	}


	Device (PINS)
	{
		Name (_HID, "AMD0030")

		Name (_CRS, ResourceTemplate () {
			/* PEx_RST */
			GpioIo (Exclusive, PullNone, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.SBUS.GPIO", 0, ResourceConsumer) { 0x108, 0x10C }
			/* PEx_WDIS */
			GpioIo (Exclusive, PullUp, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.SBUS.GPIO", 0, ResourceConsumer) { 0x11C, 0x120 }
		#if CONFIG(BOARD_PCENGINES_APU5)
			/* GENINT1/G22 */
			GpioIo (Exclusive, PullNone, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.SBUS.GPIO", 0, ResourceConsumer) { 0x164 }
		#endif
		#if !CONFIG(BOARD_PCENGINES_APU2)
			/* GENINT2/G33 */
			GpioIo (Exclusive, PullNone, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.SBUS.GPIO", 0, ResourceConsumer) { 0x168 }
		#endif
		})

		Name (_DSD, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {"compatible", Package () {"gpio-leds"}},
			}
		})

		Device (GP51)
		{
			Name (_HID, "AMD0030")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
				#if CONFIG(BOARD_PCENGINES_APU5)
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"SIM1:reset"},
				#elif CONFIG(BOARD_PCENGINES_APU2)
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"mPCIe1:reset"},
				#else
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"mPCIe3:reset"},
				#endif
					Package () {"gpios", Package () {^^LEDS, 0, 0, 1 /* low-active */}},
					Package () {"default-state", "off"},
				}
			})
		}

		Device (GP55)
		{
			Name (_HID, "AMD0030")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
				#if CONFIG(BOARD_PCENGINES_APU5)
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"SIM2:reset"},
				#else
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"mPCIe1:reset"},
				#endif
					Package () {"gpios", Package () {^^LEDS, 0, 1, 1 /* low-active */}},
					Package () {"default-state", "off"},
				}
			})
		}

		Device (GP64)
		{
			Name (_HID, "AMD0030")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
				#if CONFIG(BOARD_PCENGINES_APU5)
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"SIM3:reset"},
				#else
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"mPCIe2:wlan_disable"},
				#endif
					Package () {"gpios", Package () {^^LEDS, 1, 0, 1 /* low-active */}},
					Package () {"default-state", "off"},
				}
			})
		}

		Device (GP68)
		{
			Name (_HID, "AMD0030")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
				#if CONFIG(BOARD_PCENGINES_APU5)
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"SIM1:swap"},
				#else
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER
					"mPCIe2:wlan_disable"},
				#endif
					Package () {"gpios", Package () {^^LEDS, 1, 1, 1 /* low-active */}},
					Package () {"default-state", "off"},
				}
			})
		}

	#if CONFIG(BOARD_PCENGINES_APU5)
		Device (GP32)
		{
			Name (_HID, "AMD0030")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER "SIM2:swap"},
					Package () {"gpios", Package () {^^LEDS, 2, 0, 1 /* low-active */}},
					Package () {"default-state", "off"},
				}
			})
		}
	#endif

	#if !CONFIG(BOARD_PCENGINES_APU2)
		Device (GP33)
		{
			Name (_HID, "AMD0030")
			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
				#if CONFIG(BOARD_PCENGINES_APU5)
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER "SIM3:swap"},
				#else
					Package () {"label", CONFIG_MAINBOARD_PART_NUMBER "SIM:swap"},
				#endif
					Package () {"gpios", Package () {^^LEDS, 3, 0, 1 /* low-active */}},
					Package () {"default-state", "off"},
				}
			})
		}
	#endif



	}
}
