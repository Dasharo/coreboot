/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <string.h>		// memset, memcpy
#include <timer.h>

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
	 * This will take more time, but makes the code easier to understand.
	 *
	 * TODO: check if we can skip both cpureg and save_self for nonfunctional
	 * cores
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

static void pba_reset(void)
{
	long time;
	/* Stopping Block Copy Download Engine
	*0x00068010                   // undocumented, PU_BCDE_CTL_SCOM
		[all] 0
		[0]   1
	*/
	write_scom(0x00068010, PPC_BIT(0));

	/* Stopping Block Copy Upload Engine
	*0x00068015                   // undocumented, PU_BCUE_CTL_SCOM
		[all] 0
		[0]   1
	*/
	write_scom(0x00068015, PPC_BIT(0));

	/* Polling on, to verify that BCDE & BCUE are indeed stopped
	timeout(256*256us):
		*0x00068012                   // undocumented, PU_BCDE_STAT_SCOM
		  [0] PBA_BC_STAT_RUNNING?
		*0x00068017                   // undocumented, PU_BCUE_STAT_SCOM
		  [0] PBA_BC_STAT_RUNNING?
		if both bits are clear: break
	*/
	time = wait_us(256*256,
	               (((read_scom(0x00068012) & PPC_BIT(0)) == 0) &&
	                ((read_scom(0x00068017) & PPC_BIT(0)) == 0)));

	if (!time)
		die("Timed out waiting for stopping of BCDE/BCUE\n");

	/* Clear the BCDE and BCUE stop bits */
	write_scom(0x00068010, 0);
	write_scom(0x00068015, 0);

	/* Reset each slave and wait for completion
	timeout(16*1us):
	  // This write is inside the timeout loop. I don't know if this will cause slaves to reset
	  // on each iteration or not, but this is how it is done in hostboot.
	  *0x00068001                 // undocumented, PU_PBASLVRST_SCOM
		[all] 0
		[0]   1     // reset?
		[1-2] sl
	  if *0x00068001[4 + sl] == 0: break      // 4 + sl: reset in progress?
	if *0x00068001[8 + sl]: die()             // 8 + sl: busy?
	*/
	for (int sl = 0; sl < 3; sl++) {	// Fourth is owned by SBE, do not reset
		time = wait_us(16,
		               (write_scom(0x00068001, PPC_BIT(0) | PPC_SHIFT(sl, 2)),
		                (read_scom(0x00068001) & PPC_BIT(4 + sl)) == 0));

		if (!time || read_scom(0x00068001) & PPC_BIT(8 + sl))
			die("Timed out waiting for slave %d reset\n", sl);
	}

	/* Reset PBA regs
	*0x00068013                       // undocumented, PU_BCDE_PBADR_SCOM
	*0x00068014                       // undocumented, PU_BCDE_OCIBAR_SCOM
	*0x00068015                       // undocumented, PU_BCUE_CTL_SCOM
	*0x00068016                       // undocumented, PU_BCUE_SET_SCOM
	*0x00068018                       // undocumented, PU_BCUE_PBADR_SCOM
	*0x00068019                       // undocumented, PU_BCUE_OCIBAR_SCOM
	*0x00068026                       // undocumented, PU_PBAXSHBR0_SCOM
	*0x0006802A                       // undocumented, PU_PBAXSHBR1_SCOM
	*0x00068027                       // undocumented, PU_PBAXSHCS0_SCOM
	*0x0006802B                       // undocumented, PU_PBAXSHCS1_SCOM
	*0x00068004                       // undocumented, PU_PBASLVCTL0_SCOM
	*0x00068005                       // undocumented, PU_PBASLVCTL1_SCOM
	*0x00068006                       // undocumented, PU_PBASLVCTL2_SCOM
	BRIDGE.PBA.PBAFIR                 // 0x05012840
	BRIDGE.PBA.PBAERRRPT0             // 0x0501284C
	  [all] 0
	*/
	write_scom(0x00068013, 0);
	write_scom(0x00068014, 0);
	write_scom(0x00068015, 0);
	write_scom(0x00068016, 0);
	write_scom(0x00068018, 0);
	write_scom(0x00068019, 0);
	write_scom(0x00068026, 0);
	write_scom(0x0006802A, 0);
	write_scom(0x00068027, 0);
	write_scom(0x0006802B, 0);
	write_scom(0x00068004, 0);
	write_scom(0x00068005, 0);
	write_scom(0x00068006, 0);
	write_scom(0x05012840, 0);
	write_scom(0x0501284C, 0);

	/* Perform non-zero reset operations
	BRIDGE.PBA.PBACFG                 // 0x0501284B
	  [all] 0
	  [38]  PBACFG_CHSW_DIS_GROUP_SCOPE = 1
	*/
	write_scom(0x0501284B, PPC_BIT(38));

	/*
	*0x00068021                       // undocumented, PU_PBAXCFG_SCOM
	  [all] 0
	  [2]   1   // PBAXCFG_SND_RESET?
	  [3]   1   // PBAXCFG_RCV_RESET?
	*/
	write_scom(0x00068021, PPC_BIT(2) | PPC_BIT(3));

	/*
	 * The following registers are undocumented. Their fields can be decoded
	 * from hostboot, but the values are always the same, so why bother...
	 */
	/* Set the PBA_MODECTL register */
	write_scom(0x00068000, 0x00A0BA9000000000);

	/* Slave 0 (SGPE and OCC boot) */
	write_scom(0x00068004, 0xB7005E0000000000);

	/* Slave 1 (405 ICU/DCU) */
	write_scom(0x00068005, 0xD5005E4000000000);

	/* Slave 2 (PGPE Boot) */
	write_scom(0x00068006, 0xA7005E4000000000);
}

