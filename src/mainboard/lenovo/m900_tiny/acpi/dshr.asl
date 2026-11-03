/* SPDX-License-Identifier: GPL-2.0-only */

Scope (\_SB) {
	/* Must match the EC (LDN 0xb) I/O base in devicetree.cb */
	#define NCT6687D_EC_BASE 0xa20
	#include <superio/nuvoton/nct6687d/acpi/dshr.asl>
}
