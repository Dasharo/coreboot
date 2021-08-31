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

#include <lib.h>

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

#define NUM_OP_POINTS         4
#define NUM_JUMP_VALUES       4
#define NUM_THRESHOLD_POINTS  4
#define VPD_NUM_SLOPES_REGION 3

struct vpd_operating_point {
	uint32_t vdd_mv;
	uint32_t vcs_mv;
	uint32_t idd_100ma;
	uint32_t ics_100ma;
	uint32_t frequency_mhz;
	uint8_t  pstate;
	uint8_t  pad[3];
};

struct sys_power_dist {
	/* Impedance of the load line from a processor VDD VRM to the Processor Module pins. */
	uint32_t loadline_uohm;

	/* Impedance of the VDD distribution loss sense point to the circuit. */
	uint32_t distloss_uohm;

	/* Offset voltage to apply to the rail VRM distribution to the processor module. */
	uint32_t distoffset_uv;
};

/* Percent bias applied to VPD operating points prior to interolation in 0.5 percent (hp). */
struct vpd_bias {
	int8_t vdd_ext_hp;
	int8_t vdd_int_hp;
	int8_t vdn_ext_hp;
	int8_t vcs_ext_hp;
	int8_t frequency_hp;
};

#define IVRM_ARRAY_SIZE 64

struct ivrm_params {
	/* Pwidth from 0.03125 to 1.96875 in 1/32 increments at Vin=Vin_Max */
	uint8_t strength_lookup[IVRM_ARRAY_SIZE];

	/* Scaling factor for the Vin_Adder calculation */
	uint8_t vin_multiplier[IVRM_ARRAY_SIZE];

	/* Vin_Max used in Vin_Adder calculation (in millivolts) */
	uint16_t vin_max_mv;

	/* Delay between steps. Maximum: 65.536us. */
	uint16_t step_delay_ns;

	/* Stabilization delay once target voltage has been reached. Maximum: 65.536us. */
	uint16_t stablization_delay_ns;

	/* Deadzone. Maximum: 255mV. Value of 0 is interpreted as 50mV. */
	uint8_t deadzone_mv;

	/* Pad to 8B */
	uint8_t pad;
};

#define RESCLK_FREQ_REGIONS 8
#define RESCLK_STEPS        64
#define RESCLK_L3_STEPS     4

struct resonant_clocking {
	/* Lower frequency of Resclk Regions */
	uint8_t resclk_freq[RESCLK_FREQ_REGIONS];

	/* Index into value array for the respective Resclk Region */
	uint8_t resclk_index[RESCLK_FREQ_REGIONS];

	/* Array containing the transition steps */
	uint16_t steparray[RESCLK_STEPS];

	/* Delay between steps. Maximum: 65.536us. */
	uint16_t step_delay_ns;

	/* L3 Clock Stepping Array */
	uint8_t l3_steparray[RESCLK_L3_STEPS];

	/* Resonant Clock Voltage Threshold. This value is used to choose the
	 * appropriate L3 clock region setting. */
	uint16_t l3_threshold_mv;
};

/* #W data points (version 2) */
struct poundw_entry {
	uint16_t ivdd_tdp_ac_current_10ma;
	uint16_t ivdd_tdp_dc_current_10ma;
	uint8_t  vdm_overvolt_small_thresholds;
	uint8_t  vdm_large_extreme_thresholds;
	uint8_t  vdm_normal_freq_drop;		// N_S and N_L Drop
	uint8_t  vdm_normal_freq_return;	// L_S and S_N Return
	uint8_t  vdm_vid_compare_ivid;
	uint8_t  vdm_spare;
};

struct resistance_entry {
	uint16_t r_package_common;
	uint16_t r_quad;
	uint16_t r_core;
	uint16_t r_quad_header;
	uint16_t r_core_header;
};

struct PoundW_data {
	struct poundw_entry poundw[NUM_OP_POINTS];
	struct resistance_entry resistance_data;
	uint8_t undervolt_tested;
	uint8_t reserved;
	uint64_t reserved1;
	uint8_t reserved2;
};