static void stop_gpe_init(struct homer_st *homer)
{
	/* First check if SGPE_ACTIVE is not set in OCCFLAG register
	if (TP.TPCHIP.OCC.OCI.OCB.OCB_OCI_OCCFLG[8] == 1):        // 0x0006C08A
	  TP.TPCHIP.OCC.OCI.OCB.OCB_OCI_OCCFLG (CLEAR)            // 0x0006C08B
		[all] 0
		[8]   1       // SGPE_ACTIVE, bits in this register are defined by OCC firmware
	*/
	if (read_scom(0x0006C08A) & PPC_BIT(8)) {
		printk(BIOS_WARNING, "SGPE_ACTIVE is set in OCCFLAG register, clearing it\n");
		write_scom(0x0006C08B, PPC_BIT(8));
	}

	/*
	 * Program SGPE IVPR
	 * ATTR_STOPGPE_BOOT_COPIER_IVPR_OFFSET is set in updateGpeAttributes() in 15.1
	TP.TPCHIP.OCC.OCI.GPE3.GPEIVPR                            // 0x00066001
	  [all]   0
	  [0-31]  GPEIVPR_IVPR = ATTR_STOPGPE_BOOT_COPIER_IVPR_OFFSET
			  // Only bits [0-22] are actually defined, meaning IVPR must be aligned to 512B
	*/
	uint32_t ivpr = 0x80000000 + homer->qpmr.sgpe.header.l1_offset +
	                offsetof(struct homer_st, qpmr);
	write_scom(0x00066001, PPC_SHIFT(ivpr, 31));

	/* Program XCR to ACTIVATE SGPE
	TP.TPCHIP.OCC.OCI.GPE3.GPENXIXCR                          // 0x00066010
	  [all] 0
	  [1-3] PPE_XIXCR_XCR = 6     // hard reset
	TP.TPCHIP.OCC.OCI.GPE3.GPENXIXCR                          // 0x00066010
	  [all] 0
	  [1-3] PPE_XIXCR_XCR = 4     // toggle XSR[TRH]
	TP.TPCHIP.OCC.OCI.GPE3.GPENXIXCR                          // 0x00066010
	  [all] 0
	  [1-3] PPE_XIXCR_XCR = 2     // resume
	*/
	write_scom(0x00066010, PPC_SHIFT(6, 3));
	write_scom(0x00066010, PPC_SHIFT(4, 3));
	write_scom(0x00066010, PPC_SHIFT(2, 3));


	/*
	 * Now wait for SGPE to not be halted and for the HCode to indicate to be
	 * active.
	 * Warning: consts names in hostboot say timeouts are in ms, but code treats
	 * it as us. With debug output it takes much more than 20us between reads
	 * (~150us) and passes on 5th pass, which gives ~600us, +/- 150us on 4-core
	 * CPU (4 active CMEs).
	timeout(125*20us):
	  if ((TP.TPCHIP.OCC.OCI.OCB.OCB_OCI_OCCFLG[8] == 1) &&     // 0x0006C08A
		  (TP.TPCHIP.OCC.OCI.GPE3.GPEXIXSR[0] == 0)): break     // 0x00066021
	*/
	long time = wait_us(125*20, ((read_scom(0x0006C08A) & PPC_BIT(8)) &&
	                             !(read_scom(0x00066021) & PPC_BIT(0))));

	if (!time)
		die("Timeout while waiting for SGPE activation\n");
}

