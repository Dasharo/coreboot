/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <string.h>		// memset, memcpy

#include "chip.h"
#include "homer.h"
#include "xip.h"

#include <lib.h>

extern void mount_part_from_pnor(const char *part_name,
				 struct mmap_helper_region_device *mdev);

static size_t copy_section(void *dst, struct xip_section *section, void *base,
                           uint8_t dd)
{
	if (!section->dd_support) {
		memcpy(dst, base + section->offset, section->size);
		return section->size;
	}

	struct dd_container *cont = base + section->offset;
	int i;

	assert(cont->magic == DD_CONTAINER_MAGIC);
	for (i = 0; i < cont->num; i++) {
		if (cont->blocks[i].dd == dd) {
			memcpy(dst, (void *)cont + cont->blocks[i].offset,
			       cont->blocks[i].size);
			return cont->blocks[i].size;
		}
	}

	die("XIP: Can't find container for DD=%x\n", dd);
}

static void build_sgpe(struct homer_st *homer, struct xip_sgpe_header *sgpe,
                       uint8_t dd)
{
	struct sgpe_img_header *hdr;
	size_t size;

	assert(sgpe->magic == XIP_MAGIC_SGPE);

	/* SPGE header */
	size = copy_section(&homer->qpmr.sgpe.header, &sgpe->qpmr, sgpe, dd);
	assert(size <= sizeof(struct qpmr_header));

	/*
	 * 0xFFF00000 (SRAM base) + 4k (IPC) + 60k (GPE0) + 64k (GPE1) + 50k (PGPE)
	 * + 2k (aux) + 2k (shared) = 0xFFF2D800
	 *
	 * WARNING: I have no idea if this is constant or depends on SPGE version.
	 */
	assert(homer->qpmr.sgpe.header.sram_region_start == 0xFFF2D800);
	assert(homer->qpmr.sgpe.header.sram_region_size == SGPE_SRAM_IMG_SIZE);
	/*
	 * Apart from these the only filled fields (same values for all DDs) are:
	 * - magic ("XIP SPGE")
	 * - build_date
	 * - build_ver
	 * - img_offset (0x0a00, overwritten with the same value later by code)
	 * - img_len (0xbf64, ~49kB, overwritten with the same value later by code)
	 */

	/* SPGE L1 bootloader */
	size = copy_section(&homer->qpmr.sgpe.l1_bootloader, &sgpe->l1_bootloader,
	                    sgpe, dd);
	homer->qpmr.sgpe.header.l1_offset = offsetof(struct qpmr_st,
	                                             sgpe.l1_bootloader);
	assert(size <= GPE_BOOTLOADER_SIZE);

	/* SPGE L2 bootloader */
	size = copy_section(&homer->qpmr.sgpe.l2_bootloader, &sgpe->l2_bootloader,
	                    sgpe, dd);
	homer->qpmr.sgpe.header.l2_offset = offsetof(struct qpmr_st,
	                                             sgpe.l2_bootloader);
	homer->qpmr.sgpe.header.l2_len = size;
	assert(size <= GPE_BOOTLOADER_SIZE);

	/* SPGE HCODE */
	size = copy_section(&homer->qpmr.sgpe.sram_image, &sgpe->hcode, sgpe, dd);
	homer->qpmr.sgpe.header.img_offset = offsetof(struct qpmr_st,
	                                              sgpe.sram_image);
	homer->qpmr.sgpe.header.img_len = size;
	assert(size <= SGPE_SRAM_IMG_SIZE);
	assert(size > (INT_VECTOR_SIZE + sizeof(struct sgpe_img_header)));

	/* Cache SCOM region */
	homer->qpmr.sgpe.header.scom_offset =
	         offsetof(struct qpmr_st, cache_scom_region);
	homer->qpmr.sgpe.header.scom_len = CACHE_SCOM_REGION_SIZE;

	/* Update SRAM image header */
	hdr = (struct sgpe_img_header *)
	      &homer->qpmr.sgpe.sram_image[INT_VECTOR_SIZE];
	hdr->ivpr_addr = homer->qpmr.sgpe.header.sram_region_start;
	hdr->cmn_ring_occ_offset = homer->qpmr.sgpe.header.img_len;
	hdr->cmn_ring_ovrd_occ_offset = 0;
	hdr->spec_ring_occ_offset = 0;
	hdr->scom_offset = 0;
	/* Nest frequency divided by 64. */
	hdr->timebase_hz = (1866 * MHz) / 64;

	/* SPGE auxiliary functions */
	/*
	 * TODO: check if it is really enabled. This comes from hostboot attributes,
	 * but I don't know if/where those are set.
	 */
	hdr->aux_control = 1 << 24;
	/*
	 * 0x80000000 (HOMER in OCI PBA memory space) + 1M (QPMR offset)
	 * + 512k (offset to aux)
	 */
	hdr->aux_offset = 0x80000000 + offsetof(struct homer_st, qpmr.aux);
	hdr->aux_len = CACHE_SCOM_AUX_SIZE;
	homer->qpmr.sgpe.header.enable_24x7_ima = 1;
	homer->qpmr.sgpe.header.aux_offset = offsetof(struct homer_st, qpmr.aux);
	homer->qpmr.sgpe.header.aux_len = CACHE_SCOM_AUX_SIZE;
}

