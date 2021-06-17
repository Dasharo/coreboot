/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_HOMER_H
#define __SOC_IBM_POWER9_HOMER_H

#include <commonlib/bsd/helpers.h>

/* All fields are big-endian */

#define HOMER_ONE_REGION_SIZE		(1 * MiB)

/*
 * OCC complex shares 768 kB SRAM, according to Figure 23-3
 * https://wiki.raptorcs.com/w/images/c/ce/POWER9_um_OpenPOWER_v21_10OCT2019_pub.pdf
 */
#define SPGE_SRAM_IMG_SIZE		(74 * KiB)
#define PPGE_SRAM_IMG_SIZE		(50 * KiB)

/* According to above figure, CMEs have 32 kB SRAM each, how does it fit? */
#define CME_SRAM_IMG_SIZE		(64 * KiB)
#define PGE_BOOTLOADER_SIZE		(1 * KiB)

/*
 * This is how CACHE_SCOM_REGION_SIZE is defined in hostboot. On the other hand,
 * hostboot defines that quad has 256 entries, 16 bytes each. This gives 4kB per
 * quad, and there are 6 quads (maximum) on POWER9 CPU, which gives 24kB total.
 * One of the values is obviously wrong, but because this region is immediately
 * followed by a padding it should not overwrite anything important. This is one
 * of the reasons to clear whole HOMER, not just the used parts.
 */
#define CACHE_SCOM_REGION_SIZE		(6 * KiB)
#define CACHE_SCOM_REGION_OFFSET	(128 * KiB)
#define CACHE_SCOM_AUX_SIZE		(64 * KiB)
#define CACHE_SCOM_AUX_OFFSET		(512 * KiB)
#define SELF_RESTORE_REGION_SIZE	(9 * KiB)
#define CORE_SCOM_RESTORE_SIZE		(6 * KiB)
#define CORE_SCOM_RESTORE_OFFSET	(256 * KiB)
#define PGPE_AUX_TASK_SIZE		(2 * KiB)
#define OCC_PARAM_BLOCK_REGION_SIZE	(16 * KiB)
#define OCC_PARAM_BLOCK_REGION_OFFSET	(128 * KiB)
#define PSTATE_OUTPUT_TABLES_SIZE	(16 * KiB)
#define OCC_WOF_TABLES_SIZE		(256 * KiB)
#define OCC_WOF_TABLES_OFFSET		(768 * KiB)
#define PPMR_HEADER_SIZE		(1 * KiB)

/* =================== QPMR =================== */

struct qpmr_header {
	uint64_t magic;		/* "SGPE_1.0" */
	uint32_t l1_offset;
	uint32_t reserved;
	uint32_t l2_offset;
	uint32_t l2_len;
	uint32_t build_date;
	uint32_t build_ver;
	uint64_t reserved_flags;
	uint32_t img_offset;
	uint32_t img_len;
	uint32_t common_ring_offset;
	uint32_t common_ring_len;
	uint32_t common_ovrd_offset;
	uint32_t common_ovrd_len;
	uint32_t spec_ring_offset;
	uint32_t spec_ring_len;
	uint32_t scom_offset;
	uint32_t scom_len;
	uint32_t aux_offset;
	uint32_t aux_len;
	uint32_t stop_ffdc_offset;
	uint32_t stop_ffdc_len;
	uint32_t boot_prog_code;
	uint32_t sram_img_size;
	uint32_t max_quad_restore_entry;
	uint32_t enable_24x7_ima;
	uint32_t sram_region_start;
	uint32_t sram_region_size;
} __attribute__((packed, aligned(512)));

/* This header is part of SRAM image, it starts after interrupt vectors. */
#define SPGE_IMG_HEADER_OFFSET	384
struct sgpe_img_header {
	uint64_t magic;
	uint32_t reset_addr;
	uint32_t reserve1;
	uint32_t ivpr_addr;
	uint32_t timebase_hz;
	uint32_t build_date;
	uint32_t build_ver;
	uint32_t reserve_flags;
	uint16_t location_id;
	uint16_t addr_extension;
	uint32_t cmn_ring_occ_offset;
	uint32_t cmn_ring_ovrd_occ_offset;
	uint32_t spec_ring_occ_offset;
	uint32_t scom_offset;
	uint32_t scom_mem_offset;
	uint32_t scom_mem_len;
	uint32_t aux_offset;
	uint32_t aux_len;
	uint32_t aux_control;
	uint32_t reserve4;
	uint64_t chtm_mem_cfg;
};

struct spge_st {
	struct qpmr_header header;
	uint8_t l1_bootloader[PGE_BOOTLOADER_SIZE];
	uint8_t l2_bootloader[PGE_BOOTLOADER_SIZE];
	uint8_t sram_image[SPGE_SRAM_IMG_SIZE];
};

check_member(spge_st, l1_bootloader, 512);