static uint64_t get_available_cores(void)
{
	uint64_t ret = 0;
	for (int i = 0; i < MAX_CORES; i++) {
		uint64_t val = read_scom_for_chiplet(EC00_CHIPLET_ID + i, 0xF0040);
		if (val & PPC_BIT(0)) {
			printk(BIOS_SPEW, "Core %d is functional%s\n", i,
			       (val & PPC_BIT(1)) ? "" : " and running");
			ret |= PPC_BIT(i);

			/* Might as well set multicast groups for cores */
			if ((read_scom_for_chiplet(EC00_CHIPLET_ID + i, 0xF0001) & PPC_BITMASK(3,5))
				== PPC_BITMASK(3,5))
				scom_and_or_for_chiplet(EC00_CHIPLET_ID + i, 0xF0001,
				                        ~(PPC_BITMASK(3,5) | PPC_BITMASK(16,23)),
				                        PPC_BITMASK(19,21));

			if ((read_scom_for_chiplet(EC00_CHIPLET_ID + i, 0xF0002) & PPC_BITMASK(3,5))
				== PPC_BITMASK(3,5))
				scom_and_or_for_chiplet(EC00_CHIPLET_ID + i, 0xF0002,
				                        ~(PPC_BITMASK(3,5) | PPC_BITMASK(16,23)),
				                        PPC_BIT(5) | PPC_BITMASK(19,21));
		}
	}
	return ret;
}

#define IS_EC_FUNCTIONAL(ec, cores)		(!!((cores) & PPC_BIT(ec)))
#define IS_EX_FUNCTIONAL(ex, cores)		(!!((cores) & PPC_BITMASK(2*(ex), 2*(ex) + 1)))
#define IS_EQ_FUNCTIONAL(eq, cores)		(!!((cores) & PPC_BITMASK(4*(eq), 4*(eq) + 3)))

/*
 * This logic is for SMF disabled only!
 */
