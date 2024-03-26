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

int prog_load_hook(struct prog *prog)
{
	struct cbfs_stage stage;
	const uint8_t *sbm_digest_ptr;
	void *start;
	size_t fsize;
	size_t foffset;
	const struct sb_manifest *sbm = (struct sb_manifest *)SB_MANIFEST_BASE;
	const struct region_device *fh = prog_rdev(prog);
	uint8_t digest[VB2_SHA256_DIGEST_SIZE];

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

	if (rdev_readat(fh, &stage, 0, sizeof(stage)) != sizeof(stage))
		return -1;

	fsize = region_device_sz(fh);
	fsize -= sizeof(stage);
	foffset = 0;
	foffset += sizeof(stage);

	if (stage.len != fsize)
		return -2;

	start = rdev_mmap(fh, foffset, fsize);

	if (!start)
		return -3;

	if (vb2_digest_buffer((const uint8_t*)start, fsize,
			      VB2_HASH_SHA256, digest,
			      VB2_SHA256_DIGEST_SIZE) != VB2_SUCCESS) {
		rdev_munmap(fh, start);
		return -4;
	}

	if (!memcmp((void *)sbm_digest_ptr, digest, VB2_SHA256_DIGEST_SIZE)) {
		printk (BIOS_INFO, "%s SHA256 hash verified\n", prog->name);
		rdev_munmap(fh, start);
		return 0;
	}

	rdev_munmap(fh, start);
	return -5;
}
