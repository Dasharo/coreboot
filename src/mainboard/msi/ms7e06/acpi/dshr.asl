/* SPDX-License-Identifier: GPL-2.0-only */

Scope (\_SB) {
	/* Must match EC_IO_BASE in bootblock.c */
	#define NCT6687D_EC_BASE	0xa20
	/* CPU_FAN and PUMP_FAN */
	#define NCT6687D_DSHR_CPU_FANS	0x03
	#include <superio/nuvoton/nct6687d/acpi/dshr.asl>
}
