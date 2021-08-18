/* Initially copied from System76: src/mainboard/system76/cml-u/acpi */
/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#define COLOR_KEYBOARD 0

Scope (\_SB) {
	#include <ec/clevo/it5570/ac.asl>
	#include <ec/clevo/it5570/battery.asl>
	#include <ec/clevo/it5570/buttons.asl>
	#include <ec/clevo/it5570/hid.asl>
	#include <ec/clevo/it5570/lid.asl>
}


Scope (_GPE) {
	#include <ec/clevo/it5570/gpe.asl>
}