void build_homer_image(void *homer_bar)
{
	struct mmap_helper_region_device mdev = {0};
	struct homer_st *homer = homer_bar;
	struct xip_hw_header *hw = homer_bar;
	uint8_t dd = get_dd();
	uint64_t cores = get_available_cores();

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

	/* 15.2 set HOMER BAR */
	write_scom(0x05012B00, (uint64_t)homer);
	write_scom(0x05012B04, (4 * MiB - 1) & ~((uint64_t)MiB - 1));

	/* 15.3 establish EX chiplet */
	/* Multicast groups for cores were assigned in get_available_cores() */
	for (int i = 0; i < MAX_CORES/4; i++) {
		if (IS_EQ_FUNCTIONAL(i, cores) &&
		    (read_scom_for_chiplet(EP00_CHIPLET_ID + i, 0xF0001) & PPC_BITMASK(3,5))
		    == PPC_BITMASK(3,5))
			scom_and_or_for_chiplet(EP00_CHIPLET_ID + i, 0xF0001,
			                        ~(PPC_BITMASK(3,5) | PPC_BITMASK(16,23)),
			                        PPC_BITMASK(19,21));
	}

	/*  Writing OCC CCSR */
	write_scom(0x0006C090, cores);

	/* Writing OCC QCSR */
	uint64_t qcsr = 0;
	for (int i = 0; i < MAX_CORES/2; i++) {
		if (IS_EX_FUNCTIONAL(i, cores))
			qcsr |= PPC_BIT(i);
	}
	write_scom(0x0006C094, qcsr);

	/* 15.4 start STOP engine */

	/* Initialize the PFET controllers */
	for (int i = 0; i < MAX_CORES; i++) {
		if ((i % 4) == 0 && IS_EQ_FUNCTIONAL(i/4, cores)) {
			/*
			TP.TPCHIP.NET.PCBSLEP03.PPMQ.PPM_COMMON_REGS.PPM_PFDLY        // 0x100F011B
				[all] 0
				[0-3] PPM_PFDLY_POWDN_DLY = 0x9     // 250ns, converted and encoded
				[4-7] PPM_PFDLY_POWUP_DLY = 0x9
			*/
			write_scom_for_chiplet(EP00_CHIPLET_ID + i/4, 0x100F011B,
			                       PPC_SHIFT(0x9, 3) | PPC_SHIFT(0x9, 7));
			/*
			TP.TPCHIP.NET.PCBSLEP03.PPMQ.PPM_COMMON_REGS.PPM_PFOF         // 0x100F011D
				[all] 0
				[0-3] PPM_PFOFF_VDD_VOFF_SEL =  0x8
				[4-7] PPM_PFOFF_VCS_VOFF_SEL =  0x8
			*/
			write_scom_for_chiplet(EP00_CHIPLET_ID + i/4, 0x100F011D,
			                       PPC_SHIFT(0x8, 3) | PPC_SHIFT(0x8, 7));
		}

		/*
		TP.TPCHIP.NET.PCBSLEC14.PPMC.PPM_COMMON_REGS.PPM_PFDLY        // 0x200F011B
			[all] 0
			[0-3] PPM_PFDLY_POWDN_DLY = 0x9     // 250ns, converted and encoded
			[4-7] PPM_PFDLY_POWUP_DLY = 0x9
		*/
		write_scom_for_chiplet(EC00_CHIPLET_ID + i, 0x200F011B,
		                       PPC_SHIFT(0x9, 3) | PPC_SHIFT(0x9, 7));
		/*
		TP.TPCHIP.NET.PCBSLEC14.PPMC.PPM_COMMON_REGS.PPM_PFOF         // 0x200F011D
			[all] 0
			[0-3] PPM_PFOFF_VDD_VOFF_SEL =  0x8
			[4-7] PPM_PFOFF_VCS_VOFF_SEL =  0x8
		*/
		write_scom_for_chiplet(EC00_CHIPLET_ID + i, 0x200F011D,
		                       PPC_SHIFT(0x8, 3) | PPC_SHIFT(0x8, 7));

		// Reriodic core quiesce workaround
		/*
		TP.TPCHIP.NET.PCBSLEC14.PPMC.PPM_CORE_REGS.CPPM_CPMMR (WOR)     // 0x200F0108
			[all] 0
			[2]   CPPM_CPMMR_RESERVED_2 = 1
		*/
		write_scom_for_chiplet(EC00_CHIPLET_ID + i, 0x200F0108, PPC_BIT(2));
	}

	/* Condition the PBA back to the base boot configuration */
	pba_reset();

	/*
	 * TODO: this is tested only if (ATTR_VDM_ENABLED || ATTR_IVRM_ENABLED),
	 * both are set (or not) in 15.1 - p9_pstate_parameter_block(). For now
	 * assume they are enabled.
	 */
	/* TP.TPCHIP.TPC.ITR.FMU.KVREF_AND_VMEAS_MODE_STATUS_REG     // 0x01020007
		if ([16] == 0): die()
	*/
	if (!(read_scom(0x01020007) & PPC_BIT(16)))
		die("VDMs/IVRM are enabled but necessary VREF calibration failed\n");

	/* First mask bit 7 in OIMR and then clear bit 7 in OISR
	TP.TPCHIP.OCC.OCI.OCB.OCB_OCI_OIMR0  (OR)               // 0x0006C006
		[all] 0
		[7]   OCB_OCI_OISR0_GPE2_ERROR =  1
	TP.TPCHIP.OCC.OCI.OCB.OCB_OCI_OISR0  (CLEAR)            // 0x0006C001
		[all] 0
		[7]   OCB_OCI_OISR0_GPE2_ERROR =  1
	*/
	write_scom(0x0006C006, PPC_BIT(7));
	write_scom(0x0006C001, PPC_BIT(7));

	/*
	 * Setup the SGPE Timer Selects
	 * These hardcoded values are assumed by the SGPE Hcode for setting up
	 * the FIT and Watchdog values.
	  TP.TPCHIP.OCC.OCI.GPE3.GPETSEL                          // 0x00066000
		[all] 0
		[0-3] GPETSEL_FIT_SEL =       0x1     // FIT - fixed interval timer
		[4-7] GPETSEL_WATCHDOG_SEL =  0xA
	 */
	write_scom(0x00066000, PPC_SHIFT(0x1, 3) | PPC_SHIFT(0xA, 7));

	/* Clear error injection bits
	  *0x0006C18B                         // undocumented, PU_OCB_OCI_OCCFLG2_CLEAR
		[all] 0
		[30]  1       // OCCFLG2_SGPE_HCODE_STOP_REQ_ERR_INJ
	*/
	write_scom(0x0006C18B, PPC_BIT(30));

	// Boot the STOP GPE
	stop_gpe_init(homer);

	//hexdump(&homer->qpmr, sgpe->qpmr.size);
}