static const uint32_t _SMF = 0x5F534D46; // "_SMF"

static const uint32_t ATTN_OP             = 0x00000200;
static const uint32_t BLR_OP              = 0x4E800020;
static const uint32_t SKIP_SPR_REST_INST  = 0x4800001C;
static const uint32_t MR_R0_TO_R10_OP     = 0x7C0A0378;
static const uint32_t MR_R0_TO_R21_OP     = 0x7C150378;
static const uint32_t MR_R0_TO_R9_OP      = 0x7C090378;
static const uint32_t MTLR_R30_OP         = 0x7FC803A6;
static const uint32_t MFLR_R30_OP         = 0x7FC802A6;

static const uint32_t init_cpureg_template[] = {
	0x63000000, /* ori %r24, %r0, 0        */ /* |= spr, key for lookup      */
	0x7C000278, /* xor %r0, %r0, %r0       */
	0x64000000, /* oris %r0, %r0, 0        */ /* |= val >> 48                */
	0x60000000, /* ori %r0, %r0, 0         */ /* |= (val >> 32) & 0x0000FFFF */
	0x780007C6, /* rldicr %r0, %r0, 32, 31 */
	0x64000000, /* oris %r0, %r0, 0        */ /* |= (val >> 16) & 0x0000FFFF */
	0x60000000, /* ori %r0, %r0, 0         */ /* |= val & 0x0000FFFF         */
	0x7C0003A6, /* mtspr 0, %r0            */ /* |= spr, encoded             */
};

/*
 * These SPRs are not described in PowerISA 3.0B.
 * MSR is not SPR, but self restore code treats it this way.
 */
#define SPR_USPRG0				0x1F0
#define SPR_USPRG1				0x1F1
#define SPR_URMOR				0x1F9
#define SPR_SMFCTRL				0x1FF
#define SPR_LDBAR				0x352
#define SPR_HID					0x3F0
#define SPR_MSR					0x7D0

static void add_init_cpureg_entry(uint32_t *base, uint16_t spr, uint64_t val,
                                  int init)
{
	while ((*base != (init_cpureg_template[0] | spr)) && *base != BLR_OP)
		base++;

	/* Must change next instruction from attn to blr when adding new entry. */
	if (*base == BLR_OP)
		*(base + ARRAY_SIZE(init_cpureg_template)) = BLR_OP;

	memcpy(base, init_cpureg_template, sizeof(init_cpureg_template));
	base[0] |= spr;

	if (init) {
		base[1] = SKIP_SPR_REST_INST;
	} else {
		base[2] |= (val >> 48) & 0xFFFF;
		base[3] |= (val >> 32) & 0xFFFF;
		base[5] |= (val >> 16) & 0xFFFF;
		base[6] |=  val        & 0xFFFF;
	}

	/* Few exceptions are handled differently. */
	if (spr == SPR_MSR) {
		base[7] = MR_R0_TO_R21_OP;
	} else if (spr == SPR_HRMOR) {
		base[7] = MR_R0_TO_R10_OP;
	} else if (spr == SPR_URMOR) {
		base[7] = MR_R0_TO_R9_OP;
	} else {
		base[7] |= ((spr & 0x1F) << 16) | ((spr & 0x3E) << 6);
	}
}

