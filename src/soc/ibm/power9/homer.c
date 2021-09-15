/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <cbfs.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <cpu/power/mvpd.h>
#include <cpu/power/powerbus.h>
#include <cpu/power/scom.h>
#include <cpu/power/spr.h>
#include <string.h>		// memset, memcpy
#include <timer.h>

#include "chip.h"
#include "homer.h"
#include "tor.h"
#include "xip.h"
#include "pstates_include/p9_pstates_cmeqm.h"

#include <lib.h>

#define L2_EPS_DIVIDER   1
#define L3_EPS_DIVIDER   1

#define EX_L2_RD_EPS_REG 0x10010810
#define EX_L2_WR_EPS_REG 0x10010811
#define EX_L3_RD_EPS_REG 0x10011829
#define EX_L3_WR_EPS_REG 0x1001182A
#define EX_DRAM_REF_REG  0x1001180F
#define EX_0_NCU_DARN_BAR_REG 0x10011011

#define ODD_EVEN_EX_POS  0x00000400

#define MAX_EQ_SCOM_ENTRIES 31
#define MAX_L2_SCOM_ENTRIES 16
#define MAX_L3_SCOM_ENTRIES 16

#define QUAD_BIT_POS     24

#define PPC_PLACE(val, pos, len) \
	PPC_SHIFT((val) & ((1 << ((len) + 1)) - 1), ((pos) + ((len) - 1)))

/* Subsections of STOP image that contain SCOM entries */
enum scom_section {
	STOP_SECTION_CORE_SCOM,
	STOP_SECTION_EQ_SCOM,
	STOP_SECTION_L2,
	STOP_SECTION_L3,
};

struct ring_data {
	void *rings_buf;
	void *work_buf1;
	void *work_buf2;
	void *work_buf3;
	uint32_t rings_buf_size;
	uint32_t work_buf1_size;
	uint32_t work_buf2_size;
	uint32_t work_buf3_size;
};

struct cme_cmn_ring_list {
	uint16_t ring[8]; // In order: EC_FUNC, EC_GPTR, EC_TIME, EC_MODE, EC_ABST, 3 reserved
	uint8_t payload[];
};

struct cme_inst_ring_list {
	uint16_t ring[4]; // In order: EC_REPR0, EC_REPR1, 2 reserved
	uint8_t payload[];
};

struct sgpe_cmn_ring_list {
	uint16_t ring[64]; // See the list in layout_cmn_rings_for_sgpe() skipping EQ_ANA_BNDY, 3 reserved
	uint8_t payload[];
};

struct sgpe_inst_ring_list {
	/* For each quad, in order:
	 * EQ_REPR0, EX0_L3_REPR, EX1_L3_REPR, EX0_L2_REPR, EX1_L2_REPR,
	 * EX0_L3_REFR_REPR, EX1_L3_REFR_REPR, EX0_L3_REFR_TIME,
	 * EX1_L3_REFR_TIME, 3 reserved. */
	uint16_t ring[MAX_QUADS_PER_CHIP][12];

	uint8_t payload[];
};

struct scom_entry_t {
	uint32_t hdr;
	uint32_t address;
	uint64_t data;
};

struct stop_cache_section_t {
	struct scom_entry_t non_cache_area[MAX_EQ_SCOM_ENTRIES];
	struct scom_entry_t l2_cache_area[MAX_L2_SCOM_ENTRIES];
	struct scom_entry_t l3_cache_area[MAX_L3_SCOM_ENTRIES];
};

enum scom_operation {
	SCOM_APPEND,
	SCOM_REPLACE
};

extern void mount_part_from_pnor(const char *part_name,
				 struct mmap_helper_region_device *mdev);

enum operation_type {
	COPY,
	FIND
};

static size_t copy_section(void *dst, struct xip_section *section, void *base,
                           uint8_t dd, enum operation_type op)
{
	if (!section->dd_support) {
		if (op == COPY) {
			memcpy(dst, base + section->offset, section->size);
		} else {
			*(void **)dst = base + section->offset;
		}
		return section->size;
	}

	struct dd_container *cont = base + section->offset;
	int i;

	assert(cont->magic == DD_CONTAINER_MAGIC);
	for (i = 0; i < cont->num; i++) {
		if (cont->blocks[i].dd == dd) {
			if (op == COPY) {
				memcpy(dst, (void *)cont + cont->blocks[i].offset,
				       cont->blocks[i].size);
			} else {
				*(void **)dst = (void *)cont + cont->blocks[i].offset;
			}
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
	size = copy_section(&homer->qpmr.sgpe.header, &sgpe->qpmr, sgpe, dd, COPY);
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
	                    sgpe, dd, COPY);
	homer->qpmr.sgpe.header.l1_offset = offsetof(struct qpmr_st,
	                                             sgpe.l1_bootloader);
	assert(size <= GPE_BOOTLOADER_SIZE);

	/* SPGE L2 bootloader */
	size = copy_section(&homer->qpmr.sgpe.l2_bootloader, &sgpe->l2_bootloader,
	                    sgpe, dd, COPY);
	homer->qpmr.sgpe.header.l2_offset = offsetof(struct qpmr_st,
	                                             sgpe.l2_bootloader);
	homer->qpmr.sgpe.header.l2_len = size;
	assert(size <= GPE_BOOTLOADER_SIZE);

	/* SPGE HCODE */
	size = copy_section(&homer->qpmr.sgpe.sram_image, &sgpe->hcode, sgpe, dd,
	                    COPY);
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
	 *
	 * This probably is full address and not offset.
	 */
	hdr->aux_offset = 0x80000000 + offsetof(struct homer_st, qpmr.aux);
	hdr->aux_len = CACHE_SCOM_AUX_SIZE;
	homer->qpmr.sgpe.header.enable_24x7_ima = 1;
	/* Here offset is relative to QPMR */
	homer->qpmr.sgpe.header.aux_offset = offsetof(struct qpmr_st, aux);
	homer->qpmr.sgpe.header.aux_len = CACHE_SCOM_AUX_SIZE;
}

static const uint32_t _SMF = 0x5F534D46; // "_SMF"

static const uint32_t ATTN_OP             = 0x00000200;
static const uint32_t BLR_OP              = 0x4E800020;
static const uint32_t ORI_OP              = 0x60000000;
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
		base[7] |= ((spr & 0x1F) << 16) | ((spr & 0x3E0) << 6);
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
	**ptr |= i;
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
                               struct xip_restore_header *rest, uint8_t dd,
                               uint64_t functional_cores)
{
	/* Assumptions: SMT4 only, SMF available but disabled. */
	size_t size;
	uint32_t *ptr;
	const uint64_t hrmor = read_spr(SPR_HRMOR);
	/* See cpu_winkle(), additionally set Hypervisor Doorbell Exit Enable */
	const uint64_t lpcr =
	   (read_spr(SPR_LPCR)
	    & ~(SPR_LPCR_EEE | SPR_LPCR_DEE | SPR_LPCR_OEE | SPR_LPCR_HDICE))
	   | (SPR_LPCR_HVICE | SPR_LPCR_HVEE | SPR_LPCR_HDEE);
	/*
	 * Timing facilities may be lost. During their restoration Large Decrementer
	 * in LPCR may be initially turned off, which may result in a spurious
	 * Decrementer Exception. Disable External Interrupts on self-restore, they
	 * will be re-enabled later by coreboot.
	 */
	const uint64_t msr = read_msr() & ~PPC_BIT(48);
	/* Clear en_attn for HID */
	const uint64_t hid = read_spr(SPR_HID) & ~PPC_BIT(3);

	/*
	 * Data in XIP has its first 256 bytes zeroed, reserved for header, so even
	 * though this is exe part of self restore region, we should copy it to
	 * header's address.
	 */
	size = copy_section(&homer->cpmr.header, &rest->self, rest, dd, COPY);
	assert(size > sizeof(struct cpmr_header));

	/* Now, overwrite header. */
	size = copy_section(&homer->cpmr.header, &rest->cpmr, rest, dd, COPY);
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
	for (size = 0; size < (192 * KiB) / sizeof(uint32_t); size++) {
		ptr[size] = ATTN_OP;
	}

	/*
	 * This loop combines three functions from hostboot:
	 * initSelfRestoreRegion(), initSelfSaveRestoreEntries() and
	 * applyHcodeGenCpuRegs(). There is inconsistency as for calling them for
	 * all cores vs only functional ones. As far as I can tell, cores are waken
	 * based on OCC CCSR register, so nonfunctional ones should be skipped and
	 * don't need any self-restore code.
	 */
	for (int core = 0; core < MAX_CORES_PER_CHIP; core++) {
		/*
		 * TODO: test if we can skip both cpureg and save_self for nonfunctional
		 * cores
		 */
		if (!IS_EC_FUNCTIONAL(core, functional_cores))
			continue;

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
				 * SMFCTRL through USPRG1:  key = 1C..1E
				 */
				int tsa_key = i;
				if (i > 7)
					tsa_key += 0x14;

				if (thread_sprs[i] == SPR_LPCR) {
					add_init_cpureg_entry(csr->thread_restore_area[thread],
					                      thread_sprs[i], lpcr, 0);
				} else if (thread_sprs[i] == SPR_MSR && thread == 0) {
					/* One MSR per core, restored last so must (?) be here */
					add_init_cpureg_entry(csr->thread_restore_area[thread],
					                      thread_sprs[i], msr, 0);
				} else {
					add_init_cpureg_entry(csr->thread_restore_area[thread],
					                      thread_sprs[i], 0, 1);
				}
				add_init_save_self_entry(&tsa, tsa_key);
			}

			*tsa++ = MTLR_R30_OP;
			*tsa++ = BLR_OP;
		}

		csr->core_restore_area[0] = BLR_OP;
		*csa++ = MFLR_R30_OP;
		for (int i = 0; i < ARRAY_SIZE(core_sprs); i++) {
			if (core_sprs[i] == SPR_HRMOR || core_sprs[i] == SPR_URMOR) {
				add_init_cpureg_entry(csr->core_restore_area, core_sprs[i],
				                      hrmor, 0);
			} else if (core_sprs[i] == SPR_HID) {
				add_init_cpureg_entry(csr->core_restore_area, core_sprs[i],
				                      hid, 0);
			} else {
				add_init_cpureg_entry(csr->core_restore_area, core_sprs[i],
				                      0, 1);
			}
			/*
			 * HID through PTCR: key = 0x15..0x18
			 * HRMOR and URMOR are skipped.
			 */
			if (core_sprs[i] != SPR_HRMOR && core_sprs[i] != SPR_URMOR)
				add_init_save_self_entry(&csa, i + 0x14);
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

	size = copy_section(&homer->cpmr.cme_sram_region, &cme->hcode, cme, dd,
	                    COPY);
	assert(size <= CME_SRAM_IMG_SIZE);
	assert(size > (INT_VECTOR_SIZE + sizeof(struct cme_img_header)));

	hdr = (struct cme_img_header *)
	      &homer->cpmr.cme_sram_region[INT_VECTOR_SIZE];

	hdr->hcode_offset = 0;
	hdr->hcode_len = size;

	hdr->pstate_region_offset = 0;
	hdr->pstate_region_len = 0;

	hdr->cpmr_phy_addr = (uint64_t) homer | 2 * MiB;
	/* With SMF disabled unsecure HOMER is the same as regular one */
	hdr->unsec_cpmr_phy_addr = hdr->cpmr_phy_addr;

	hdr->common_ring_offset = hdr->hcode_offset + hdr->hcode_len;
	hdr->common_ring_len = 0;

	hdr->scom_offset = 0;
	hdr->scom_len = CORE_SCOM_RESTORE_SIZE / MAX_CORES_PER_CHIP / 2;

	hdr->core_spec_ring_offset = 0;
	hdr->max_spec_ring_len = 0;
}

