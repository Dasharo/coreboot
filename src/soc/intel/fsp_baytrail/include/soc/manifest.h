/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2013-2014 Sage Electronic Engineering, LLC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _BAYTRAIL_MANIFEST_H_
#define _BAYTRAIL_MANIFEST_H_

#include <vb2_sha.h>

#define SB_MANIFEST_BASE		(0xfffe0000)
#define SB_MANIFEST_SIZE		0x1000
#define SB_MANIFEST_MAGIC		0x4d425624  /* $VBM in LE */
#define SB_MANIFEST_VERSION		0x1

#define KEY_MANIFEST_BASE		(0xfffe0000 - 0x1000)
#define KEY_MANIFEST_SIZE		0x1000
#define KEY_MANIFEST_MAGIC		0x4d4b5324  /* $SKM in LE */
#define KEY_MANIFEST_VERSION		0x1

#define SBM_OEM_DATA_VERSION		0x1
#define SBM_OEM_DATA_TYPE		0x2

/* Add more indices here if needed */
enum component_hash_index {
	SbHashIdxRamstage,
	SbHashIdxFspStage2,
	SbHashIdxPayload,
	SbHashIdxCount
};

struct manifest_header {
	uint32_t magic;
	uint32_t version;
	uint32_t size;
	uint32_t svn;
} __packed;

struct hash_sha256 {
	uint8_t digest[VB2_SHA256_DIGEST_SIZE];
} __packed;

struct sb_oem_data {
	uint16_t version;	/* Should be 0x1 */
	uint16_t type;		/* Should be 0x2 */
	struct hash_sha256 fw_hash[SbHashIdxCount];
	uint8_t reserved_oem[400 - 4 - sizeof(struct hash_sha256)*SbHashIdxCount];
} __packed;

struct key_signature {
	uint8_t modulus[256];
	uint32_t exponent;
	uint8_t signature[256];
} __packed;

struct sb_manifest {
	struct manifest_header header;
	uint32_t reserved1;
	struct hash_sha256 ibb_hash;
	uint32_t reserved2;
	uint32_t reserved3[8];
	struct sb_oem_data oem_data;
	uint8_t reserved4[20];
	struct key_signature key_sig;
} __packed;

struct key_manifest {
	struct manifest_header header;
	uint32_t km_id;
	struct hash_sha256 sb_key_hash;
	uint8_t padding[456];
	struct key_signature key_sig;
} __packed;

#endif