static const uint32_t init_save_self_template[] = {
	0x60000000, /* ori %r0, %r0, 0         */ /* |= i */
	0x3BFF0020, /* addi %r31, %r31, 0x20   */
	0x60000000, /* nop (ori %r0, %r0, 0)   */
};

/* Honestly, I have no idea why saving uses different key than restoring... */
static void add_init_save_self_entry(uint32_t **ptr, int i)
{
	memcpy(*ptr, init_save_self_template, sizeof(init_save_self_template));
	*ptr += ARRAY_SIZE(init_save_self_template);
}

static const uint16_t thread_sprs[] = {
	SPR_CIABR,
	SPR_DAWR,
	SPR_DAWRX,
	SPR_HSPRG0,
	SPR_LDBAR,
	SPR_LPCR,
	SPR_PSSCR,
	SPR_MSR,
	SPR_SMFCTRL,
	SPR_USPRG0,
	SPR_USPRG1,
};

static const uint16_t core_sprs[] = {
	SPR_HRMOR,
	SPR_HID,
	SPR_HMEER,
	SPR_PMCR,
	SPR_PTCR,
	SPR_URMOR,
};

static void build_self_restore(struct homer_st *homer,
                               struct xip_restore_header *rest, uint8_t dd)
{
	/* Assumptions: SMT4 only, SMF available but disabled. */
	size_t size;
	uint32_t *ptr;

	/*
	 * Data in XIP has its first 256 bytes zeroed, reserved for header, so even
	 * though this is exe part of self restore region, we should copy it to
	 * header's address.
	 */
	size = copy_section(&homer->cpmr.header, &rest->self, rest, dd);
	assert(size > sizeof(struct cpmr_header));

	/* Now, overwrite header. */
	size = copy_section(&homer->cpmr.header, &rest->cpmr, rest, dd);
	assert(size <= sizeof(struct cpmr_header));

	/*
	 * According to comment in p9_hcode_image_build.C it is for Nimbus >= DD22.
	 * Earlier versions do things differently. For now die(), implement if
	 * needed.
	 *
	 * If _SMF doesn't exist:
	 * - fill memory from (CPMR + 8k + 256) for 192k with ATTN
	 * - starting from the beginning of that region change instruction at every
	 *   2k bytes into BLR
	 *
	 * If _SMF exists:
	 * - fill CPMR.core_self_restore with ATTN instructions
	 * - for every core:
	 *   - change every thread's restore first instruction (at 0, 512, 1024,
	 *     1536 bytes) to BLR
	 *   - change core's restore first instruction (at 3k) to BLR
	 */
	if (*(uint32_t *)&homer->cpmr.exe[0x1300 - sizeof(struct cpmr_header)] !=
	    _SMF)
		die("No _SMF magic number in self restore region\n");

	ptr = (uint32_t *)homer->cpmr.core_self_restore;
	for (size = 0; size < (96 * KiB) / sizeof(uint32_t); size++) {
		ptr[size] = ATTN_OP;
	}

