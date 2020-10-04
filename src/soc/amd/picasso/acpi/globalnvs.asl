/* SPDX-License-Identifier: GPL-2.0-or-later */

/* Global variables */
Name (PMOD, Zero)	/* Interrupt Mode used by OS. Assume PIC. */

/*
 * NOTE: The layout of the GNVS structure below must match the layout in
 * soc/amd/picasso/include/soc/nvs.h !!!
 *
 */

External (NVSA)

OperationRegion (GNVS, SystemMemory, NVSA, 0x1000)
Field (GNVS, ByteAcc, NoLock, Preserve)
{
	/* Miscellaneous */
	Offset (0x00),
	PCNT,	8,	// 0x00 - Processor Count
	LIDS,	8,	// 0x01 - LID State
	PWRS,	8,	// 0x02 - AC Power State
	CBMC,	32,	// 0x03 - 0x06 - coreboot Memory Console
	PM1I,	64,	// 0x07 - 0x0e - System Wake Source - PM1 Index
	GPEI,	64,	// 0x0f - 0x16 - GPE Wake Source
	TMPS,	8,	// 0x17 - Temperature Sensor ID
	TCRT,	8,	// 0x18 - Critical Threshold
	TPSV,	8,	// 0x19 - Passive Threshold
	/* ChromeOS stuff (0x100 -> 0xfff, size 0xeff) */
	Offset (0x100),
	#include <vendorcode/google/chromeos/acpi/gnvs.asl>
}
