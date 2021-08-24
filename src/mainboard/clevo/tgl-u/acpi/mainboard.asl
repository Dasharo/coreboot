/* Initially copied from System76: src/mainboard/system76/cml-u/acpi */
/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#define EC_GPE_SWI 0x6B // echo '\_SB.SLPB._PRW' | sudo tee /proc/acpi/call; sudo cat /proc/acpi/call
			// correct for SLPB and LID0
			// meanwhile PWRB._PRW has 0x43 and AWAC._PRW has 0x72,
			// so this may not be entirely correct.
#define EC_GPE_SCI 0x6E // echo '\_SB.PC00.LPCB.EC._GPE' | sudo tee /proc/acpi/call; sudo cat /proc/acpi/call

Scope (\_SB) {
	#include <ec/clevo/it5570/ac.asl>
	#include <ec/clevo/it5570/battery.asl>
	#include <ec/clevo/it5570/buttons.asl>
	#include <ec/clevo/it5570/lid.asl>

	Scope (PCI0.LPCB) {
		#include <ec/clevo/it5570/ec.asl>
	}

	#include "hid.asl"
}

Scope (_GPE) {
	#include <ec/clevo/it5570/gpe.asl>
}
