/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 Google Inc.
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


#include <stdlib.h>
#include <cbfs.h>
#include <cbmem.h>
#include <console/console.h>
#include <fallback.h>
#include <halt.h>
#include <lib.h>
#include <program_loading.h>
#include <reset.h>
#include <romstage_handoff.h>
#include <rmodule.h>
#include <stage_cache.h>
#include <symbols.h>
#include <spi_flash.h>
#include <timestamp.h>
#include <fit_payload.h>
#include <vb2_api.h>
#include <commonlib/region.h>
#include <fmap.h>
#include <security/tpm/tspi.h>
#include <security/vboot/vbnv.h>
#include <drivers/spi/spi_flash_internal.h>

/* Only can represent up to 1 byte less than size_t. */
const struct mem_region_device addrspace_32bit =
	MEM_REGION_DEV_RO_INIT(0, ~0UL);

int prog_locate(struct prog *prog)
{
	struct cbfsf file;

	cbfs_prepare_program_locate();

#ifdef __RAMSTAGE__
	u32 cbfs_type;
	u8 sr0, sr1;
	const struct spi_flash *flash = boot_device_spi_flash();
	uint8_t data_hash[VB2_SHA256_DIGEST_SIZE];
	uint8_t golden_hash[VB2_SHA256_DIGEST_SIZE] = {
		0x66, 0x76, 0x93, 0x80, 0x53, 0x47, 0x01, 0x9f,
		0x3d, 0x0b, 0x80, 0x47, 0x87, 0x67, 0x6b, 0x01,
		0x6f, 0x4b, 0x90, 0xf9, 0xf5, 0x75, 0xcb, 0x9e,
		0xe0, 0xb9, 0x28, 0x82, 0x39, 0xda, 0x36, 0x5e,
	};
	void* prog_memmap;

	if (flash == NULL) {
		printk(BIOS_ALERT, "Boot device SPI flash not found\n");
		return -1;
	}


	if (spi_flash_status(flash, &sr0) < 0) {
		printk(BIOS_ALERT, "Failed to read SPI status register 0\n");
		return -1;
	}
	if (spi_flash_cmd(&flash->spi, 0x35, &sr1, sizeof(sr1))) {
		printk(BIOS_ALERT, "Failed to read SPI status register 1\n");
		return -1;
	}

	cbfs_prepare_program_locate();

	/* Check if we looking for payload and SPI flash is locked. */
	if (!strcmp(CONFIG_CBFS_PREFIX "/payload", prog_name(prog))
	    && ((sr0 & 0x80) == 0x80) && ((sr1 & 1) == 1))  {
		struct region_device rdev;
		cbfs_type = CBFS_TYPE_SELF;
		/* Locate PSPDIR just to fill the rdev fields */
		fmap_locate_area_as_rdev("PSPDIR", &rdev);
		/* Update the region fields to locate modified FW_MAIN_A region */
		rdev.region.offset=0x5a900;
		rdev.region.size=0xc5700;
		/* We should load UEFI payload from FW_MAIN_A now */
		if (cbfs_locate(&file, &rdev, prog_name(prog), &cbfs_type) < 0)
			return -1;
		cbfsf_file_type(&file, &prog->cbfs_type);
		cbfs_file_data(prog_rdev(prog), &file);
		if (tpm_measure_region(prog_rdev(prog), 2, "FMAP: FW_MAIN_A CBFS: fallback/payload"))
			return -1;
		prog_memmap = rdev_mmap_full(prog_rdev(prog));
                if (!prog_memmap)
			return -1;
                /* TODO verify SHA256sum to ensure verified boot is preserved */
		vb2_digest_buffer((const uint8_t *)prog_memmap,
                                  region_device_sz(prog_rdev(prog)),
                                  VB2_HASH_SHA256, data_hash,
                                  sizeof(data_hash));
 
                rdev_munmap(prog_rdev(prog), prog_memmap);
 
                if (!memcmp((const void *) golden_hash,
                            (const void *) data_hash,
                            VB2_SHA256_DIGEST_SIZE)) {
			set_recovery_mode_into_vbnv(0);
                        return 0;
                } else {
                        printk(BIOS_ALERT, "Failed to verify payload integrity\n");
			hexdump(golden_hash, VB2_SHA256_DIGEST_SIZE);
			hexdump(data_hash, VB2_SHA256_DIGEST_SIZE);
                        return -1;
                }

		return 0;
	}
#endif
	if (cbfs_boot_locate(&file, prog_name(prog), NULL))
		return -1;

	cbfsf_file_type(&file, &prog->cbfs_type);

	cbfs_file_data(prog_rdev(prog), &file);

	return 0;
}

void run_romstage(void)
{
	struct prog romstage =
		PROG_INIT(PROG_ROMSTAGE, CONFIG_CBFS_PREFIX "/romstage");

	if (prog_locate(&romstage))
		goto fail;

	timestamp_add_now(TS_START_COPYROM);

	if (cbfs_prog_stage_load(&romstage))
		goto fail;

	timestamp_add_now(TS_END_COPYROM);

	prog_run(&romstage);

fail:
	if (CONFIG(BOOTBLOCK_CONSOLE))
		die_with_post_code(POST_INVALID_ROM,
				   "Couldn't load romstage.\n");
	halt();
}