struct local_pstate_params {
	/* Magic Number (the last byte of this number is the structure's version) */
	uint64_t magic;

	/* QM Flags */
	uint16_t qmflags;

	/* Operating points */
	struct vpd_operating_point operating_points[NUM_OP_POINTS];

	/* Loadlines and Distribution values for the VDD rail */
	struct sys_power_dist vdd_sysparm;

	/* External Biases */
	struct vpd_bias ext_biases[NUM_OP_POINTS];

	/* Internal Biases */
	struct vpd_bias int_biases[NUM_OP_POINTS];

	/* IVRM Data */
	struct ivrm_params ivrm;

	/* Resonant Clock Grid Management Setup */
	struct resonant_clocking resclk;

	/* VDM Data */
	struct PoundW_data vpd_w_data;

	/* DPLL pstate 0 value */
	uint32_t dpll_pstate0_value;

	/* Biased Compare VID operating points */
	uint8_t vid_point_set[NUM_OP_POINTS];

	/* Biased Threshold operation points */
	uint8_t threshold_set[NUM_OP_POINTS][NUM_THRESHOLD_POINTS];

	/* pstate-volt compare slopes */
	int16_t PsVIDCompSlopes[VPD_NUM_SLOPES_REGION];

	/* pstate-volt threshold slopes */
	int16_t PsVDMThreshSlopes[VPD_NUM_SLOPES_REGION][NUM_THRESHOLD_POINTS];

	/* Jump-value operating points */
	uint8_t jump_value_set[NUM_OP_POINTS][NUM_JUMP_VALUES];

