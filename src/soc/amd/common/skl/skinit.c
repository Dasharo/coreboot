/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <cpu/x86/lapic.h>
#include <program_loading.h>
#include <stdlib.h>
#include <security/tpm/tis.h>
#include <cpu/amd/msr.h>
#include <slrt/slrt.h>

#include <lib.h>

static uintptr_t payload_start, payload_size;

/* For SELF, prog->start and prog->size are not set so obtain it differently. */
void platform_segment_loaded(uintptr_t start, size_t size, int flags)
{
	/* FIXME: need to differentiate between payload and other loaded segments */
	if (/*payload_start != 0 || payload_size != 0 || */flags != SEG_FINAL)
		die("ELF payload must have only one loadable segment for DRTM!\n");

	payload_start = start;
	payload_size = size;
}

/* TODO: move into slrt.h */
struct sl_header {
	uint16_t _entry;
	uint16_t _end_of_measured;
	uint8_t reserved1[16];
	uint32_t reserved2;
	uint32_t reserved3;
	uint16_t reserved4;
	uint32_t reserved5[9];
	uint16_t skl_info;
	uint16_t bootloader_data;
} __packed;

static inline void *next_tag(void* t)
{
	void *x = t + ((struct slr_entry_hdr*)t)->size;
	return x;
}

void platform_prog_run(struct prog *prog)
{
	void *skl = NULL;
	uint16_t bootloader_data_offset;
	struct slr_table *slrt;
	struct slr_entry_dl_info *dl_info;
	struct slr_entry_amd_info *amd_info;
	struct slr_entry_hdr *end;

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
	lapic_send_ipi_others(LAPIC_INT_LEVELTRIG | LAPIC_INT_ASSERT | LAPIC_DM_INIT);

	skl = memalign(64*KiB, 64*KiB);

	cbfs_load(CONFIG_CBFS_PREFIX "/drtm_payload", skl, 64*KiB);

	bootloader_data_offset = ((struct sl_header *)skl)->bootloader_data;
	slrt = (struct slr_table *)(skl + bootloader_data_offset);

	memset(slrt, 0, 64*KiB - (skl - (void *)slrt));

	slrt->magic = 0x4452544d;
	slrt->revision = 0x1;
	slrt->architecture = 0x2;
	slrt->size = 16;
	slrt->max_size = 0;

	dl_info = (struct slr_entry_dl_info *)(skl + bootloader_data_offset + 16); // TODO: Do Smarter
	dl_info->hdr.tag = SLR_ENTRY_DL_INFO;
	dl_info->hdr.size = sizeof(struct slr_entry_dl_info);
	dl_info->dlme_base = payload_start;
	dl_info->dlme_size = payload_size;
	dl_info->dlme_entry = (uint32_t)prog->entry;
	dl_info->bl_context.bootloader = SLR_BOOTLOADER_GRUB; // TODO: BOOTLOADER_COREBOOT?
	slrt->size += dl_info->hdr.size;

	amd_info = next_tag(dl_info);
	amd_info->hdr.tag = SLR_ENTRY_AMD_INFO;
	amd_info->hdr.size = sizeof(*amd_info);
	slrt->size += amd_info->hdr.size;

	end = next_tag(amd_info);
	end->tag = SLR_ENTRY_END;
	end->size = sizeof(struct slr_entry_hdr);
	slrt->size += end->size;

	/* TODO: DRTM TPM event log and SKL hash(es) */

	msr_t msr;

	msr = rdmsr(SMM_BASE_MSR);
	printk(BIOS_DEBUG, "SMM_BASE_MSR = %#8.8x%8.8x\n", msr.hi, msr.lo);

	msr = rdmsr(SMM_ADDR_MSR);
	printk(BIOS_DEBUG, "SMM_ADDR_MSR = %#8.8x%8.8x\n", msr.hi, msr.lo);

	msr = rdmsr(SMM_MASK_MSR);
	printk(BIOS_DEBUG, "SMM_MASK_MSR = %#8.8x%8.8x\n", msr.hi, msr.lo);

	msr = rdmsr(HWCR_MSR);
	printk(BIOS_DEBUG, "HWCR_MSR     = %#8.8x%8.8x\n", msr.hi, msr.lo);

	asm volatile ("skinit" :: "a"(skl));
}
