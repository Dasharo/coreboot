/* SPDX-License-Identifier: GPL-2.0-only */

#define EC_GPE_SLPB 0x6B // PME_STS
#define EC_GPE_LID 0x6B // PME_STS
#define EC_GPE_SCI 0x6E // ESPI_SCI
#define EC_GPE_PWRB 0x43 // GPD9 Power Button
#define EC_GPE_RTC 0x72 // RTC Alarm Status
// echo '\_SB.SLPB._PRW' | sudo tee /proc/acpi/call; sudo cat /proc/acpi/call
// echo '\_SB.PC00.LPCB.EC._GPE' | sudo tee /proc/acpi/call; sudo cat /proc/acpi/call

Scope (\_SB) {
	#include "sleep.asl"

	#include <ec/clevo/it5570/acpi/ac.asl>
	#include <ec/clevo/it5570/acpi/battery.asl>
	#include <ec/clevo/it5570/acpi/buttons.asl>
	#include <ec/clevo/it5570/acpi/lid.asl>

	Scope (PCI0.LPCB) {
		#include <ec/clevo/it5570/acpi/ec.asl>
	}
}

Scope (_GPE) {
	#include <ec/clevo/it5570/acpi/gpe.asl>
}