	/*
	 * This loop combines two functions from hostboot:
	 * initSelfRestoreRegion() and initSelfSaveRestoreEntries(). The second one
	 * writes only sections for functional cores, code below does it for all.
	 * This will take more time, but we don't have an easy and fast way of
	 * checking which cores are functional yet.
	 */
	for (int core = 0; core < MAX_CORES; core++) {
		struct smf_core_self_restore *csr = &homer->cpmr.core_self_restore[core];
		uint32_t *csa = csr->core_save_area;

		for (int thread = 0; thread < 4; thread++) {
			csr->thread_restore_area[thread][0] = BLR_OP;
			uint32_t *tsa = csr->thread_save_area[thread];
			*tsa++ = MFLR_R30_OP;

			for (int i = 0; i < ARRAY_SIZE(thread_sprs); i++) {
				/*
				 * Hostboot uses strange calculation for *_save_area keys.
				 * I don't know if this is only used by hostboot and save/restore
				 * code generated by it, or if something else (CME?) requires such
				 * format. For now leave it as hostboot does it, we can simplify
				 * this later.
				 *
				 * CIABR through MSR:       key =  0..7
				 * SMFCTRL through USPRG1:  key = 13..15
				 */
				int tsa_key = i;
				if (i > 7)
					tsa_key += 5;

				add_init_cpureg_entry(csr->thread_restore_area[thread],
				                      thread_sprs[i], 0, 1);
				add_init_save_self_entry(&tsa, tsa_key);
			}

			*tsa++ = MTLR_R30_OP;
			*tsa++ = BLR_OP;
		}

		csr->core_restore_area[0] = BLR_OP;
		*csa++ = MFLR_R30_OP;
		for (int i = 0; i < ARRAY_SIZE(core_sprs); i++) {
			add_init_cpureg_entry(csr->core_restore_area, core_sprs[i], 0, 1);
			/*
			 * HID through PTCR: key = 9..12
			 * HRMOR and URMOR are skipped.
			 */
			if (core_sprs[i] != SPR_HRMOR && core_sprs[i] != SPR_URMOR)
				add_init_save_self_entry(&csa, i + 8);
		}

		*csa++ = MTLR_R30_OP;
		*csa++ = BLR_OP;
	}

	/* Populate CPMR header */
	homer->cpmr.header.fused_mode_status = 0xAA;  // non-fused

	/* For SMF enabled */
#if 0
	homer->cpmr.header.urmor_fix = 1;
#endif

	homer->cpmr.header.self_restore_ver = 2;
	homer->cpmr.header.stop_api_ver = 2;

	/*
	 * WARNING: Hostboot filled CME header field with information whether cores
	 * are fused or not here. However, at this point CME image is not yet
	 * loaded, so that field will get overwritten.
	 */
}

static void build_cme(struct homer_st *homer, struct xip_cme_header *cme,
                      uint8_t dd)
{
	size_t size;
	struct cme_img_header *hdr;

	size = copy_section(&homer->cpmr.cme_sram_region, &cme->hcode, cme, dd);
	assert(size <= CME_SRAM_IMG_SIZE);
	assert(size > (INT_VECTOR_SIZE + sizeof(struct cme_img_header)));

	hdr = (struct cme_img_header *)
	      &homer->cpmr.cme_sram_region[INT_VECTOR_SIZE];

	hdr->hcode_offset = 0;
	hdr->hcode_len = size;

	hdr->pstate_region_offset = 0;
	hdr->pstate_region_len = 0;

	hdr->cpmr_phy_addr = (uint64_t) homer | 2 * MiB;

	hdr->common_ring_offset = hdr->hcode_offset + hdr->hcode_len;
	hdr->common_ring_len = 0;

	hdr->scom_offset = 0;
	hdr->scom_len = CORE_SCOM_RESTORE_SIZE / MAX_CORES / 2;

	hdr->core_spec_ring_offset = 0;
	hdr->max_spec_ring_len = 0;
}

