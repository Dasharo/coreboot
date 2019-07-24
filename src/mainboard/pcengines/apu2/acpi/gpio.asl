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

Scope (\_SB.PCI0.SBUS)
{
	Device(GPIO) {
		Name (_HID, "AMD0030")
		Name (_CID, "AMD0030")
		Name (_UID, 0)
		Name (_DDN, "GPIO Controller")

		Method (_CRS, 0x0, Serialized) {
			Name (RBUF, ResourceTemplate () {
				Interrupt(ResourceConsumer, Level, ActiveLow, Shared, , , ) {11}
				Memory32Fixed(ReadWrite, 0xFED81500, 0x300)
			})

			Return (RBUF)
		}

		Method (_STA, 0x0, NotSerialized) {
				Return (0x0F)
		}

		Name (_DSD, Package () {
			ToUUID("dbb8e3e6-5886-4ba6-8795-1319f52a966b"),
			Package () {
				Package () {"gpio66", "GP66"},
				Package () {"gpio67", "GP67"},
				Package () {"gpio71", "GP71"},
				Package () {"gpio72", "GP72"},
			#if CONFIG(BOARD_PCENGINES_APU5)
				Package () {"gpio89", "GP89"},
			#endif
			#if !CONFIG(BOARD_PCENGINES_APU2)
				Package () {"gpio90", "GP90"},
			#endif
			}
		})

		Name (GP66, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {"gpios", Package () {66, 1}},
				Package () {"output-high", 1},
			#if CONFIG(BOARD_PCENGINES_APU5)
				Package () {"line-name", "SIM1-reset"},
			#elif CONFIG(BOARD_PCENGINES_APU2)
				Package () {"line-name", "mPCIe1-reset"},
			#else
				Package () {"line-name", "mPCIe3-reset"},
			#endif
			}
		})

		Name (GP67, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {"gpios", Package () {67, 1}},
				Package () {"output-high", 1},
			#if CONFIG(BOARD_PCENGINES_APU5)
				Package () {"line-name", "SIM2-reset"},
			#else
				Package () {"line-name", "mPCIe2-reset"},
			#endif
			}
		})

		Name (GP71, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {"gpios", Package () {71, 1}},
				Package () {"output-high", 1},
			#if CONFIG(BOARD_PCENGINES_APU5)
				Package () {"line-name", "SIM3-reset"},
			#elif CONFIG(BOARD_PCENGINES_APU2)
				Package () {"line-name", "mPCIe1-wlan-disable"},
			#else
				Package () {"line-name", "mPCIe3-wlan-disable"},
			#endif
			}
		})

		Name (GP72, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {"gpios", Package () {72, 1}},
				Package () {"output-high", 1},
			#if CONFIG(BOARD_PCENGINES_APU5)
				Package () {"line-name", "SIM1-swap"},
			#else
				Package () {"line-name", "mPCIe2-wlan-disable"},
			#endif
			}
		})
	#if CONFIG(BOARD_PCENGINES_APU5)
		Name (GP89, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {"gpios", Package () {89, 1}},
				Package () {"output-high", 1},
				Package () {"line-name", "SIM2-swap"},
			}
		})
	#endif
	#if !CONFIG(BOARD_PCENGINES_APU2)
		Name (GP90, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {"gpios", Package () {90, 1}},
				Package () {"output-high", 1},
			#if CONFIG(BOARD_PCENGINES_APU5)
				Package () {"line-name", "SIM3-swap"},
			#else
				Package () {"line-name", "SIM-swap"},
			#endif
			}
		})
	#endif
	}
}