struct qpmr_st {
	struct spge_st spge;
	uint8_t pad1[CACHE_SCOM_REGION_OFFSET - sizeof(struct spge_st)];
	uint8_t cache_scom_region[CACHE_SCOM_REGION_SIZE];
	uint8_t pad2[CACHE_SCOM_AUX_OFFSET
                     -(CACHE_SCOM_REGION_OFFSET + CACHE_SCOM_REGION_SIZE)];
	uint8_t aux[CACHE_SCOM_AUX_SIZE];
};

check_member(qpmr_st, cache_scom_region, 128 * KiB);
check_member(qpmr_st, aux, 512 * KiB);

/* =================== CPMR =================== */

struct cpmr_header {
	uint32_t attn_opcodes[2];
	uint64_t magic;		/* "CPMR_2.0" */
	uint32_t build_date;
	uint32_t version;
	uint8_t reserved_flags[4];
	uint8_t self_restore_ver;
	uint8_t stop_api_ver;
	uint8_t urmor_fix;
	uint8_t fused_mode_status;
	uint32_t img_offset;
	uint32_t img_len;
	uint32_t cme_common_ring_offset;;
	uint32_t cme_common_ring_len;
	uint32_t cme_pstate_offset;
	uint32_t cme_pstate_len;
	uint32_t core_spec_ring_offset;
	uint32_t core_spec_ring_len;
	uint32_t core_scom_offset;
	uint32_t core_scom_len;
	uint32_t core_self_restore_offset;
	uint32_t core_self_restore_len;
	uint32_t core_max_scom_entry;
	uint32_t quad0_pstate_offset;
	uint32_t quad1_pstate_offset;
	uint32_t quad2_pstate_offset;
	uint32_t quad3_pstate_offset;
	uint32_t quad4_pstate_offset;
	uint32_t quad5_pstate_offset;
} __attribute__((packed, aligned(256)));

struct cpmr_st {
	struct cpmr_header header;
	uint8_t exe[SELF_RESTORE_REGION_SIZE - sizeof(struct cpmr_header)];
	/* This is 96kB followed by a padding in hostboot, KISS. */
	uint8_t core_self_restore[CORE_SCOM_RESTORE_OFFSET
                                  - SELF_RESTORE_REGION_SIZE];
	uint8_t core_scom[CORE_SCOM_RESTORE_SIZE];
	uint8_t cme_sram_region[CME_SRAM_IMG_SIZE];
};

check_member(cpmr_st, core_self_restore, 9 * KiB);
check_member(cpmr_st, core_scom, 256 * KiB);

/* =================== PPMR =================== */

struct ppmr_st {
	uint8_t  header[PPMR_HEADER_SIZE];
	uint8_t  l1_bootloader[PGE_BOOTLOADER_SIZE];
	uint8_t  l2_bootloader[PGE_BOOTLOADER_SIZE];
	uint8_t  pgpe_sram_img[PPGE_SRAM_IMG_SIZE];
	uint8_t  aux_task[PGPE_AUX_TASK_SIZE];
	uint8_t  pad0[OCC_PARAM_BLOCK_REGION_OFFSET - (PPMR_HEADER_SIZE +
	              PGE_BOOTLOADER_SIZE + PGE_BOOTLOADER_SIZE +
	              PPGE_SRAM_IMG_SIZE + PGPE_AUX_TASK_SIZE)];
	/*
	 * Two following fields in hostboot have different sizes, but are padded
	 * to 16kB each anyway. There are two consecutive paddings between
	 * pstate_table and wof_tables in hostboot.
	 */
	uint8_t  occ_parm_block[OCC_PARAM_BLOCK_REGION_SIZE];
	uint8_t  pstate_table[PSTATE_OUTPUT_TABLES_SIZE];
	uint8_t  pad1[OCC_WOF_TABLES_OFFSET - (OCC_PARAM_BLOCK_REGION_OFFSET +
	              OCC_PARAM_BLOCK_REGION_SIZE + PSTATE_OUTPUT_TABLES_SIZE)];
	uint8_t  wof_tables[OCC_WOF_TABLES_SIZE];
};

check_member(ppmr_st, occ_parm_block, 128 * KiB);
check_member(ppmr_st, pstate_table, 144 * KiB);
check_member(ppmr_st, wof_tables, 768 * KiB);

struct homer_st {
	uint8_t occ_host_area[HOMER_ONE_REGION_SIZE];
	struct qpmr_st qpmr;
	uint8_t pad_qpmr[HOMER_ONE_REGION_SIZE - sizeof(struct qpmr_st)];
	struct cpmr_st cpmr;
	uint8_t pad_cpmr[HOMER_ONE_REGION_SIZE - sizeof(struct cpmr_st)];
	struct ppmr_st ppmr;
	uint8_t pad_ppmr[HOMER_ONE_REGION_SIZE - sizeof(struct ppmr_st)];
};

check_member(homer_st, qpmr, 1 * MiB);
check_member(homer_st, cpmr, 2 * MiB);
check_member(homer_st, ppmr, 3 * MiB);

#endif /* __SOC_IBM_POWER9_HOMER_H */