static void build_pgpe(struct homer_st *homer, struct xip_pgpe_header *pgpe,
                       uint8_t dd)
{
	size_t size;
	struct pgpe_img_header *hdr;

	/* PPGE header */
	size = copy_section(&homer->ppmr.header, &pgpe->ppmr, pgpe, dd, COPY);
	assert(size <= PPMR_HEADER_SIZE);
	/*
	 * 0xFFF00000 (SRAM base) + 4k (IPC) + 60k (GPE0) + 64k (GPE1) = 0xFFF20000
	 *
	 * WARNING: I have no idea if this is constant or depends on PPGE version.
	 */
	assert(homer->ppmr.header.sram_region_start == 0xFFF20000);
	assert(homer->ppmr.header.sram_region_size == PGPE_SRAM_IMG_SIZE +
	                                              PGPE_AUX_TASK_SIZE +
	                                              PGPE_OCC_SHARED_SRAM_SIZE);

	/* PPGE L1 bootloader */
	size = copy_section(homer->ppmr.l1_bootloader, &pgpe->l1_bootloader, pgpe,
	                    dd, COPY);
	assert(size <= GPE_BOOTLOADER_SIZE);
	homer->ppmr.header.l1_offset = offsetof(struct ppmr_st, l1_bootloader);

	/* PPGE L2 bootloader */
	size = copy_section(homer->ppmr.l2_bootloader, &pgpe->l2_bootloader, pgpe,
	                    dd, COPY);
	assert(size <= GPE_BOOTLOADER_SIZE);
	homer->ppmr.header.l2_offset = offsetof(struct ppmr_st, l2_bootloader);
	homer->ppmr.header.l2_len = size;

	/* PPGE HCODE */
	size = copy_section(homer->ppmr.pgpe_sram_img, &pgpe->hcode, pgpe, dd,
	                    COPY);
	assert(size <= PGPE_SRAM_IMG_SIZE);
	assert(size > (INT_VECTOR_SIZE + sizeof(struct pgpe_img_header)));
	homer->ppmr.header.hcode_offset = offsetof(struct ppmr_st, pgpe_sram_img);
	homer->ppmr.header.hcode_len = size;

	/* PPGE auxiliary task */
	size = copy_section(homer->ppmr.aux_task, &pgpe->aux_task, pgpe, dd, COPY);
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

static uint64_t get_available_cores(int *me)
{
	uint64_t ret = 0;
	for (int i = 0; i < MAX_CORES_PER_CHIP; i++) {
		uint64_t val = read_scom_for_chiplet(EC00_CHIPLET_ID + i, 0xF0040);
		if (val & PPC_BIT(0)) {
			printk(BIOS_SPEW, "Core %d is functional%s\n", i,
			       (val & PPC_BIT(1)) ? "" : " and running");
			ret |= PPC_BIT(i);
			if (((val & PPC_BIT(1)) == 0) && me != NULL)
				*me = i;

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

/* TODO: similar is used in 13.3. Add missing parameters and make it public? */
static void psu_command(uint8_t flags, long time)
{
	/* TP.TPCHIP.PIB.PSU.PSU_SBE_DOORBELL_REG */
	if (read_scom(0x000D0060) & PPC_BIT(0))
		die("MBOX to SBE busy, this should not happen\n");

	if (read_scom(0x000D0063) & PPC_BIT(0)) {
		printk(BIOS_ERR, "SBE to Host doorbell already active, clearing it\n");
		write_scom(0x000D0064, ~PPC_BIT(0));
	}

	/* https://github.com/open-power/hostboot/blob/master/src/include/usr/sbeio/sbe_psudd.H#L418 */
	/* TP.TPCHIP.PIB.PSU.PSU_HOST_SBE_MBOX0_REG */
	/* REQUIRE_RESPONSE, CLASS_CORE_STATE, CMD_CONTROL_DEADMAN_LOOP, flags */
	write_scom(0x000D0050, 0x000001000000D101 | PPC_SHIFT(flags, 31));

	/* TP.TPCHIP.PIB.PSU.PSU_HOST_SBE_MBOX0_REG */
	write_scom(0x000D0051, time);

	/* Ring the host->SBE doorbell */
	/* TP.TPCHIP.PIB.PSU.PSU_SBE_DOORBELL_REG_OR */
	write_scom(0x000D0062, PPC_BIT(0));

	/* Wait for response */
	/* TP.TPCHIP.PIB.PSU.PSU_HOST_DOORBELL_REG */
	time = wait_ms(time, read_scom(0x000D0063) & PPC_BIT(0));

	if (!time)
		die("Timed out while waiting for SBE response\n");

	/* Clear SBE->host doorbell */
	/* TP.TPCHIP.PIB.PSU.PSU_HOST_DOORBELL_REG_AND */
	write_scom(0x000D0064, ~PPC_BIT(0));
}

#define DEADMAN_LOOP_START	0x0001
#define DEADMAN_LOOP_STOP	0x0002

static void block_wakeup_int(int core, int state)
{
	// TP.TPCHIP.NET.PCBSLEC14.PPMC.PPM_COMMON_REGS.PPM_GPMMR		// 0x200F0100
	/* Depending on requested state we write to SCOM1 (CLEAR) or SCOM2 (OR). */
	uint64_t scom = state ? 0x200F0102 : 0x200F0101;

	write_scom_for_chiplet(EC00_CHIPLET_ID + core, 0x200F0108, PPC_BIT(1));
	/* Register is documented, but its bits are reserved... */
	write_scom_for_chiplet(EC00_CHIPLET_ID + core, scom, PPC_BIT(6));

	write_scom_for_chiplet(EC00_CHIPLET_ID + core, 0x200F0107, PPC_BIT(1));
}

/*
 * Some time will be lost between entering and exiting STOP 15, but we don't
 * have a way of calculating it. In theory we could read tick count from one of
 * the auxiliary chips (SBE, SGPE), but accessing those and converting to the
 * frequency of TB may take longer than sleep took.
 */
struct save_state {
	uint64_t r1;	/* stack */
	uint64_t r2;	/* TOC */
	uint64_t msr;
	uint64_t nia;
	uint64_t tb;
	uint64_t lr;
} sstate;

static void cpu_winkle(void)
{
	uint64_t lpcr = read_spr(SPR_LPCR);
	/*
	 * Clear {External, Decrementer, Other} Exit Enable and Hypervisor
	 * Decrementer Interrupt Conditionally Enable
	 */
	lpcr &= ~(SPR_LPCR_EEE | SPR_LPCR_DEE | SPR_LPCR_OEE | SPR_LPCR_HDICE);
	/*
	 * Set Hypervisor Virtualization Interrupt Conditionally Enable
	 * and Hypervisor Virtualization Exit Enable
	 */
	lpcr |= SPR_LPCR_HVICE | SPR_LPCR_HVEE;
	write_spr(SPR_LPCR, lpcr);
	write_spr(SPR_PSSCR, 0x00000000003F00FF);
	sstate.msr = read_msr();

	/*
	 * Cannot clobber:
	 * - r1 (stack) - reloaded from sstate
	 * - r2 (TOC aka PIC register) - reloaded from sstate
	 * - r3 (address of sstate) - storage duration limited to block below
	 */
	{
		register void *r3 asm ("r3") = &sstate;
		asm volatile("std      1, 0(%0)\n"
		             "std      2, 8(%0)\n"
		             "mflr     1\n"
		             "std      1, 40(%0)\n"
		             "lnia     1\n"
		             "__tmp_nia:"
		             "addi     1, 1, wakey - __tmp_nia\n"
		             "std      1, 24(%0)\n"
		             "mftb     1\n"
		             "std      1, 32(%0)\n"  /* TB - save as late as possible */
		             "sync\n"
		             "stop\n"
		             "wakey:\n"
		             : "+r"(r3) ::
		             "r0", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11",
		             "r12", "r13", "r14", "r15", "r16", "r17", "r18", "r19",
		             "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27",
		             "r28", "r29", "r30", "r31", "memory", "cc");
	}

	/*
	 * Hostboot restored two additional registers at this point: LPCR and PSSCR.
	 *
	 * LPCR was restored from core self-restore region, coreboot won't need to
	 * change it unless it does delays using decrementer, as it did in romstage.
	 *
	 * PSSCR won't be used before next 'stop' instruction, which won't happen
	 * before new settings are written by the payload.
	 */

	/*
	 * Timing facilities were lost, this includes DEC register. Because during
	 * self-restore Large Decrementer was disabled for few instructions, value
	 * of DEC is trimmed to 32 bits. Restore it to something bigger, otherwise
	 * interrupt would arrive in ~4 seconds.
	 */
	write_spr(SPR_DEC, SPR_DEC_LONGEST_TIME);
}

static void istep_16_1(int this_core)
{
	report_istep(16,1);
	/*
	 * Wait time 10.5 sec, anything larger than 10737 ms can cause overflow on
	 * SBE side of the timeout calculations.
	 */
	long time = 10500;

	/*
	 * Debugging aid - 0xE40 is Hypervisor Emulation Assistance vector. It is
	 * taken when processor tries to execute unimplemented instruction. All 0s
	 * is (and will always be) such an instruction, meaning we will get here
	 * when processor jumps into uninitialized memory. If this instruction were
	 * also uninitialized, processor would hit another exception and again jump
	 * here. This time, however, it would overwrite original HSRR0 value with
	 * 0xE40. Instruction below is 'b .'. This way HSRR0 will retain its value
	 * - address of instruction which generated this exception. It can be then
	 * read with pdbg.
	 */
	*(volatile uint32_t *)0xE40 = 0x48000000;

	configure_xive(this_core);

	printk(BIOS_ERR, "XIVE configured, enabling External Interrupt\n");
	write_msr(read_msr() | PPC_BIT(48));

	/*
	 * This will request SBE to wake us up after we enter STOP 15. Hopefully
	 * we will come back to the place where we were before.
	 */
	printk(BIOS_ERR, "Entering dead man loop\n");
	psu_command(DEADMAN_LOOP_START, time);

	/* REMOVE ME! */
	/*
	 * This masks FIR bit that is set by SBE when dead man timer expires, which
	 * results in checkstop. To help with debugging it is masked here, but it
	 * must not be in the final code!
	 */
	//write_scom(0x0504000F, PPC_BIT(32));

	block_wakeup_int(this_core, 1);

	cpu_winkle();

	/*
	 * SBE sets this doorbell bit when it finishes its part of STOP 15 wakeup.
	 * No need to handle the timeout, if it happens, SBE will checkstop the
	 * system anyway.
	 */
	wait_us(time, read_scom(0x000D0063) & PPC_BIT(2));

	write_scom(0x000D0064, ~PPC_BIT(2));

	/*
	 * This tells SBE that we were properly awoken. Hostboot uses default
	 * timeout of 90 seconds, but if SBE doesn't answer in 10 there is no reason
	 * to believe it will answer at all.
	 */
	psu_command(DEADMAN_LOOP_STOP, time);

	// core_checkstop_helper_hwp(..., true)
	//     p9_core_checkstop_handler(___, true)
	// core_checkstop_helper_homer()
	//     p9_stop_save_scom() and others
}

static void get_ppe_scan_rings(struct xip_hw_header *hw, uint8_t dd,
			       enum ppe_type ppe, struct ring_data *ring_data)
{
	const uint32_t max_rings_buf_size = ring_data->rings_buf_size;

	struct tor_hdr *rings;
	struct tor_hdr *overlays;

	if (dd < 0x20)
		die("DD must be at least 0x20!");
	if (!hw->overlays.dd_support)
		die("Overlays must support DD!");

	copy_section(&rings, &hw->rings, hw, dd, FIND);
	copy_section(&overlays, &hw->overlays, hw, dd, FIND);

	if (!tor_access_ring(rings, UNDEFINED_RING_ID, ppe, UNDEFINED_RING_VARIANT,
			     UNDEFINED_INSTANCE_ID, ring_data->rings_buf,
			     &ring_data->rings_buf_size, GET_PPE_LEVEL_RINGS))
		die("Failed to access PPE level rings!");

	assert(ring_data->work_buf1_size == MAX_RING_BUF_SIZE);
	assert(ring_data->work_buf2_size == MAX_RING_BUF_SIZE);
	assert(ring_data->work_buf3_size == MAX_RING_BUF_SIZE);

	tor_fetch_and_insert_vpd_rings((struct tor_hdr *)ring_data->rings_buf,
				       &ring_data->rings_buf_size, max_rings_buf_size,
				       overlays, ppe,
				       ring_data->work_buf1,
				       ring_data->work_buf2,
				       ring_data->work_buf3);
}

static void layout_cmn_rings_for_cme(struct homer_st *homer,
				     struct ring_data *ring_data,
				     uint8_t ring_variant, uint32_t *ring_len)
{
	struct cme_cmn_ring_list *tmp =
		(void *)&homer->cpmr.cme_sram_region[*ring_len];
	uint8_t *start = (void *)tmp;
	uint8_t *payload = tmp->payload;

	uint32_t i = 0;
	const enum ring_id ring_ids[] = { EC_FUNC, EC_GPTR, EC_TIME, EC_MODE };

	for (i = 0; i < sizeof(ring_ids) / sizeof(ring_ids[0]); ++i) {
		const enum ring_id id = ring_ids[i];

		uint32_t ring_size = MAX_RING_BUF_SIZE;
		uint8_t *ring_dst = start + ALIGN_UP(payload - start, 8);

		uint8_t this_ring_variant = ring_variant;
		if (id == EC_GPTR || id == EC_TIME)
			this_ring_variant = RV_BASE;

		if (!tor_access_ring(ring_data->rings_buf, id, PT_CME,
				     this_ring_variant, EC00_CHIPLET_ID,
				     ring_dst, &ring_size, GET_RING_DATA))
			continue;

		tmp->ring[i] = ring_dst - start;
		payload = ring_dst + ALIGN_UP(ring_size, 8);
	}

	if (payload != tmp->payload)
		*ring_len += payload - start;

	*ring_len = ALIGN_UP(*ring_len, 8);
}

static void layout_inst_rings_for_cme(struct homer_st *homer,
				      struct ring_data *ring_data,
				      uint64_t cores,
				      uint8_t ring_variant, uint32_t *ring_len)
{
	uint32_t max_ex_len = 0;

	uint32_t ex = 0;

	for (ex = 0; ex < MAX_CMES_PER_CHIP; ++ex) {
		uint32_t i = 0;
		uint32_t ex_len = 0;

		for (i = 0; i < MAX_CORES_PER_EX; ++i) {
			const uint32_t core = ex * MAX_CORES_PER_EX + i;

			uint32_t ring_size = 0;

			if (!IS_EC_FUNCTIONAL(core, cores))
				continue;

			ring_size = ring_data->work_buf1_size;
			if (!tor_access_ring(ring_data->rings_buf, EC_REPR,
					     PT_CME, RV_BASE,
					     EC00_CHIPLET_ID + core,
					     ring_data->work_buf1,
					     &ring_size, GET_RING_DATA))
			    continue;

			ex_len += ALIGN_UP(ring_size, 8);
		}

		if (ex_len > max_ex_len)
			max_ex_len = ex_len;
	}

	if (max_ex_len > 0) {
		max_ex_len += sizeof(struct cme_inst_ring_list);
		max_ex_len = ALIGN_UP(max_ex_len, 32);
	}

	for (ex = 0; ex < MAX_CMES_PER_CHIP; ++ex) {
		const uint32_t ex_offset =
			ex * (max_ex_len + ALIGN_UP(sizeof(LocalPstateParmBlock), 32));

		uint8_t *start = &homer->cpmr.cme_sram_region[*ring_len + ex_offset];
		struct cme_inst_ring_list *tmp = (void *)start;
		uint8_t *payload = tmp->payload;

		uint32_t i = 0;

		for (i = 0; i < MAX_CORES_PER_EX; ++i) {
			const uint32_t core = ex * MAX_CORES_PER_EX + i;

			uint32_t ring_size = MAX_RING_BUF_SIZE;

			if (!IS_EC_FUNCTIONAL(core, cores))
				continue;

			if ((payload - start) % 8 != 0)
				payload = start + ALIGN_UP(payload - start, 8);

			if (!tor_access_ring(ring_data->rings_buf, EC_REPR,
					     PT_CME, RV_BASE,
					     EC00_CHIPLET_ID + core,
					     payload,
					     &ring_size, GET_RING_DATA))
			    continue;

			tmp->ring[i] = payload - start;
			payload += ALIGN_UP(ring_size, 8);
		}
	}

	*ring_len = max_ex_len;
}

static void layout_rings_for_cme(struct homer_st *homer,
				 struct ring_data *ring_data,
				 uint64_t cores, uint8_t ring_variant)
{
	struct cpmr_header *cpmr_hdr = &homer->cpmr.header;
	struct cme_img_header *cme_hdr =
		(void *)&homer->cpmr.cme_sram_region[INT_VECTOR_SIZE];

	uint32_t ring_len = cme_hdr->hcode_offset + cme_hdr->hcode_len;

	assert(be64toh(cpmr_hdr->magic) == CPMR_VDM_PER_QUAD);

	layout_cmn_rings_for_cme(homer, ring_data, ring_variant, &ring_len);

	cme_hdr->common_ring_len = ring_len - (cme_hdr->hcode_offset + cme_hdr->hcode_len);

	// if common ring is empty, force offset to be 0
	if (cme_hdr->common_ring_len == 0)
		cme_hdr->common_ring_offset = 0;

	ring_len = ALIGN_UP(ring_len, 32);

	layout_inst_rings_for_cme(homer, ring_data, cores, RV_BASE, &ring_len);

	if (ring_len != 0) {
		cme_hdr->max_spec_ring_len = ALIGN_UP(ring_len, 32) / 32;
		cme_hdr->core_spec_ring_offset =
			ALIGN_UP(cme_hdr->common_ring_offset + cme_hdr->common_ring_len, 32) / 32;
	}
}

static enum ring_id resolve_eq_inex_bucket(void)
{
	switch (powerbus_cfg()->core_floor_ratio) {
		case FABRIC_CORE_FLOOR_RATIO_RATIO_8_8:
			return EQ_INEX_BUCKET_4;

		case FABRIC_CORE_FLOOR_RATIO_RATIO_7_8:
		case FABRIC_CORE_FLOOR_RATIO_RATIO_6_8:
		case FABRIC_CORE_FLOOR_RATIO_RATIO_5_8:
		case FABRIC_CORE_FLOOR_RATIO_RATIO_4_8:
			return EQ_INEX_BUCKET_3;

		case FABRIC_CORE_FLOOR_RATIO_RATIO_2_8:
			return EQ_INEX_BUCKET_2;
	}

	die("Failed to resolve EQ_INEX_BUCKET_*!\n");
}

static void layout_cmn_rings_for_sgpe(struct homer_st *homer,
				      struct ring_data *ring_data,
				      uint8_t ring_variant)
{
	const enum ring_id ring_ids[] = {
		EQ_FURE, EQ_GPTR, EQ_TIME, EQ_INEX, EX_L3_FURE, EX_L3_GPTR, EX_L3_TIME,
		EX_L2_MODE, EX_L2_FURE, EX_L2_GPTR, EX_L2_TIME, EX_L3_REFR_FURE,
		EX_L3_REFR_GPTR, EQ_ANA_FUNC, EQ_ANA_GPTR, EQ_DPLL_FUNC, EQ_DPLL_GPTR,
		EQ_DPLL_MODE, EQ_ANA_BNDY_BUCKET_0, EQ_ANA_BNDY_BUCKET_1,
		EQ_ANA_BNDY_BUCKET_2, EQ_ANA_BNDY_BUCKET_3, EQ_ANA_BNDY_BUCKET_4,
		EQ_ANA_BNDY_BUCKET_5, EQ_ANA_BNDY_BUCKET_6, EQ_ANA_BNDY_BUCKET_7,
		EQ_ANA_BNDY_BUCKET_8, EQ_ANA_BNDY_BUCKET_9, EQ_ANA_BNDY_BUCKET_10,
		EQ_ANA_BNDY_BUCKET_11, EQ_ANA_BNDY_BUCKET_12, EQ_ANA_BNDY_BUCKET_13,
		EQ_ANA_BNDY_BUCKET_14, EQ_ANA_BNDY_BUCKET_15, EQ_ANA_BNDY_BUCKET_16,
		EQ_ANA_BNDY_BUCKET_17, EQ_ANA_BNDY_BUCKET_18, EQ_ANA_BNDY_BUCKET_19,
		EQ_ANA_BNDY_BUCKET_20, EQ_ANA_BNDY_BUCKET_21, EQ_ANA_BNDY_BUCKET_22,
		EQ_ANA_BNDY_BUCKET_23, EQ_ANA_BNDY_BUCKET_24, EQ_ANA_BNDY_BUCKET_25,
		EQ_ANA_BNDY_BUCKET_L3DCC, EQ_ANA_MODE, EQ_ANA_BNDY_BUCKET_26,
		EQ_ANA_BNDY_BUCKET_27, EQ_ANA_BNDY_BUCKET_28, EQ_ANA_BNDY_BUCKET_29,
		EQ_ANA_BNDY_BUCKET_30, EQ_ANA_BNDY_BUCKET_31, EQ_ANA_BNDY_BUCKET_32,
		EQ_ANA_BNDY_BUCKET_33, EQ_ANA_BNDY_BUCKET_34, EQ_ANA_BNDY_BUCKET_35,
		EQ_ANA_BNDY_BUCKET_36, EQ_ANA_BNDY_BUCKET_37, EQ_ANA_BNDY_BUCKET_38,
		EQ_ANA_BNDY_BUCKET_39, EQ_ANA_BNDY_BUCKET_40, EQ_ANA_BNDY_BUCKET_41
	};

	const enum ring_id eq_index_bucket_id = resolve_eq_inex_bucket();

	struct qpmr_header *qpmr_hdr = &homer->qpmr.sgpe.header;
	struct sgpe_cmn_ring_list *tmp =
		(void *)&homer->qpmr.sgpe.sram_image[qpmr_hdr->img_len];
	uint8_t *start = (void *)tmp;
	uint8_t *payload = tmp->payload;

	uint32_t i = 0;

	for (i = 0; i < sizeof(ring_ids) / sizeof(ring_ids[0]); ++i) {
		uint8_t this_ring_variant;
		uint32_t ring_size = MAX_RING_BUF_SIZE;

		enum ring_id id = ring_ids[i];
		if (id == EQ_INEX)
			id = eq_index_bucket_id;

		this_ring_variant = ring_variant;
		if (id == EQ_GPTR         || // EQ GPTR
		    id == EQ_ANA_GPTR     ||
		    id == EQ_DPLL_GPTR    ||
		    id == EX_L3_GPTR      || // EX GPTR
		    id == EX_L2_GPTR      ||
		    id == EX_L3_REFR_GPTR ||
		    id == EQ_TIME         || // EQ TIME
		    id == EX_L3_TIME      || // EX TIME
		    id == EX_L2_TIME)
			this_ring_variant = RV_BASE;

		if ((payload - start) % 8 != 0)
			payload = start + ALIGN_UP(payload - start, 8);

		if (!tor_access_ring(ring_data->rings_buf, id, PT_SGPE,
				     this_ring_variant, EP00_CHIPLET_ID,
				     payload, &ring_size, GET_RING_DATA))
			continue;

		tmp->ring[i] = payload - start;
		payload += ALIGN_UP(ring_size, 8);
	}

	qpmr_hdr->common_ring_len = payload - start;
	qpmr_hdr->common_ring_offset =
		offsetof(struct qpmr_st, sgpe.sram_image) + qpmr_hdr->img_len;
}

static void layout_inst_rings_for_sgpe(struct homer_st *homer,
				       struct ring_data *ring_data,
				       uint64_t cores, uint32_t ring_variant)
{
	struct qpmr_header *qpmr_hdr = &homer->qpmr.sgpe.header;
	uint32_t inst_rings_offset = qpmr_hdr->img_len + qpmr_hdr->common_ring_len;

	uint8_t *start = &homer->qpmr.sgpe.sram_image[inst_rings_offset];
	struct sgpe_inst_ring_list *tmp = (void *)start;
	uint8_t *payload = tmp->payload;

	/* It's EQ_REPR and three pairs of EX rings */
	const enum ring_id ring_ids[] = {
		EQ_REPR, EX_L3_REPR, EX_L3_REPR, EX_L2_REPR, EX_L2_REPR,
		EX_L3_REFR_REPR, EX_L3_REFR_REPR, EX_L3_REFR_TIME,
		EX_L3_REFR_TIME
	};

	uint8_t quad = 0;

	for (quad = 0; quad < MAX_QUADS_PER_CHIP; ++quad) {
		uint8_t i = 0;

		/* Skip non-functional quads */
		if (!IS_EQ_FUNCTIONAL(quad, cores))
		    continue;

		for (i = 0; i < sizeof(ring_ids) / sizeof(ring_ids[0]); ++i) {
			const enum ring_id id = ring_ids[i];

			uint32_t ring_size = MAX_RING_BUF_SIZE;

			/* Despite the constant, this is not an SCOM chiplet ID,
			 * it's just used as a base value */
			uint8_t instance_id = EP00_CHIPLET_ID + quad;
			if (i != 0) {
				instance_id += quad;
				if (i % 2 == 0)
					++instance_id;
			}

			if ((payload - start) % 8 != 0)
				payload = start + ALIGN_UP(payload - start, 8);

			if (!tor_access_ring(ring_data->rings_buf, id, PT_SGPE,
					     ring_variant, instance_id, payload,
					     &ring_size, GET_RING_DATA))
				continue;

			tmp->ring[quad][i] = payload - start;
			payload += ALIGN_UP(ring_size, 8);
		}
	}

	qpmr_hdr->spec_ring_offset = qpmr_hdr->common_ring_offset + qpmr_hdr->common_ring_len;
	qpmr_hdr->spec_ring_len = payload - start;
}

static void layout_rings_for_sgpe(struct homer_st *homer,
				  struct ring_data *ring_data,
				  struct xip_sgpe_header *sgpe,
				  uint64_t cores, uint8_t ring_variant)
{
	struct qpmr_header *qpmr_hdr = &homer->qpmr.sgpe.header;
	struct sgpe_img_header *sgpe_img_hdr =
		(void *)&homer->qpmr.sgpe.sram_image[INT_VECTOR_SIZE];

	layout_cmn_rings_for_sgpe(homer, ring_data, ring_variant);
	layout_inst_rings_for_sgpe(homer, ring_data, cores, RV_BASE);

	if (qpmr_hdr->common_ring_len == 0)
		/* If quad common rings don't exist, ensure it's offset in image
		 * header is zero */
		sgpe_img_hdr->cmn_ring_occ_offset = 0;

	if (qpmr_hdr->spec_ring_len > 0) {
		sgpe_img_hdr->spec_ring_occ_offset = qpmr_hdr->img_len + qpmr_hdr->common_ring_len;
		sgpe_img_hdr->scom_offset = sgpe_img_hdr->spec_ring_occ_offset + qpmr_hdr->spec_ring_len;
	}
}

static void stop_save_scom(struct homer_st *homer, uint32_t scom_address,
			   uint64_t scom_data, enum scom_section section,
			   enum scom_operation operation)
{
	enum {
		STOP_API_VER = 0x00,
		SCOM_ENTRY_START = 0xDEADDEAD,
	};

	chiplet_id_t chiplet_id = (scom_address >> 24) & 0x3f;
	uint32_t max_scom_restore_entries = 0;
	struct stop_cache_section_t *stop_cache_scom = NULL;
	struct scom_entry_t *scom_entry = NULL;
	struct scom_entry_t *nop_entry = NULL;
	struct scom_entry_t *matching_entry = NULL;
	struct scom_entry_t *end_entry = NULL;
	struct scom_entry_t *entry = NULL;
	uint32_t entry_limit = 0;

	if (chiplet_id >= EC00_CHIPLET_ID) {
		uint32_t offset = (chiplet_id - EC00_CHIPLET_ID)*CORE_SCOM_RESTORE_SIZE_PER_CORE;
		scom_entry = (struct scom_entry_t *)&homer->cpmr.core_scom[offset];
		max_scom_restore_entries = homer->cpmr.header.core_max_scom_entry;
	} else {
		uint32_t offset = (chiplet_id - EP00_CHIPLET_ID)*QUAD_SCOM_RESTORE_SIZE_PER_QUAD;
		stop_cache_scom =
			(struct stop_cache_section_t *)&homer->qpmr.cache_scom_region[offset];
		max_scom_restore_entries = homer->qpmr.sgpe.header.max_quad_restore_entry;
	}

	if (stop_cache_scom == NULL)
		die("Failed to prepare for updating STOP SCOM\n");

	switch (section) {
		case STOP_SECTION_CORE_SCOM:
			entry_limit = max_scom_restore_entries;
			break;
		case STOP_SECTION_EQ_SCOM:
			scom_entry = stop_cache_scom->non_cache_area;
			entry_limit = MAX_EQ_SCOM_ENTRIES;
			break;
		default:
			die("Unhandled STOP image section.\n");
			break;
	}

	for (uint32_t i = 0; i < entry_limit; ++i) {
		uint32_t entry_address = scom_entry[i].address;
		uint32_t entry_hdr = scom_entry[i].hdr;

		if (entry_address == scom_address && matching_entry == NULL)
			matching_entry = &scom_entry[i];

		if ((entry_address == ORI_OP || entry_address == ATTN_OP ||
		     entry_address == BLR_OP) && nop_entry == NULL)
			nop_entry = &scom_entry[i];

		/* If entry is either 0xDEADDEAD or has SCOM entry limit in LSB of its header,
		 * the place is already occupied */
		if (entry_hdr == SCOM_ENTRY_START || (entry_hdr & 0x000000ff))
			continue;

		end_entry = &scom_entry[i];
		break;
	}

	if (matching_entry == NULL && end_entry == NULL)
		die("Failed to find SCOM entry in STOP image.\n");

	entry = end_entry;
	if (operation == SCOM_APPEND && nop_entry != NULL)
		entry = nop_entry;
	else if (operation == SCOM_REPLACE && matching_entry != NULL)
		entry = matching_entry;

	if (entry == NULL)
		die("Failed to insert SCOM entry in STOP image.\n");

	entry->hdr = (0x000000ff & max_scom_restore_entries)
		   | ((STOP_API_VER & 0x7) << 28);
	entry->address = scom_address;
	entry->data = scom_data;
}

static void populate_epsilon_l2_scom_reg(struct homer_st *homer)
{
	const struct powerbus_cfg *pb_cfg = powerbus_cfg();

	uint32_t eps_r_t0 = pb_cfg->eps_r[0] / 8 / L2_EPS_DIVIDER + 1;
	uint32_t eps_r_t1 = pb_cfg->eps_r[1] / 8 / L2_EPS_DIVIDER + 1;
	uint32_t eps_r_t2 = pb_cfg->eps_r[2] / 8 / L2_EPS_DIVIDER + 1;

	uint32_t eps_w_t0 = pb_cfg->eps_w[0] / 8 / L2_EPS_DIVIDER + 1;
	uint32_t eps_w_t1 = pb_cfg->eps_w[1] / 8 / L2_EPS_DIVIDER + 1;

	uint64_t eps_r = PPC_PLACE(eps_r_t0, 0, 12)
		       | PPC_PLACE(eps_r_t1, 12, 12)
		       | PPC_PLACE(eps_r_t2, 24, 12);

	uint64_t eps_w = PPC_PLACE(eps_w_t0, 0, 12)
		       | PPC_PLACE(eps_w_t1, 12, 12)
		       | PPC_PLACE(L2_EPS_DIVIDER, 24, 4);

	uint8_t quad = 0;

	for (quad = 0; quad < MAX_QUADS_PER_CHIP; ++quad) {
		uint32_t scom_addr;

		/* Create restore entry for epsilon L2 RD register */

		scom_addr = (EX_L2_RD_EPS_REG | (quad << QUAD_BIT_POS));
		stop_save_scom(homer, scom_addr, eps_r, STOP_SECTION_EQ_SCOM,
			       SCOM_APPEND);

		scom_addr |= ODD_EVEN_EX_POS;
		stop_save_scom(homer, scom_addr, eps_r, STOP_SECTION_EQ_SCOM,
			       SCOM_APPEND);

		/* Create restore entry for epsilon L2 WR register */

		scom_addr = (EX_L2_WR_EPS_REG | (quad << QUAD_BIT_POS));
		stop_save_scom(homer, scom_addr, eps_w, STOP_SECTION_EQ_SCOM,
			       SCOM_APPEND);

		scom_addr |= ODD_EVEN_EX_POS;
		stop_save_scom(homer, scom_addr, eps_w, STOP_SECTION_EQ_SCOM,
			       SCOM_APPEND);
	}
}

static void populate_epsilon_l3_scom_reg(struct homer_st *homer)
{
	const struct powerbus_cfg *pb_cfg = powerbus_cfg();

	uint32_t eps_r_t0 = pb_cfg->eps_r[0] / 8 / L3_EPS_DIVIDER + 1;
	uint32_t eps_r_t1 = pb_cfg->eps_r[1] / 8 / L3_EPS_DIVIDER + 1;
	uint32_t eps_r_t2 = pb_cfg->eps_r[2] / 8 / L3_EPS_DIVIDER + 1;

	uint32_t eps_w_t0 = pb_cfg->eps_w[0] / 8 / L3_EPS_DIVIDER + 1;
	uint32_t eps_w_t1 = pb_cfg->eps_w[1] / 8 / L3_EPS_DIVIDER + 1;

	uint64_t eps_r = PPC_PLACE(eps_r_t0, 0, 12)
		       | PPC_PLACE(eps_r_t1, 12, 12)
		       | PPC_PLACE(eps_r_t2, 24, 12);

	uint64_t eps_w = PPC_PLACE(eps_w_t0, 0, 12)
		       | PPC_PLACE(eps_w_t1, 12, 12)
		       | PPC_PLACE(L2_EPS_DIVIDER, 30, 4);

	uint8_t quad = 0;

	for (quad = 0; quad < MAX_QUADS_PER_CHIP; ++quad) {
		uint32_t scom_addr;

		/* Create restore entry for epsilon L2 RD register */

		scom_addr = (EX_L3_RD_EPS_REG | (quad << QUAD_BIT_POS));
		stop_save_scom(homer, scom_addr, eps_r, STOP_SECTION_EQ_SCOM,
			       SCOM_APPEND);

		scom_addr |= ODD_EVEN_EX_POS;
		stop_save_scom(homer, scom_addr, eps_r, STOP_SECTION_EQ_SCOM,
			       SCOM_APPEND);

		/* Create restore entry for epsilon L2 WR register */

		scom_addr = (EX_L3_WR_EPS_REG | (quad << QUAD_BIT_POS));
		stop_save_scom(homer, scom_addr, eps_w, STOP_SECTION_EQ_SCOM,
			       SCOM_APPEND);

		scom_addr |= ODD_EVEN_EX_POS;
		stop_save_scom(homer, scom_addr, eps_w, STOP_SECTION_EQ_SCOM,
			       SCOM_APPEND);
	}
}

static void populate_l3_refresh_scom_reg(struct homer_st *homer, uint8_t dd)
{
	uint64_t refresh_val = 0x2000000000000000ULL;

	uint8_t quad = 0;

	/* ATTR_CHIP_EC_FEATURE_HW408892 === (DD <= 0x20) */
	if (powerbus_cfg()->fabric_freq >= 2000 && dd > 0x20)
		refresh_val |= PPC_PLACE(0x2, 8, 4);

	for (quad = 0; quad < MAX_QUADS_PER_CHIP; ++quad) {
		/* Create restore entry for L3 Refresh Timer Divider register */

		uint32_t scom_addr = (EX_DRAM_REF_REG | (quad << QUAD_BIT_POS));
		stop_save_scom(homer, scom_addr, refresh_val,
			       STOP_SECTION_EQ_SCOM, SCOM_APPEND);

		scom_addr |= ODD_EVEN_EX_POS;
		stop_save_scom(homer, scom_addr, refresh_val,
			       STOP_SECTION_EQ_SCOM, SCOM_APPEND);
	}
}

static void populate_ncu_rng_bar_scom_reg(struct homer_st *homer)
{
	enum { NX_RANGE_BAR_ADDR_OFFSET = 0x00000302031D0000 };

	uint8_t ex = 0;

	uint64_t regNcuRngBarData = PPC_PLACE(0x0, 8, 5)   // system ID
				  | PPC_PLACE(0x3, 13, 2)  // msel
				  | PPC_PLACE(0x0, 15, 4)  // group ID
				  | PPC_PLACE(0x0, 19, 3); // chip ID

	regNcuRngBarData += NX_RANGE_BAR_ADDR_OFFSET;

	for (ex = 0; ex < MAX_CMES_PER_CHIP; ++ex) {
		/* Create restore entry for NCU RNG register */

		uint32_t scom_addr = EX_0_NCU_DARN_BAR_REG
				   | ((ex / 2) << 24)
				   | ((ex % 2) ? 0x0400 : 0x0000);

		stop_save_scom(homer, scom_addr, regNcuRngBarData,
			       STOP_SECTION_EQ_SCOM, SCOM_REPLACE);
	}
}

static void update_headers(struct homer_st *homer, uint64_t cores)
{
	/*
	 * Update CPMR Header with Scan Ring details
	 * This function for each entry does one of:
	 * - write constant value
	 * - copy value form other field
	 * - one or both of the above with arithmetic operations
	 * Consider writing these fields in previous functions instead.
	 */
	struct cpmr_header *cpmr_hdr   = &homer->cpmr.header;
	struct cme_img_header *cme_hdr = (struct cme_img_header *)
	                                 &homer->cpmr.cme_sram_region[INT_VECTOR_SIZE];
	cpmr_hdr->img_offset           = offsetof(struct cpmr_st, cme_sram_region) / 32;
	cpmr_hdr->cme_pstate_offset    = offsetof(struct cpmr_st, cme_sram_region) + cme_hdr->pstate_region_offset;
	cpmr_hdr->cme_pstate_len       = cme_hdr->pstate_region_len;
	cpmr_hdr->img_len              = cme_hdr->hcode_len;
	cpmr_hdr->core_scom_offset     = offsetof(struct cpmr_st, core_scom);
	cpmr_hdr->core_scom_len        = CORE_SCOM_RESTORE_SIZE;			// 6k
	cpmr_hdr->core_max_scom_entry  = 15;

	if (cme_hdr->common_ring_len) {
		cpmr_hdr->cme_common_ring_offset = offsetof(struct cpmr_st, cme_sram_region) +
		                                   cme_hdr->common_ring_offset;
		cpmr_hdr->cme_common_ring_len    = cme_hdr->common_ring_len;
	}

	if (cme_hdr->max_spec_ring_len) {
		cpmr_hdr->core_spec_ring_offset  = ALIGN_UP(cpmr_hdr->img_offset * 32 +
		                                            cpmr_hdr->img_len +
		                                            cpmr_hdr->cme_pstate_len +
		                                            cpmr_hdr->cme_common_ring_len,
		                                            32) / 32;
		cpmr_hdr->core_spec_ring_len     = cme_hdr->max_spec_ring_len;
	}

	cme_hdr->custom_length =
		ALIGN_UP(cme_hdr->max_spec_ring_len * 32 + sizeof(LocalPstateParmBlock), 32) / 32;

	for (int cme = 0; cme < MAX_CORES_PER_CHIP/2; cme++) {
		/*
		 * CME index/position is the same as EX, however this means that Pstate
		 * offset is overwritten when there are 2 functional CMEs in one quad.
		 * Maybe we can use "for each functional quad" instead, but maybe
		 * 'cme * cme_hdr->custom_length' points to different data, based on
		 * whether there is one or two functional CMEs (is that even possible?).
		 */
		if (!IS_EX_FUNCTIONAL(cme, cores))
			continue;

		/* Assuming >= CPMR_2.0 */
		cpmr_hdr->quad_pstate_offset [cme/2] = cpmr_hdr->core_spec_ring_offset +
		                                       cpmr_hdr->core_spec_ring_len +
		                                       cme * cme_hdr->custom_length;
	}

	/* Updating CME Image header */
	/* Assuming >= CPMR_2.0 */
	cme_hdr->scom_offset =
		ALIGN_UP(cme_hdr->pstate_offset * 32 + sizeof(LocalPstateParmBlock), 32) / 32;

	/* Adding to it instance ring length which is already a multiple of 32B */
	cme_hdr->scom_len = 512;

	/* Timebase frequency */
	/* FIXME: get PB frequency properly */
	cme_hdr->timebase_hz = 1866 * MHz / 64;

	/*
	 * Update QPMR Header area in HOMER
	 * In Hostboot, qpmrHdr is a copy of the header, it doesn't operate on HOMER
	 * directly until now - it fills the following fields in the copy and then
	 * does memcpy() to HOMER. As BAR is set up in next istep, I don't see why.
	 */
	homer->qpmr.sgpe.header.sram_img_size =
	              homer->qpmr.sgpe.header.img_len +
	              homer->qpmr.sgpe.header.common_ring_len +
	              homer->qpmr.sgpe.header.spec_ring_len;
	homer->qpmr.sgpe.header.max_quad_restore_entry  = 255;
	homer->qpmr.sgpe.header.build_ver               = 3;
	struct sgpe_img_header *sgpe_hdr = (struct sgpe_img_header *)
	                                   &homer->qpmr.sgpe.sram_image[INT_VECTOR_SIZE];
	sgpe_hdr->scom_mem_offset = offsetof(struct homer_st, qpmr.cache_scom_region);

	/* Update PPMR Header area in HOMER */
	struct pgpe_img_header *pgpe_hdr = (struct pgpe_img_header *)
	                                   &homer->ppmr.pgpe_sram_img[INT_VECTOR_SIZE];
	pgpe_hdr->core_throttle_assert_cnt   = 0;
	pgpe_hdr->core_throttle_deassert_cnt = 0;
	pgpe_hdr->ivpr_addr                  = 0xFFF20000;	// OCC_SRAM_PGPE_BASE_ADDR
	                                 // = homer->ppmr.header.sram_region_start
	pgpe_hdr->gppb_sram_addr             = 0;		// set by PGPE Hcode (or not?)
	pgpe_hdr->hcode_len                  = homer->ppmr.header.hcode_len;
	/* FIXME: remove hardcoded HOMER in OCI PBA */
	pgpe_hdr->gppb_mem_offset            = 0x80000000 + offsetof(struct homer_st, ppmr) +
	                                       homer->ppmr.header.gppb_offset;
	pgpe_hdr->gppb_len                   = homer->ppmr.header.gppb_len;
	pgpe_hdr->gen_pstables_mem_offset    = 0x80000000 + offsetof(struct homer_st, ppmr) +
	                                       homer->ppmr.header.pstables_offset;
	pgpe_hdr->gen_pstables_len           = homer->ppmr.header.pstables_len;
	pgpe_hdr->occ_pstables_sram_addr     = 0;
	pgpe_hdr->occ_pstables_len           = 0;
	pgpe_hdr->beacon_addr                = 0;
	pgpe_hdr->quad_status_addr           = 0;
	pgpe_hdr->wof_state_address          = 0;
	pgpe_hdr->wof_values_address         = 0;
	pgpe_hdr->req_active_quad_address    = 0;
	pgpe_hdr->wof_table_addr             = homer->ppmr.header.wof_table_offset;
	pgpe_hdr->wof_table_len              = homer->ppmr.header.wof_table_len;
	pgpe_hdr->timebase_hz                = 1866 * MHz / 64;
	pgpe_hdr->doptrace_offset            = homer->ppmr.header.doptrace_offset;
	pgpe_hdr->doptrace_len               = homer->ppmr.header.doptrace_len;

	/* Update magic numbers */
	homer->qpmr.sgpe.header.magic = 0x51504d525f312e30;	// QPMR_1.0
	homer->cpmr.header.magic      = 0x43504d525f322e30;	// CPMR_2.0
	homer->ppmr.header.magic      = 0x50504d525f312e30;	// PPMR_1.0
	sgpe_hdr->magic               = 0x534750455f312e30;	// SGPE_1.0
	cme_hdr->magic                = 0x434d455f5f312e30;	// CME__1.0
	pgpe_hdr->magic               = 0x504750455f312e30;	// PGPE_1.0
}


/*
 * This logic is for SMF disabled only!
 */
void build_homer_image(void *homer_bar)
{
	static uint8_t rings_buf[300 * KiB];

	static uint8_t work_buf1[MAX_RING_BUF_SIZE];
	static uint8_t work_buf2[MAX_RING_BUF_SIZE];
	static uint8_t work_buf3[MAX_RING_BUF_SIZE];

	struct ring_data ring_data = {
		.rings_buf = rings_buf, .rings_buf_size = sizeof(rings_buf),
		.work_buf1 = work_buf1, .work_buf1_size = sizeof(work_buf1),
		.work_buf2 = work_buf2, .work_buf2_size = sizeof(work_buf2),
		.work_buf3 = work_buf3, .work_buf3_size = sizeof(work_buf3),
	};
	uint8_t ring_variant;

	struct mmap_helper_region_device mdev = {0};
	struct homer_st *homer = homer_bar;
	struct xip_hw_header *hw = homer_bar;
	uint8_t dd = get_dd();
	int this_core = -1;
	uint64_t cores = get_available_cores(&this_core);

	if (this_core == -1)
		die("Couldn't found active core\n");

	printk(BIOS_ERR, "DD%2.2x, boot core: %d\n", dd, this_core);

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
	                   dd, cores);

	build_cme(homer, (struct xip_cme_header *)(homer_bar + hw->cme.offset), dd);

	build_pgpe(homer, (struct xip_pgpe_header *)(homer_bar + hw->pgpe.offset),
	           dd);

	ring_variant = (dd < 0x23 ? RV_BASE : RV_RL4);

	get_ppe_scan_rings(hw, dd, PT_CME, &ring_data);
	layout_rings_for_cme(homer, &ring_data, cores, ring_variant);

	/* Reset buffer sizes to maximum values before reusing the structure */
	ring_data.rings_buf_size = sizeof(rings_buf);
	ring_data.work_buf1_size = sizeof(work_buf1);
	ring_data.work_buf2_size = sizeof(work_buf2);
	ring_data.work_buf3_size = sizeof(work_buf3);
	get_ppe_scan_rings(hw, dd, PT_SGPE, &ring_data);
	layout_rings_for_sgpe(homer, &ring_data,
			      (struct xip_sgpe_header *)(homer_bar + hw->sgpe.offset),
			      cores, ring_variant);

	build_parameter_blocks(homer, cores);

	update_headers(homer, cores);

	populate_epsilon_l2_scom_reg(homer);
	populate_epsilon_l3_scom_reg(homer);

	/* Update L3 Refresh Timer Control SCOM Registers */
	populate_l3_refresh_scom_reg(homer, dd);

	/* Populate HOMER with SCOM restore value of NCU RNG BAR SCOM Register */
	populate_ncu_rng_bar_scom_reg(homer);

	/* Update flag fields in image headers */
	((struct sgpe_img_header *)&homer->qpmr.sgpe.sram_image[INT_VECTOR_SIZE])->reserve_flags = 0x04000000;
	((struct cme_img_header *)&homer->cpmr.cme_sram_region[INT_VECTOR_SIZE])->qm_mode_flags = 0xf100;
	((struct pgpe_img_header *)&homer->ppmr.pgpe_sram_img[INT_VECTOR_SIZE])->flags = 0xf032;

	// Set the Fabric IDs
	// setFabricIds( pChipHomer, i_procTgt );
	//	- doesn't modify anything?

	/* Set up wakeup mode */
	for (int i = 0; i < MAX_CORES_PER_CHIP; i++) {
		if (!IS_EC_FUNCTIONAL(i, cores))
			continue;

		/*
		TP.TPCHIP.NET.PCBSLEC14.PPMC.PPM_CORE_REGS.CPPM_CPMMR		// 0x200F0106
			// These bits, when set, make core wake up in HV (not UV)
			[3]	CPPM_CPMMR_RESERVED_2_9	= 1
			[4]	CPPM_CPMMR_RESERVED_2_9	= 1
		*/
		/* SCOM2 - OR, 0x200F0108 */
		write_scom_for_chiplet(EC00_CHIPLET_ID + i, 0x200F0108,
		                       PPC_BIT(3) | PPC_BIT(4));
	}

	/* 15.2 set HOMER BAR */
	report_istep(15,2);
	write_scom(0x05012B00, (uint64_t)homer);
	write_scom(0x05012B04, (4 * MiB - 1) & ~((uint64_t)MiB - 1));
	write_scom(0x05012B02, (uint64_t)homer + 8 * 4 * MiB);		// FIXME
	write_scom(0x05012B06, (8 * MiB - 1) & ~((uint64_t)MiB - 1));

	/* 15.3 establish EX chiplet */
	report_istep(15,3);
	/* Multicast groups for cores were assigned in get_available_cores() */
	for (int i = 0; i < MAX_QUADS_PER_CHIP; i++) {
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
	for (int i = 0; i < MAX_CMES_PER_CHIP; i++) {
		if (IS_EX_FUNCTIONAL(i, cores))
			qcsr |= PPC_BIT(i);
	}
	write_scom(0x0006C094, qcsr);

	/* 15.4 start STOP engine */
	report_istep(15,4);

	/* Initialize the PFET controllers */
	for (int i = 0; i < MAX_CORES_PER_CHIP; i++) {
		if (IS_EC_FUNCTIONAL(i, cores)) {
			// Periodic core quiesce workaround
			/*
			TP.TPCHIP.NET.PCBSLEC14.PPMC.PPM_CORE_REGS.CPPM_CPMMR (WOR)     // 0x200F0108
				[all] 0
				[2]   CPPM_CPMMR_RESERVED_2 = 1
			*/
			write_scom_for_chiplet(EC00_CHIPLET_ID + i, 0x200F0108, PPC_BIT(2));

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
		}

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

	istep_16_1(this_core);
}