static void build_pgpe(struct homer_st *homer, struct xip_pgpe_header *pgpe,
                       uint8_t dd)
{
	size_t size;
	struct pgpe_img_header *hdr;

	/* PPGE header */
	size = copy_section(&homer->ppmr.header, &pgpe->ppmr, pgpe, dd);
	assert(size <= PPMR_HEADER_SIZE);
	/*
	 * 0xFFF00000 (SRAM base) + 4k (IPC) + 60k (GPE0) + 64k (GPE1) = 0xFFF20000
	 *
	 * WARNING: I have no idea if this is constant or depends on SPGE version.
	 */
	assert(homer->ppmr.header.sram_region_start == 0xFFF20000);
	assert(homer->ppmr.header.sram_region_size == PGPE_SRAM_IMG_SIZE +
	                                              PGPE_AUX_TASK_SIZE +
	                                              PGPE_OCC_SHARED_SRAM_SIZE);

	/* PPGE L1 bootloader */
	size = copy_section(homer->ppmr.l1_bootloader, &pgpe->l1_bootloader, pgpe,
	                    dd);
	assert(size <= GPE_BOOTLOADER_SIZE);
	homer->ppmr.header.l1_offset = offsetof(struct ppmr_st, l1_bootloader);

	/* PPGE L2 bootloader */
	size = copy_section(homer->ppmr.l2_bootloader, &pgpe->l2_bootloader, pgpe,
	                    dd);
	assert(size <= GPE_BOOTLOADER_SIZE);
	homer->ppmr.header.l2_offset = offsetof(struct ppmr_st, l2_bootloader);
	homer->ppmr.header.l2_len = size;

	/* PPGE HCODE */
	size = copy_section(homer->ppmr.pgpe_sram_img, &pgpe->hcode, pgpe, dd);
	assert(size <= PGPE_SRAM_IMG_SIZE);
	assert(size > (INT_VECTOR_SIZE + sizeof(struct pgpe_img_header)));
	homer->ppmr.header.hcode_offset = offsetof(struct ppmr_st, pgpe_sram_img);
	homer->ppmr.header.hcode_len = size;

	/* PPGE auxiliary task */
	size = copy_section(homer->ppmr.aux_task, &pgpe->aux_task, pgpe, dd);
	assert(size <= PGPE_AUX_TASK_SIZE);
	homer->ppmr.header.aux_task_offset = offsetof(struct ppmr_st, aux_task);
	homer->ppmr.header.aux_task_len = size;

	/* 0x80000000 = HOMER in OCI PBA memory space */
	homer->ppmr.header.doptrace_offset =
	                0x80000000 + offsetof(struct homer_st, ppmr.doptrace);
	homer->ppmr.header.doptrace_len = PGPE_DOPTRACE_SIZE;

	/* Update SRAM image header */
	hdr = (struct pgpe_img_header *)
	      &homer->ppmr.pgpe_sram_img[INT_VECTOR_SIZE];

	/* SPGE auxiliary functions */
	/*
	 * TODO: check if it is really enabled. This comes from hostboot attributes,
	 * but I don't know if/where those are set.
	 */
	hdr->aux_controls = 1 << 24;
}

/*
 * This logic is for SMF disabled only!
 */
void build_homer_image(void *homer_bar)
{
	struct mmap_helper_region_device mdev = {0};
	struct homer_st *homer = homer_bar;
	struct xip_hw_header *hw = homer_bar;
	uint8_t dd = get_dd();

	printk(BIOS_ERR, "DD%2.2x\n", dd);

	/* HOMER must be aligned to 4M because CME HRMOR has bit for 2M set */
	if (!IS_ALIGNED((uint64_t) homer_bar, 4 * MiB))
		die("HOMER (%p) is not aligned to 4MB\n", homer_bar);

	memset(homer_bar, 0, 4 * MiB);

	/*
	 * This will work as long as we don't call mmap(). mmap() calls
	 * mem_poll_alloc() which doesn't check if mdev->pool is valid or at least
	 * not NULL.
	 */
	mount_part_from_pnor("HCODE", &mdev);
	/* First MB of HOMER is unused, we can write OCC image from PNOR there. */
	rdev_readat(&mdev.rdev, hw, 0, 1 * MiB);

	assert(hw->magic == XIP_MAGIC_HW);
	assert(hw->image_size <= 1 * MiB);

	build_sgpe(homer, (struct xip_sgpe_header *)(homer_bar + hw->sgpe.offset),
	           dd);

	build_self_restore(homer,
	                   (struct xip_restore_header *)(homer_bar + hw->restore.offset),
	                   dd);

	build_cme(homer, (struct xip_cme_header *)(homer_bar + hw->cme.offset), dd);

	build_pgpe(homer, (struct xip_pgpe_header *)(homer_bar + hw->pgpe.offset),
	           dd);

	//hexdump(&homer->qpmr, sgpe->qpmr.size);
}
