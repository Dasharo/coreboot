/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2024 3mdeb
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

#include <commonlib/cbfs_serialized.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <program_loading.h>
#include <soc/manifest.h>
#include <vb2_sha.h>

static void print_hash(const uint8_t *buf, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++)
		printk(BIOS_DEBUG, "%02X", buf[i]);

	printk(BIOS_DEBUG, "\n");
}

int prog_load_hook(struct prog *prog, void *buffer, size_t prog_size)
{
	const uint8_t *sbm_digest_ptr;
	const struct sb_manifest *sbm = (struct sb_manifest *)SB_MANIFEST_BASE;
	uint8_t digest[VB2_SHA256_DIGEST_SIZE];

	if (!prog || !buffer || prog_size == 0)
		return -1;

	switch (prog->type) {
	case PROG_RAMSTAGE:
		sbm_digest_ptr = sbm->oem_data.fw_hash[SbHashIdxRamstage].digest;
		break;
	case PROG_PAYLOAD:
		sbm_digest_ptr = sbm->oem_data.fw_hash[SbHashIdxPayload].digest;
		break;
	default:
		return 0;
	}

	printk (BIOS_INFO, "Verifying SHA256 hash of %s\n", prog->name);

	if (vb2_digest_buffer((const uint8_t*)buffer, prog_size,
			      VB2_HASH_SHA256, digest,
			      VB2_SHA256_DIGEST_SIZE) != VB2_SUCCESS) {
		return -2;
	}

	if (!memcmp((void *)sbm_digest_ptr, digest, VB2_SHA256_DIGEST_SIZE)) {
		printk (BIOS_INFO, "%s SHA256 hash verified\n", prog->name);
		return 0;
	}

	printk (BIOS_INFO, "SHA256 hash of %s:\n", prog->name);
	print_hash(sbm_digest_ptr, VB2_SHA256_DIGEST_SIZE);
	printk (BIOS_INFO, "Calculated:\n");
	print_hash(digest, VB2_SHA256_DIGEST_SIZE);

	return -3;
}