void __weak stage_cache_add(int stage_id,
						const struct prog *stage) {}
void __weak stage_cache_load_stage(int stage_id,
							struct prog *stage) {}

static void ramstage_cache_invalid(void)
{
	printk(BIOS_ERR, "ramstage cache invalid.\n");
	if (CONFIG(RESET_ON_INVALID_RAMSTAGE_CACHE)) {
		board_reset();
	}
}

static void run_ramstage_from_resume(struct prog *ramstage)
{
	if (!romstage_handoff_is_resume())
		return;

	/* Load the cached ramstage to runtime location. */
	stage_cache_load_stage(STAGE_RAMSTAGE, ramstage);

	if (prog_entry(ramstage) != NULL) {
		printk(BIOS_DEBUG, "Jumping to image.\n");
		prog_run(ramstage);
	}
	ramstage_cache_invalid();
}

static int load_relocatable_ramstage(struct prog *ramstage)
{
	struct rmod_stage_load rmod_ram = {
		.cbmem_id = CBMEM_ID_RAMSTAGE,
		.prog = ramstage,
	};

	return rmodule_stage_load(&rmod_ram);
}

static int load_nonrelocatable_ramstage(struct prog *ramstage)
{
	if (CONFIG(HAVE_ACPI_RESUME)) {
		uintptr_t base = 0;
		size_t size = cbfs_prog_stage_section(ramstage, &base);
		if (size)
			backup_ramstage_section(base, size);
	}

	return cbfs_prog_stage_load(ramstage);
}

void run_ramstage(void)
{
	struct prog ramstage =
		PROG_INIT(PROG_RAMSTAGE, CONFIG_CBFS_PREFIX "/ramstage");

	if (ENV_POSTCAR)
		timestamp_add_now(TS_END_POSTCAR);

	/* Call "end of romstage" here if postcar stage doesn't exist */
	if (ENV_ROMSTAGE)
		timestamp_add_now(TS_END_ROMSTAGE);

	/*
	 * Only x86 systems using ramstage stage cache currently take the same
	 * firmware path on resume.
	 */
	if (CONFIG(ARCH_X86) &&
	    !CONFIG(NO_STAGE_CACHE))
		run_ramstage_from_resume(&ramstage);

	if (prog_locate(&ramstage))
		goto fail;

	timestamp_add_now(TS_START_COPYRAM);

	if (CONFIG(RELOCATABLE_RAMSTAGE)) {
		if (load_relocatable_ramstage(&ramstage))
			goto fail;
	} else if (load_nonrelocatable_ramstage(&ramstage))
		goto fail;

	if (!CONFIG(NO_STAGE_CACHE))
		stage_cache_add(STAGE_RAMSTAGE, &ramstage);

	timestamp_add_now(TS_END_COPYRAM);

	prog_run(&ramstage);

fail:
	die_with_post_code(POST_INVALID_ROM, "Ramstage was not loaded!\n");
}

#ifdef __RAMSTAGE__ // gc-sections should take care of this

static struct prog global_payload =
	PROG_INIT(PROG_PAYLOAD, CONFIG_CBFS_PREFIX "/payload");

void __weak mirror_payload(struct prog *payload)
{
}

void payload_load(void)
{
	struct prog *payload = &global_payload;

	timestamp_add_now(TS_LOAD_PAYLOAD);

	if (prog_locate(payload))
		goto out;

	mirror_payload(payload);

	switch (prog_cbfs_type(payload)) {
	case CBFS_TYPE_SELF: /* Simple ELF */
		selfload_check(payload, BM_MEM_RAM);
		break;
	case CBFS_TYPE_FIT: /* Flattened image tree */
		if (CONFIG(PAYLOAD_FIT_SUPPORT)) {
			fit_payload(payload);
			break;
		} /* else fall-through */
	default:
		die_with_post_code(POST_INVALID_ROM,
				   "Unsupported payload type.\n");
		break;
	}

out:
	if (prog_entry(payload) == NULL) {
		printk(BIOS_ALERT, "Payload not loaded.\n");
		set_recovery_mode_into_vbnv(0x03);
		board_reset();
	}
}

void payload_run(void)
{
	struct prog *payload = &global_payload;

	/* Reset to booting from this image as late as possible */
	boot_successful();

	printk(BIOS_DEBUG, "Jumping to boot code at %p(%p)\n",
		prog_entry(payload), prog_entry_arg(payload));

	post_code(POST_ENTER_ELF_BOOT);

	timestamp_add_now(TS_SELFBOOT_JUMP);

	/* Before we go off to run the payload, see if
	 * we stayed within our bounds.
	 */
	checkstack(_estack, 0);

	prog_run(payload);
}

#endif
