/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef MCTI_H
#define MCTI_H

#include <stdint.h>
#include <stdlib.h>

#define SERVER		0
#define DESKTOP		1
//#define MOBILE	2
#define REV_F		0
#define REV_DR		1
#define REV_FDR		2

/*----------------------------------------------------------------------------
COMMENT OUT ALL BUT 1
----------------------------------------------------------------------------*/
//#define BUILD_VERSION   REV_F		/*BIOS supports rev F only*/
//#define BUILD_VERSION   REV_DR	/*BIOS supports rev 10 only*/
//#define BUILD_VERSION   REV_FDR	/*BIOS supports both rev F and 10*/

/*----------------------------------------------------------------------------
COMMENT OUT ALL BUT 1
----------------------------------------------------------------------------*/
#ifndef SYSTEM_TYPE
#define    SYSTEM_TYPE	    SERVER
//#define    SYSTEM_TYPE     DESKTOP
//#define    SYSTEM_TYPE     MOBILE
#endif

/*----------------------------------------------------------------------------
UPDATE AS NEEDED
----------------------------------------------------------------------------*/
#ifndef MAX_NODES_SUPPORTED
#define MAX_NODES_SUPPORTED		8
#endif

#ifndef MAX_DIMMS_SUPPORTED
#if CONFIG(DIMM_DDR3)
 #define MAX_DIMMS_SUPPORTED		6
#else
 #define MAX_DIMMS_SUPPORTED		8
#endif
#endif

#ifndef MAX_CS_SUPPORTED
#define MAX_CS_SUPPORTED		8
#endif

#ifndef MCT_DIMM_SPARE_NO_WARM
#define MCT_DIMM_SPARE_NO_WARM	0
#endif

#ifndef MEM_MAX_LOAD_FREQ
#if CONFIG(DIMM_DDR3)
 #define MEM_MAX_LOAD_FREQ			933
 #define MEM_MIN_PLATFORM_FREQ_FAM10		400
 #define MEM_MIN_PLATFORM_FREQ_FAM15		333
#else						 /* AMD_FAM10_DDR2 */
 #define MEM_MAX_LOAD_FREQ			400
 #define MEM_MIN_PLATFORM_FREQ_FAM10		200
 /* DDR2 not available on Family 15h */
 #define MEM_MIN_PLATFORM_FREQ_FAM15		0
#endif
#endif

#define MCT_TRNG_KEEPOUT_START		0x00000C00
#define MCT_TRNG_KEEPOUT_END		0x00000CFF

#define NVRAM_DDR2_800 0
#define NVRAM_DDR2_667 1
#define NVRAM_DDR2_533 2
#define NVRAM_DDR2_400 3

#define NVRAM_DDR3_1600 0
#define NVRAM_DDR3_1333 1
#define NVRAM_DDR3_1066 2
#define NVRAM_DDR3_800  3

/* The recommended maximum GFX Upper Memory Area
 * size is 256M, however, to be on the safe side
 * move TOM down by 512M.
 */
#define MAXIMUM_GFXUMA_SIZE 0x20000000

/* Do not allow less than 16M of DRAM in 32-bit space.
 * This number is not hardware constrained and can be
 * changed as needed.
 */
#define MINIMUM_DRAM_BELOW_4G 0x1000000

static const u16 ddr2_limits[4] = {400, 333, 266, 200};
static const u16 ddr3_limits[16] = {933, 800, 666, 533, 400, 333, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#include <cpu/amd/amddefs.h>
#include <cpu/amd/common/common.h>

#if CONFIG(DIMM_DDR3)
	#include <drivers/amd/amdmct/mct/mct_ddr3/mct_d.h>
#else
	#include <drivers/amd/amdmct/mct/mct_ddr2/mct_d.h>
#endif

#include <northbridge/amd/amdfam10/raminit.h>

#if CONFIG(DIMM_DDR2)
void mctSaveDQSSigTmg_D(void);
void mctGetDQSSigTmg_D(void);
u8 mctSetNodeBoundary_D(void);
#endif
u16 mct_get_nv_bits(u8 index);
void mct_hook_after_dimm_pre(void);
void mct_get_max_load_freq(struct DCTStatStruc *p_dct_stat);
void mct_adjust_auto_cyc_tmg_d(void);
void mct_hook_after_auto_cyc_tmg(void);
void mct_get_cs_exclude_map(void);
void mct_hook_before_ecc(void);
void mct_hook_after_ecc(void);
void mct_hook_after_auto_cfg(void);
void mct_hook_after_ps_cfg(void);
void mct_hook_after_ht_map(void);
void mct_hook_after_cpu(void);
void mct_init_mem_gpios_a_d(void);
void mct_node_id_debug_port_d(void);
void mct_warm_reset_d(void);
void mct_hook_before_dram_init(void);
void mct_hook_after_dram_init(void);
void mct_hook_before_any_training(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void mct_hook_after_any_training(void);
void mct_get_dimm_addr(struct DCTStatStruc *p_dct_stat, u32 node);

#if CONFIG(DIMM_DDR3)
void v_erratum_372(struct DCTStatStruc *p_dct_stat);
void v_erratum_414(struct DCTStatStruc *p_dct_stat);
u32 mct_adjust_spd_timings(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a, u32 val);
#endif

#endif
