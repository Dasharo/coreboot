/* SPDX-License-Identifier: GPL-2.0-or-later */

/*
 * NOTE: The layout of the GNVS structure below must match the layout in
 * soc/intel/apollolake/include/soc/nvs.h !!!
 *
 */

External (NVSA)

OperationRegion (GNVS, SystemMemory, NVSA, 0x1000)
Field (GNVS, ByteAcc, NoLock, Preserve)
{
	/* Miscellaneous */
	PCNT,	8,      // 0x00 - Processor Count
	PPCM,	8,      // 0x01 - Max PPC State
	LIDS,	8,      // 0x02 - LID State
	PWRS,	8,      // 0x03 - AC Power State
	DPTE,	8,      // 0x04 - Enable DPTF
	CBMC,	32,     // 0x05 - 0x08 - coreboot Memory Console
	PM1I,	64,     // 0x09 - 0x10 - System Wake Source - PM1 Index
	GPEI,	64,     // 0x11 - 0x18 - GPE Wake Source
	NHLA,	64,     // 0x19 - 0x20 - NHLT Address
	NHLL,	32,     // 0x21 - 0x24 - NHLT Length
	PRT0,	32,     // 0x25 - 0x28 - PERST_0 Address
	SCDP,	8,      // 0x29 - SD_CD GPIO portid
	SCDO,	8,      // 0x2A - GPIO pad offset relative to the community
	UIOR,	8,      // 0x2B - UART debug controller init on S3 resume
	EPCS,   8,      // 0x2C - SGX Enabled status
	EMNA,   64,     // 0x2D - 0x34 EPC base address
	ELNG,   64,     // 0x35 - 0x3C EPC Length
	A4GB,	64,	// 0x3D - 0x44 Base of above 4GB MMIO Resource
	A4GS,	64,	// 0x45 - 0x4C Length of above 4GB MMIO Resource

	/* ChromeOS stuff (0x100 -> 0xfff, size 0xeff) */
	Offset (0x100),
	#include <vendorcode/google/chromeos/acpi/gnvs.asl>
}