	/* Jump-value slopes */
	int16_t PsVDMJumpSlopes[VPD_NUM_SLOPES_REGION][NUM_JUMP_VALUES];
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
				 * SMFCTRL through USPRG1:  key = 1C..1E
				 */
				int tsa_key = i;
				if (i > 7)
					tsa_key += 0x14;

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
	for (int i = 0; i < MAX_CORES; i++) {
		uint64_t val = read_scom_for_chiplet(EC00_CHIPLET_ID + i, 0xF0040);
		if (val & PPC_BIT(0)) {
			printk(BIOS_SPEW, "Core %d is functional%s\n", i,
			       (val & PPC_BIT(1)) ? "" : " and running");
			ret |= PPC_BIT(i);
			if (val & PPC_BIT(1) && me != NULL)
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

#define IS_EC_FUNCTIONAL(ec, cores)		(!!((cores) & PPC_BIT(ec)))
#define IS_EX_FUNCTIONAL(ex, cores)		(!!((cores) & PPC_BITMASK(2*(ex), 2*(ex) + 1)))
#define IS_EQ_FUNCTIONAL(eq, cores)		(!!((cores) & PPC_BITMASK(4*(eq), 4*(eq) + 3)))

/* TODO: similar is used in 13.3. Add missing parameters and make it public? */
static void psu_command(uint8_t flags, long time)
{
	/* TP.TPCHIP.PIB.PSU.PSU_SBE_DOORBELL_REG */
	if (read_scom(0x000D0060) & PPC_BIT(0))
		die("MBOX to SBE busy, this should not happen\n");

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
	time = wait_ms(time, read_scom(0x000D0060) & PPC_BIT(0));

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

	/* Register is documented, but its bits are reserved... */
	write_scom_for_chiplet(EC00_CHIPLET_ID + core, scom, PPC_BIT(6));
}

/*
 * TODO: check if we need to save current core/thread number. Both cores under
 * quad are currently started, but this may be caused by re-using runtime HOMER.
 *
 * TODO: save and restore TB. Some time will be lost between entering and
 * exiting STOP 15, but we don't have a way of calculating it (I think).
 */
struct save_state {
	uint64_t r1;	/* stack */
	uint64_t r2;	/* TOC */
	uint64_t msr;
	uint64_t nia;
} sstate;

/*
 * System reset vector:
 * - read thread ID from PIR
 * - thread != 0:
 *   - stop
 * - thread == 0:
 *   - reload r1 and r2 from saved state
 *   - move nia and msr to HSRR0/1
 *   - return from hypervisor interrupt
 *   - due to clobbers in inline assembly in cpu_winkle all other registers are
 *     reloaded by compiler
 *     - contents of vector and floating point registers are lost
 */
asm(
"\
system_reset:                          \n\
	mfspr   4, 1023                \n\
	extrdi. 4, 4, 2, 62            \n\
	bne     _not_thread0           \n\
	li      3, sstate@l            \n\
	oris    3, 3, sstate@h         \n\
	ld      1, 0(3)                \n\
	ld      2, 8(3)                \n\
	ld      4, 16(3)               \n\
	mtspr   315, 4                 \n\
	ld      4, 24(3)               \n\
	mtspr   314, 4                 \n\
	hrfid                          \n\
_not_thread0:                          \n\
	stop                           \n\
	b       .                      \n\
system_reset_end:                      \n\
");

extern uint8_t system_reset[];
extern uint8_t system_reset_end[];

static void cpu_winkle(void)
{
	uint64_t psscr = read_spr(SPR_PSSCR);
	uint64_t lpcr = read_spr(SPR_LPCR);
	/* Clear {External, Decrementer, Other} Exit Enable */
	lpcr &= ~(SPR_LPCR_EEE | SPR_LPCR_DEE | SPR_LPCR_OEE);
	/*
	 * Set Hypervisor Virtualization Interrupt Conditionally Enable
	 * and Hypervisor Virtualization Exit Enable
	 */
	lpcr |= SPR_LPCR_HVICE | SPR_LPCR_HVEE;
	write_spr(SPR_LPCR, lpcr);
	write_spr(SPR_PSSCR, 0x00000000003F00FF);

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

	memcpy((void*)0x100, system_reset, system_reset_end - system_reset);

	asm volatile("sync; isync" ::: "memory");

	/* TODO: address depends on active core */
	/* TODO: do we even need this? Those threads are already stopped */
	// write_scom(0x21010A9C, 0x0008080800000000);

	/*
	 * Timing facilities may be lost. During their restoration Large Decrementer
	 * in LPCR may be initially turned off, which may (but shouldn't) result in
	 * a spurious Decrementer Exception. As we don't have handlers, disable all
	 * External Interrupts just in case.
	 */
	sstate.msr = read_msr() & ~PPC_BIT(48);

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
		             "lnia     1\n"
		             "__tmp_nia:"
		             "addi     1, 1, wakey - __tmp_nia\n"
		             "std      1, 24(%0)\n"
		             "sync\n"
		             "stop\n"
		             "wakey:\n"
		             : "+r"(r3) ::
		             "r0", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11",
		             "r12", "r13", "r14", "r15", "r16", "r17", "r18", "r19",
		             "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27",
		             "r28", "r29", "r30", "r31", "memory", "cc");
	}

	write_spr(SPR_PSSCR, psscr);
}

static void istep_16_1(int this_core)
{
	/*
	 * Wait time 10.5 sec, anything larger than 10737 ms can cause overflow on
	 * SBE side of the timeout calculations.
	 */
	long time = 10500;

	/*
	 * This will request SBE to wake us up after we enter STOP 15. Hopefully
	 * we will come back to the place where we were before.
	 */
	printk(BIOS_ERR, "Entering dead man loop\n");
	uint64_t t1 = read_spr(SPR_TB);
	psu_command(DEADMAN_LOOP_START, time);

	/* REMOVE ME! */
	/*
	 * This masks FIR bit that is set by SBE when dead man timer expires, which
	 * results in checkstop. To help with debugging it is masked here, but it
	 * must not be in the final code!
	 */
	write_scom(0x0504000F, PPC_BIT(32));

	uint64_t t2 = read_spr(SPR_TB);
	block_wakeup_int(this_core, 1);

	cpu_winkle();

	/*
	 * This tells SBE that we were properly awoken. Hostboot uses default
	 * timeout of 90 seconds, but if SBE doesn't answer in 10 there is no reason
	 * to believe it will answer at all.
	 */
	psu_command(DEADMAN_LOOP_STOP, time);
	uint64_t t3 = read_spr(SPR_TB);

	printk(BIOS_ERR, "Dead man loop times: %llx, %llx\n", t2-t1, t3-t1);
	printk(BIOS_ERR, "Dead man loop times: %lldns, %lldns\n", (t2-t1)*2, (t3-t1)*2);
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
			ex * (max_ex_len + ALIGN_UP(sizeof(struct local_pstate_params), 32));

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
		cme_hdr->core_spec_ring_offset = cpmr_hdr->cme_common_ring_offset + cpmr_hdr->cme_common_ring_len;
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
		offsetof(struct homer_st, qpmr.sgpe.sram_image) + qpmr_hdr->img_len;
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

	printk(BIOS_ERR, "DD%2.2x\n", dd);

	/* HOMER must be aligned to 4M because CME HRMOR has bit for 2M set */
	if (!IS_ALIGNED((uint64_t) homer_bar, 4 * MiB))
		die("HOMER (%p) is not aligned to 4MB\n", homer_bar);



	/*
	 * Temporarily use HOMER read from running system, until code for crafting
	 * it from HCODE is ready.
	 */
	if (0) {

	void *map = cbfs_map("homer", NULL);
	memcpy(homer_bar, map, 4 * MiB);
	cbfs_unmap(map);

	} else {


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

	// buildParameterBlock();
	// updateCpmrCmeRegion();

	// Update QPMR Header area in HOMER
	// updateQpmrHeader();

	// Update PPMR Header area in HOMER
	// updatePpmrHeader();

	// Update L2 Epsilon SCOM Registers
	// populateEpsilonL2ScomReg( pChipHomer );

	// Update L3 Epsilon SCOM Registers
	// populateEpsilonL3ScomReg( pChipHomer );

	// Update L3 Refresh Timer Control SCOM Registers
	// populateL3RefreshScomReg( pChipHomer, i_procTgt);

	// Populate HOMER with SCOM restore value of NCU RNG BAR SCOM Register
	// populateNcuRngBarScomReg( pChipHomer, i_procTgt );

	// validate SRAM Image Sizes of PPE's
	// validateSramImageSize( pChipHomer, sramImgSize );

	// Update CME/SGPE Flags in respective image header.
	// updateImageFlags( pChipHomer, i_procTgt );

	// Set the Fabric IDs
	// setFabricIds( pChipHomer, i_procTgt );
	//	- doesn't modify anything?

	// Customize magic word based on endianess
	// customizeMagicWord( pChipHomer );
	}

	/* Set up wakeup mode */
	for (int i = 0; i < MAX_CORES; i++) {
		if (!IS_EC_FUNCTIONAL(i, cores))
			continue;

		/*
		TP.TPCHIP.NET.PCBSLEC14.PPMC.PPM_CORE_REGS.CPPM_CPMMR		// 0x200F0106
			// These bits, when set, make core wake up in HV (not UV)
			[3]	CPPM_CPMMR_RESERVED_2_9	= 1
			[4]	CPPM_CPMMR_RESERVED_2_9	= 1
		*/
		write_scom_for_chiplet(EC00_CHIPLET_ID + i, 0x200F0106,
		                       PPC_BIT(3) | PPC_BIT(4));
	}

	/* 15.2 set HOMER BAR */
	write_scom(0x05012B00, (uint64_t)homer);
	write_scom(0x05012B04, (4 * MiB - 1) & ~((uint64_t)MiB - 1));
	write_scom(0x05012B02, (uint64_t)homer + 8 * 4 * MiB);		// FIXME
	write_scom(0x05012B06, (7 * MiB - 1) & ~((uint64_t)MiB - 1));

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

	istep_16_1(this_core);

	//hexdump(&homer->qpmr, sgpe->qpmr.size);
}
