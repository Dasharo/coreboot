/* SPDX-License-Identifier: GPL-2.0-only */

#define EC_GPE_SCI 0x6E
#define EC_GPE_SWI 0x6B
#define CPU_CRIT_TEMP 100
#define GPU_CRIT_TEMP 105
#include <ec/system76/ec/acpi/ec.asl>

Scope (\_SB) {
	#include "sleep.asl"
	#include "touchpad.asl"
	Scope (PCI0) {
		#include "backlight.asl"
	}
}
