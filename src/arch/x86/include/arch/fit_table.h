/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _FIT_TABLE_H_
#define _FIT_TABLE_H_

#include <stdint.h>

#define FIT_ENTRY_HAS_CHECKSUM   0x80

/*
 * From "Firmware Interface Table, BIOS Specification" (document #599500), Rev. 1.6, March 2025
 */
#define FIT_ENTRY_TYPE_HEADER    0x00 // FIT Header Entry
#define FIT_ENTRY_TYPE_UCODE     0x01 // Microcode Update Entry
#define FIT_ENTRY_TYPE_SACM      0x02 // Startup AC Module Entry
#define FIT_ENTRY_TYPE_DACM      0x03 // Diagnostic AC Module Entry
#define FIT_ENTRY_TYPE_PBP       0x04 // Platform Boot Policy Entry
#define FIT_ENTRY_TYPE_MMF       0x05 // Memory Microcontroller Firmware Entry
#define FIT_ENTRY_TYPE_FRS       0x06 // FIT Reset State Entry
#define FIT_ENTRY_TYPE_BIOS_SACM 0x07 // BIOS Startup Module Entry
#define FIT_ENTRY_TYPE_TPM       0x08 // TPM Policy Record
#define FIT_ENTRY_TYPE_BIOS      0x09 // BIOS Policy Record
#define FIT_ENTRY_TYPE_TXT       0x0a // TXT Policy Record
#define FIT_ENTRY_TYPE_KM        0x0b // Key Manifest Record
#define FIT_ENTRY_TYPE_BPM       0x0c // Boot Policy Manifest
#define FIT_ENTRY_TYPE_FSP       0x0d // FSP Boot Manifest
// 0x0E - 0x0F Intel Reserved
#define FIT_ENTRY_TYPE_CSE       0x10 // CSE Secure Boot
// 0x11 - 0x19 Intel Reserved
#define FIT_ENTRY_TYPE_VAB_PT    0x1a // Vendor Authorized Boot Provisioning Table
#define FIT_ENTRY_TYPE_VAB_KM    0x1b // Vendor Authorized Boot Key Manifest
#define FIT_ENTRY_TYPE_VAB_IM    0x1c // Vendor Authorized Boot Image Manifest
#define FIT_ENTRY_TYPE_VAB_IH    0x1d // Vendor Authorized Boot Image Hash (Deprecated)
#define FIT_ENTRY_TYPE_VAB_ACPIM 0x1e // VAB Authorization Code Patch Image Manifest
// 0x1F - 0x2B Intel Reserved
#define FIT_ENTRY_TYPE_SACM_DBG  0x2c // SACM Debug Record
#define FIT_ENTRY_TYPE_FPDR      0x2d // Feature Policy Delivery Record.
				      // Deprecated in CBnT platforms.
#define FIT_ENTRY_TYPE_SCRTM_ERR 0x2e // Granular SCRTM Error Record
#define FIT_ENTRY_TYPE_JMP_DBG   0x2f // JMP $ Debug Policy
// 0x30 - 0x70 Reserved for Platform Manufacturer Use
// 0x71 - 0x7E Intel Reserved
#define FIT_ENTRY_TYPE_UNUSED    0x7f // Unused Entry (skip)

struct fit_entry {
	union {
		uint8_t bytes[sizeof(uint64_t)];
		uint64_t u64;
	} address;
	uint32_t size_reserved;
	uint16_t version;
	uint8_t  type_checksum_valid;
	uint8_t  checksum;
} __packed;

#endif /* _FIT_TABLE_H_ */
