/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <cpu/x86/lapic.h>
#include <program_loading.h>
#include <stdlib.h>
#include <security/tpm/tis.h>
#include <cpu/amd/msr.h>
#include <slrt.h>

#include <lib.h>

static uintptr_t payload_start, payload_size;

/* For CBFS_TYPE_SELF (Simple Elf), prog->start and prog->size are not set so obtain it differently. */
void platform_segment_loaded(uintptr_t start, size_t size, int flags)
{
	/* FIXME: need to differentiate between payload and other loaded segments */
	if (/*payload_start != 0 || payload_size != 0 || */flags != SEG_FINAL)
		die("ELF payload must have only one loadable segment for DRTM!\n");

	payload_start = start;
	payload_size = size;
}

void platform_prog_run(struct prog *prog)
{
	void *skl = NULL;
	uint16_t bootloader_data_offset;
	struct slr_table *slrt;
	struct slr_entry_dl_info *dl_info;
	struct slr_entry_hdr *end;

	/*
	 * Check if we're on 32b platform.
	 * TODO: add support for 64b?
	 */
	_Static_assert(sizeof(skl) == 4);

	hexdump(prog, sizeof(*prog));

	/*
	 * APs have to be in wait-for-SIPI state for at least 1000 cycles before
	 * SKINIT. Send INIT now and assume that loading SKL from CBFS is long
	 * enough.
	 */
	lapic_send_ipi_others(LAPIC_INT_LEVELTRIG | LAPIC_INT_ASSERT | LAPIC_MT_INIT);

	skl = memalign(64*KiB, 64*KiB);

	cbfs_load(CONFIG_CBFS_PREFIX "/drtm_payload", skl, 64*KiB);

	bootloader_data_offset = ((struct sl_header *)skl)->bootloader_data_offset;
	slrt = (struct slr_table *)(skl + bootloader_data_offset);

	memset(slrt, 0, 64*KiB - (skl - (void *)slrt));

	slrt->magic = SLR_TABLE_MAGIC;
	slrt->revision = SLR_TABLE_REVISION;
	slrt->architecture = SLR_AMD_SKINIT;
	slrt->size = sizeof(*slrt);
	slrt->max_size = 64*KiB - bootloader_data_offset;

	dl_info = (struct slr_entry_dl_info *)slrt->entries;
	dl_info->hdr.tag = SLR_ENTRY_DL_INFO;
	dl_info->hdr.size = sizeof(struct slr_entry_dl_info);
	dl_info->dlme_base = payload_start;
	dl_info->dlme_size = payload_size;
	dl_info->dlme_entry = (uint32_t)prog->entry - payload_start;
	dl_info->bl_context.bootloader = SLR_BOOTLOADER_GRUB; // TODO: BOOTLOADER_COREBOOT?
	slrt->size += dl_info->hdr.size;

	end = next_entry(dl_info);
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
