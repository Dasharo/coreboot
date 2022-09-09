/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <cpu/x86/lapic.h>
#include <program_loading.h>
#include <stdlib.h>
#include <security/tpm/tis.h>

#include <lib.h>

static uintptr_t payload_start, payload_size;

/* For CBFS_TYPE_SELF (Simple Elf), prog->start and prog->size are not set so obtain it differently. */
void platform_segment_loaded(uintptr_t start, size_t size, int flags)
{
	if (payload_start != 0 || payload_size != 0 || flags != SEG_FINAL)
		die("ELF payload must have only one loadable segment for DRTM!\n");

	/* SIPI vector loading also goes through this function, skip it. */
	if (size <= 4*KiB)
		return;

	payload_start = start;
	payload_size = size;
}

/* TODO: include tags.h from SKL somehow */
#define SKL_TAG_CLASS_MASK       0xF0

/* Tags with no particular class */
#define SKL_TAG_NO_CLASS         0x00
#define SKL_TAG_END              0x00
#define SKL_TAG_SETUP_INDIRECT   0x01
#define SKL_TAG_TAGS_SIZE        0x0F    /* Always first */

/* Tags specifying kernel type */
#define SKL_TAG_BOOT_CLASS       0x10
#define SKL_TAG_BOOT_LINUX       0x10
#define SKL_TAG_BOOT_MB2         0x11
#define SKL_TAG_BOOT_SIMPLE      0x12

struct skl_tag_hdr {
	uint8_t type;
	uint8_t len;
} __packed;

struct skl_tag_tags_size {
	struct skl_tag_hdr hdr;
	uint16_t size;
} __packed;

struct skl_tag_boot_simple_payload {
	struct skl_tag_hdr hdr;
	uint32_t base;
	uint32_t size;
	uint32_t entry;
	uint32_t arg;
} __packed;

struct skl_tag_evtlog {
	struct skl_tag_hdr hdr;
	uint32_t address;
	uint32_t size;
} __packed;

struct skl_tag_hash {
	struct skl_tag_hdr hdr;
	uint16_t algo_id;
	uint8_t digest[];
} __packed;

static inline void *next_tag(void* t)
{
	void *x = t + ((struct skl_tag_hdr*)t)->len;
	return x;
}

void platform_prog_run(struct prog *prog)
{
	void *skl = NULL;
	uint16_t bootloader_data_offset;
	struct skl_tag_tags_size *tags;
	struct skl_tag_boot_simple_payload *sp;
	struct skl_tag_hdr *end;

	/*
	 * Check if we're on 32b platform.
	 * TODO: add support for 64b?
	 */
	assert(sizeof(skl) == 4);

	hexdump(prog, sizeof(*prog));

	/*
	 * APs have to be in wait-for-SIPI state for at least 1000 cycles before
	 * SKINIT. Send INIT now and assume that loading SKL from CBFS is long
	 * enough.
	 */
	lapic_send_ipi_others(LAPIC_INT_LEVELTRIG | LAPIC_INT_ASSERT | LAPIC_MT_INIT);

	skl = memalign(64*KiB, 64*KiB);

	cbfs_load(CONFIG_CBFS_PREFIX "/drtm_payload", skl, 64*KiB);

	bootloader_data_offset = ((uint16_t *)skl)[1];
	tags = (struct skl_tag_tags_size *)(skl + bootloader_data_offset);

	memset(tags, 0, 64*KiB - (skl - (void *)tags));

	tags->hdr.type = SKL_TAG_TAGS_SIZE;
	tags->hdr.len = sizeof(struct skl_tag_tags_size);
	tags->size += tags->hdr.len;

	sp = next_tag(tags);
	sp->hdr.type = SKL_TAG_BOOT_SIMPLE;
	sp->hdr.len = sizeof(struct skl_tag_boot_simple_payload);
	sp->base = payload_start;
	sp->size = payload_size;
	sp->entry = (uint32_t)prog->entry;
	sp->arg = (uint32_t)prog->arg;
	tags->size += sp->hdr.len;

	/* TODO: DRTM TPM event log */

	end = next_tag(sp);
	end->type = SKL_TAG_END;
	end->len = sizeof(struct skl_tag_hdr);
	tags->size += end->len;

	asm volatile ("skinit" :: "a"(skl));
}
