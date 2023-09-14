/* SPDX-License-Identifier: GPL-2.0-only */

#define EC_GPE_SCI 0x6E
#define EC_GPE_SWI 0x6B
#define CPU_CRIT_TEMP 100
#define GPU_CRIT_TEMP 105
#include <ec/system76/ec/acpi/ec.asl>

Scope (\_SB) {
	#include "sleep.asl"
	Scope (PCI0) {
		#include "backlight.asl"
	}
}

Name(\_S0, Package(){0x0,0x0,0x0,0x0})
If (S0IX == 0) {
	Name(\_S3, Package(){0x5,0x0,0x0,0x0})
} Else {
	Name(\_S1, Package(){0x1,0x0,0x0,0x0})
}
#if !CONFIG(DISABLE_ACPI_HIBERNATE)
Name(\_S4, Package(){0x6,0x0,0x0,0x0})
#endif
Name(\_S5, Package(){0x7,0x0,0x0,0x0})
