/* SPDX-License-Identifier: GPL-2.0-only */

/* Description: Main memory controller system configuration for DDR 3 */

/* KNOWN ISSUES - ERRATA
 *
 * trtp is not calculated correctly when the controller is in 64-bit mode, it
 * is 1 busclock off.	No fix planned.	 The controller is not ordinarily in
 * 64-bit mode.
 *
 * 32 Byte burst not supported. No fix planned. The controller is not
 * ordinarily in 64-bit mode.
 *
 * trc precision does not use extra Jedec defined fractional component.
 * InsteadTrc (course) is rounded up to nearest 1 ns.
 *
 * Mini and Micro dimm not supported. Only RDIMM, UDIMM, SO-dimm defined types
 * supported.
 */

#include <console/console.h>
#include <northbridge/amd/amdfam10/debug.h>
#include <northbridge/amd/amdfam10/amdfam10.h>
#include <southbridge/amd/common/reset.h>
#include <cpu/amd/common/common.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <cpu/x86/mtrr.h>
#include <device/pci_ops.h>
#include <acpi/acpi.h>
#include <delay.h>
#include <string.h>
#include <types.h>
#include <device/dram/ddr3.h>
#include <option.h>

#include "s3utils.h"
#include "mct_d_gcc.h"
#include <drivers/amd/amdmct/wrappers/mcti.h>

static u8 reconfigure_dimm_spare_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a);
static void rqs_timing_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a,
				u8 allow_config_restore);
static void load_dqs_sig_tmg_regs_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a);
static void ht_mem_map_init_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void sync_dcts_ready_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void clear_dct_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 auto_cyc_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_preset_max_f_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void spd_get_tcl_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 auto_config_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void spd_set_banks_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void stitch_memory_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static u16 get_fk_d(u8 k);
static u8 get_dimm_address_d(struct DCTStatStruc *p_dct_stat, u8 i);
static void mct_pre_init_dct(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void mct_init_dct(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void mct_dram_init(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_sync_dcts_ready(struct DCTStatStruc *p_dct_stat);
static void get_trdrd(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_after_get_clt(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 mct_spd_calc_width(struct MCTStatStruc *p_mct_stat,\
					struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_after_stitch_memory(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 mct_dimm_presence(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static void set_other_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_twrwr(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_twrrd(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_trwt_to(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_trwt_wb(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat);
static void get_dqs_rcv_en_gross_diff(struct DCTStatStruc *p_dct_stat,
					u32 dev, u8 dct, u32 index_reg);
static void get_wr_dat_gross_diff(struct DCTStatStruc *p_dct_stat, u8 dct,
					u32 dev, u32 index_reg);
static u16 get_dqs_rcv_en_gross_max_min(struct DCTStatStruc *p_dct_stat,
				u32 dev, u8 dct, u32 index_reg, u32 index);
static void mct_final_mct_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static u16 get_wr_dat_gross_max_min(struct DCTStatStruc *p_dct_stat, u8 dct,
				u32 dev, u32 index_reg, u32 index);
static void mct_initial_mct_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void mct_init(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat);
static void clear_legacy_mode(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void mct_ht_mem_map_ext(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void set_cs_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void set_cke_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void set_odt_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void init_ddr_phy(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static u32 mct_node_present_d(void);
static void mct_other_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void mct_reset_data_struct_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a);
static void mct_early_arb_en_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_before_dram_init__prod_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_ProgramODT_D(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
void mct_clr_cl_to_nb_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat);
static u8 check_nb_cof_early_arb_en(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat);
void mct_clr_wb_enh_wsb_dis_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat);
static void mct_before_dqs_train_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a);
static void after_dram_init_d(struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_reset_dll_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static u32 mct_dis_dll_shutdown_sr(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u32 dram_config_lo, u8 dct);
static void mct_en_dll_shutdown_sr(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
void change_mem_clk(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat);

static u8 get_latency_diff(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static void sync_settings(struct DCTStatStruc *p_dct_stat);
static u8 crc_check(struct DCTStatStruc *p_dct_stat, u8 dimm);

u8 is_ecc_enabled(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat);

/*See mctAutoInitMCT header for index relationships to CL and T*/
static const u16 table_f_k[]	= {00,200,266,333,400,533 };
static const u8 tab_bank_addr[]	= {0x3F,0x01,0x09,0x3F,0x3F,0x11,0x0A,
				0x19,0x12,0x1A,0x21,0x22,0x23};
const u8 table_dqs_rcv_en_offset[] = {0x00,0x01,0x10,0x11,0x2};

/****************************************************************************
 *  Describe how platform maps MemClk pins to logical DIMMs. The MemClk pins
 *  are identified based on BKDG definition of Fn2x88[MemClkDis] bitmap.
 *  AGESA will base on this value to disable unused MemClk to save power.
 *
 *  If MEMCLK_MAPPING or MEMCLK_MAPPING contains all zeroes, AGESA will use
 *  default MemClkDis setting based on package type.
 *
 *  Example:
 *  BKDG definition of Fn2x88[MemClkDis] bitmap for AM3 package is like below:
 *       Bit AM3/S1g3 pin name
 *       0   M[B,A]_CLK_H/L[0]
 *       1   M[B,A]_CLK_H/L[1]
 *       2   M[B,A]_CLK_H/L[2]
 *       3   M[B,A]_CLK_H/L[3]
 *       4   M[B,A]_CLK_H/L[4]
 *       5   M[B,A]_CLK_H/L[5]
 *       6   M[B,A]_CLK_H/L[6]
 *       7   M[B,A]_CLK_H/L[7]
 *
 *  And platform has the following routing:
 *       CS0   M[B,A]_CLK_H/L[4]
 *       CS1   M[B,A]_CLK_H/L[2]
 *       CS2   M[B,A]_CLK_H/L[3]
 *       CS3   M[B,A]_CLK_H/L[5]
 *
 *  Then:
 *                      ;    CS0        CS1        CS2        CS3        CS4        CS5        CS6        CS7
 *  MEMCLK_MAPPING  EQU    00010000b, 00000100b, 00001000b, 00100000b, 00000000b, 00000000b, 00000000b, 00000000b
*/

/* ==========================================================================================
 * Set up clock pin to dimm mappings,
 * NOTE: If you are not sure about the pin mappings, you can keep all MemClk signals active,
 * just set all entries in the relevant table(s) to 0xff.
 * ==========================================================================================
 */
static const u8 tab_l1_clk_dis[]  = {0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04};
static const u8 tab_am3_clk_dis[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static const u8 tab_s1_clk_dis[]  = {0xA2, 0xA2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* C32: Enable CS0 - CS3 clocks (DIMM0 - DIMM1) */
static const u8 tab_c32_clk_dis[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

/* G34: Enable CS0 - CS3 clocks (DIMM0 - DIMM1) */
static const u8 tab_g34_clk_dis[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

/* FM2: Enable all the clocks for the dimms */
static const u8 tab_fm2_clk_dis[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const u8 tab_manual_clk_dis[] = {0x10, 0x04, 0x08, 0x20, 0x00, 0x00, 0x00, 0x00};
/* ========================================================================================== */

static const u8 table_comp_rise_slew_20x[] = {7, 3, 2, 2, 0xFF};
static const u8 table_comp_rise_slew_15x[] = {7, 7, 3, 2, 0xFF};
static const u8 table_comp_fall_slew_20x[] = {7, 5, 3, 2, 0xFF};
static const u8 table_comp_fall_slew_15x[] = {7, 7, 5, 3, 0xFF};

u8 dct_ddr_voltage_index(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 dimm;
	u8 ddr_voltage_index = 0;

	/* If no DIMMs are present on this DCT, report 1.5V operation and skip checking the hardware */
	if (p_dct_stat->dimm_valid_dct[dct] == 0)
		return 0x1;

	/* Find current DDR supply voltage for this DCT */
	for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm++) {
		if (p_dct_stat->dimm_valid_dct[dct] & (1 << dimm))
			ddr_voltage_index |= p_dct_stat->dimm_configured_voltage[dimm];
	}
	if (ddr_voltage_index > 0x7) {
		printk(BIOS_DEBUG,
			"%s: Insufficient DDR supply voltage indicated!  Configuring processor for 1.25V operation, but this attempt may fail...\n",
			__func__);
		ddr_voltage_index = 0x4;
	}
	if (ddr_voltage_index == 0x0) {
		printk(BIOS_DEBUG,
			"%s: No DDR supply voltage indicated!  Configuring processor for 1.5V operation, but this attempt may fail...\n",
			__func__);
		ddr_voltage_index = 0x1;
	}

	return ddr_voltage_index;
}

static u16 fam15h_mhz_to_memclk_config(u16 freq)
{
	u16 fam15h_freq_tab[] = {0, 0, 0, 0, 333, 0, 400, 0, 0, 0, 533,
				0, 0, 0, 667, 0, 0, 0, 800, 0, 0, 0, 933};
	u16 iter;

	/* Compute the index value for the given frequency */
	for (iter = 0; iter <= 0x16; iter++) {
		if (fam15h_freq_tab[iter] == freq) {
			freq = iter;
			break;
		}
	}
	if (freq == 0)
		freq = 0x4;

	return freq;
}

static u16 fam10h_mhz_to_memclk_config(u16 freq)
{
	u16 fam10h_freq_tab[] = {0, 0, 0, 400, 533, 667, 800};
	u16 iter;

	/* Compute the index value for the given frequency */
	for (iter = 0; iter <= 0x6; iter++) {
		if (fam10h_freq_tab[iter] == freq) {
			freq = iter;
			break;
		}
	}
	if (freq == 0)
		freq = 0x3;

	return freq;
}

static inline u8 is_model10_1f(void)
{
	u8 model101f = 0;
	u32 family;

	family = cpuid_eax(0x80000001);
	family = ((family & 0x0ff000) >> 12);

	if (family >= 0x10 && family <= 0x1f)
		/* Model 0x10 to 0x1f */
		model101f = 1;

	return model101f;
}

u8 is_ecc_enabled(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat)
{
	u8 ecc_enabled = 1;

	if (!p_mct_stat->try_ecc)
		ecc_enabled = 0;

	if (p_dct_stat->node_present
	&& (p_dct_stat->dimm_valid_dct[0] || p_dct_stat->dimm_valid_dct[1]))
		if (!(p_dct_stat->status & (1 << SB_ECC_DIMMS)))
			ecc_enabled = 0;

	return !!ecc_enabled;
}

u8 get_available_lane_count(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat)
{
	if (is_ecc_enabled(p_mct_stat, p_dct_stat))
		return 9;
	else
		return 8;
}

u16 mhz_to_memclk_config(u16 freq)
{
	if (is_fam15h())
		return fam15h_mhz_to_memclk_config(freq);
	else
		return fam10h_mhz_to_memclk_config(freq) + 1;
}

u32 fam10h_address_timing_compensation_code(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	u8 package_type;
	u32 calibration_code = 0;

	package_type = mct_get_nv_bits(NV_PACK_TYPE);
	u16 mem_clk_freq = (get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x7) + 1;

	/* Obtain number of DIMMs on channel */
	u8 dimm_count = p_dct_stat->ma_dimms[dct];
	u8 rank_count_dimm0;
	u8 rank_count_dimm1;

	if (package_type == PT_GR) {
		/* Socket G34 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam10h BKDG Rev. 3.62 section 2.8.9.5.8 Tables 60 - 61 */
			if (max_dimms_installable == 1) {
				if (mem_clk_freq == 0x4) {
					/* DDR3-800 */
					calibration_code = 0x00000000;
				} else if (mem_clk_freq == 0x5) {
					/* DDR3-1066 */
					calibration_code = 0x003c3c3c;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-1333 */
					calibration_code = 0x003a3a3a;
				}
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					if (mem_clk_freq == 0x4) {
						/* DDR3-800 */
						calibration_code = 0x00000000;
					} else if (mem_clk_freq == 0x5) {
						/* DDR3-1066 */
						calibration_code = 0x003c3c3c;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-1333 */
						calibration_code = 0x003a3a3a;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					if (mem_clk_freq == 0x4) {
						/* DDR3-800 */
						calibration_code = 0x00000000;
					} else if (mem_clk_freq == 0x5) {
						/* DDR3-1066 */
						calibration_code = 0x003a3c3a;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-1333 */
						calibration_code = 0x00383a38;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		} else {
			/* UDIMM */
			/* Fam10h BKDG Rev. 3.62 section 2.8.9.5.8 Table 56 */
			if (dimm_count == 1) {
				/* 1 dimm detected */
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (mem_clk_freq == 0x4) {
					/* DDR3-800 */
					if (rank_count_dimm0 == 1)
						calibration_code = 0x00000000;
					else
						calibration_code = 0x003b0000;
				} else if (mem_clk_freq == 0x5) {
					/* DDR3-1066 */
					if (rank_count_dimm0 == 1)
						calibration_code = 0x00000000;
					else
						calibration_code = 0x00380000;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-1333 */
					if (rank_count_dimm0 == 1)
						calibration_code = 0x00000000;
					else
						calibration_code = 0x00360000;
				}
			} else if (dimm_count == 2) {
				/* 2 DIMMs detected */
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
				rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (mem_clk_freq == 0x4) {
					/* DDR3-800 */
					calibration_code = 0x00390039;
				} else if (mem_clk_freq == 0x5) {
					/* DDR3-1066 */
					calibration_code = 0x00350037;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-1333 */
					calibration_code = 0x00000035;
				}
			}
		}
	} else {
		/* TODO
		 * Other socket support unimplemented
		 */
	}

	return calibration_code;
}

static u32 fam15h_phy_predriver_calibration_code(struct DCTStatStruc *p_dct_stat, u8 dct,
						u8 drive_strength)
{
	u8 lrdimm = 0;
	u8 package_type;
	u8 ddr_voltage_index;
	u32 calibration_code = 0;
	u16 mem_clk_freq = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x1f;

	ddr_voltage_index = dct_ddr_voltage_index(p_dct_stat, dct);
	package_type = mct_get_nv_bits(NV_PACK_TYPE);

	if (!lrdimm) {
		/* Not an LRDIMM */
		if ((package_type == PT_M2) || (package_type == PT_GR)) {
			/* Socket AM3 or G34 */
			if (ddr_voltage_index & 0x4) {
				/* 1.25V */
				/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 43 */
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 - DDR3-800 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xb6d;
					else if (drive_strength == 0x2)
						calibration_code = 0x924;
					else if (drive_strength == 0x3)
						calibration_code = 0x6db;
				} else if ((mem_clk_freq == 0xa) || (mem_clk_freq == 0xe)) {
					/* DDR3-1066 - DDR3-1333 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xfff;
					else if (drive_strength == 0x2)
						calibration_code = 0xdb6;
					else if (drive_strength == 0x3)
						calibration_code = 0x924;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xfff;
					else if (drive_strength == 0x2)
						calibration_code = 0xfff;
					else if (drive_strength == 0x3)
						calibration_code = 0xfff;
				}
			} else if (ddr_voltage_index & 0x2) {
				/* 1.35V */
				/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 42 */
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 - DDR3-800 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0x924;
					else if (drive_strength == 0x2)
						calibration_code = 0x6db;
					else if (drive_strength == 0x3)
						calibration_code = 0x492;
				} else if ((mem_clk_freq == 0xa) || (mem_clk_freq == 0xe)) {
					/* DDR3-1066 - DDR3-1333 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xdb6;
					else if (drive_strength == 0x2)
						calibration_code = 0xbd6;
					else if (drive_strength == 0x3)
						calibration_code = 0x6db;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xfff;
					else if (drive_strength == 0x2)
						calibration_code = 0xfff;
					else if (drive_strength == 0x3)
						calibration_code = 0xdb6;
				}
			} else if (ddr_voltage_index & 0x1) {
				/* 1.5V */
				/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 41 */
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 - DDR3-800 */
					if (drive_strength == 0x0)
						calibration_code = 0xb6d;
					else if (drive_strength == 0x1)
						calibration_code = 0x6db;
					else if (drive_strength == 0x2)
						calibration_code = 0x492;
					else if (drive_strength == 0x3)
						calibration_code = 0x492;
				} else if ((mem_clk_freq == 0xa) || (mem_clk_freq == 0xe)) {
					/* DDR3-1066 - DDR3-1333 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0x924;
					else if (drive_strength == 0x2)
						calibration_code = 0x6db;
					else if (drive_strength == 0x3)
						calibration_code = 0x6db;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xfff;
					else if (drive_strength == 0x2)
						calibration_code = 0xfff;
					else if (drive_strength == 0x3)
						calibration_code = 0xb6d;
				}
			}
		} else if (package_type == PT_C3) {
			/* Socket C32 */
			if (ddr_voltage_index & 0x4) {
				/* 1.25V */
				/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 46 */
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 - DDR3-800 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xb6d;
					else if (drive_strength == 0x2)
						calibration_code = 0x924;
					else if (drive_strength == 0x3)
						calibration_code = 0x6db;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xfff;
					else if (drive_strength == 0x2)
						calibration_code = 0xdb6;
					else if (drive_strength == 0x3)
						calibration_code = 0x924;
				} else if (mem_clk_freq == 0xe) {
					/* DDR3-1333 */
					if (drive_strength == 0x0)
						calibration_code = 0xb6d;
					else if (drive_strength == 0x1)
						calibration_code = 0x6db;
					else if (drive_strength == 0x2)
						calibration_code = 0x492;
					else if (drive_strength == 0x3)
						calibration_code = 0x492;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xfff;
					else if (drive_strength == 0x2)
						calibration_code = 0xfff;
					else if (drive_strength == 0x3)
						calibration_code = 0xfff;
				}
			} else if (ddr_voltage_index & 0x2) {
				/* 1.35V */
				/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 45 */
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 - DDR3-800 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0x924;
					else if (drive_strength == 0x2)
						calibration_code = 0x6db;
					else if (drive_strength == 0x3)
						calibration_code = 0x492;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xdb6;
					else if (drive_strength == 0x2)
						calibration_code = 0xb6d;
					else if (drive_strength == 0x3)
						calibration_code = 0x6db;
				} else if (mem_clk_freq == 0xe) {
					/* DDR3-1333 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0x924;
					else if (drive_strength == 0x2)
						calibration_code = 0x6db;
					else if (drive_strength == 0x3)
						calibration_code = 0x492;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xfff;
					else if (drive_strength == 0x2)
						calibration_code = 0xfff;
					else if (drive_strength == 0x3)
						calibration_code = 0xdb6;
				}
			} else if (ddr_voltage_index & 0x1) {
				/* 1.5V */
				/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 44 */
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 - DDR3-800 */
					if (drive_strength == 0x0)
						calibration_code = 0xb6d;
					else if (drive_strength == 0x1)
						calibration_code = 0x6db;
					else if (drive_strength == 0x2)
						calibration_code = 0x492;
					else if (drive_strength == 0x3)
						calibration_code = 0x492;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0x924;
					else if (drive_strength == 0x2)
						calibration_code = 0x6db;
					else if (drive_strength == 0x3)
						calibration_code = 0x6db;
				} else if (mem_clk_freq == 0xe) {
					/* DDR3-1333 */
					if (drive_strength == 0x0)
						calibration_code = 0xb6d;
					else if (drive_strength == 0x1)
						calibration_code = 0x6db;
					else if (drive_strength == 0x2)
						calibration_code = 0x492;
					else if (drive_strength == 0x3)
						calibration_code = 0x492;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					if (drive_strength == 0x0)
						calibration_code = 0xfff;
					else if (drive_strength == 0x1)
						calibration_code = 0xfff;
					else if (drive_strength == 0x2)
						calibration_code = 0xfff;
					else if (drive_strength == 0x3)
						calibration_code = 0xb6d;
				}
			}
		} else if (package_type == PT_FM2) {
			/* Socket FM2 */
			if (ddr_voltage_index & 0x1) {
				/* 1.5V */
				/* Fam15h BKDG Rev. 3.12 section 2.9.5.4.4 Table 22 */
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 - DDR3-800 */
					calibration_code = 0xb24;
				} else if (mem_clk_freq >= 0xa) {
					/* DDR3-1066 or higher */
					calibration_code = 0xff6;
				}
			}
		}
	} else {
		/* LRDIMM */

		/* TODO
		 * Implement LRDIMM support
		 * See Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Tables 47 - 49
		 */
	}

	return calibration_code;
}

static u32 fam15h_phy_predriver_cmd_addr_calibration_code(struct DCTStatStruc *p_dct_stat,
							u8 dct, u8 drive_strength)
{
	u8 ddr_voltage_index;
	u32 calibration_code = 0;
	u16 mem_clk_freq = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x1f;

	ddr_voltage_index = dct_ddr_voltage_index(p_dct_stat, dct);

	if (ddr_voltage_index & 0x4) {
		/* 1.25V */
		/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 52 */
		if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
			/* DDR3-667 - DDR3-800 */
			if (drive_strength == 0x0)
				calibration_code = 0x492;
			else if (drive_strength == 0x1)
				calibration_code = 0x492;
			else if (drive_strength == 0x2)
				calibration_code = 0x492;
			else if (drive_strength == 0x3)
				calibration_code = 0x492;
		} else if ((mem_clk_freq == 0xa) || (mem_clk_freq == 0xe)) {
			/* DDR3-1066 - DDR3-1333 */
			if (drive_strength == 0x0)
				calibration_code = 0xdad;
			else if (drive_strength == 0x1)
				calibration_code = 0x924;
			else if (drive_strength == 0x2)
				calibration_code = 0x6db;
			else if (drive_strength == 0x3)
				calibration_code = 0x492;
		} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
			/* DDR3-1600 - DDR3-1866 */
			if (drive_strength == 0x0)
				calibration_code = 0xff6;
			else if (drive_strength == 0x1)
				calibration_code = 0xdad;
			else if (drive_strength == 0x2)
				calibration_code = 0xb64;
			else if (drive_strength == 0x3)
				calibration_code = 0xb64;
		}
	} else if (ddr_voltage_index & 0x2) {
		/* 1.35V */
		/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 51 */
		if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
			/* DDR3-667 - DDR3-800 */
			if (drive_strength == 0x0)
				calibration_code = 0x492;
			else if (drive_strength == 0x1)
				calibration_code = 0x492;
			else if (drive_strength == 0x2)
				calibration_code = 0x492;
			else if (drive_strength == 0x3)
				calibration_code = 0x492;
		} else if ((mem_clk_freq == 0xa) || (mem_clk_freq == 0xe)) {
			/* DDR3-1066 - DDR3-1333 */
			if (drive_strength == 0x0)
				calibration_code = 0x924;
			else if (drive_strength == 0x1)
				calibration_code = 0x6db;
			else if (drive_strength == 0x2)
				calibration_code = 0x6db;
			else if (drive_strength == 0x3)
				calibration_code = 0x6db;
		} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
			/* DDR3-1600 - DDR3-1866 */
			if (drive_strength == 0x0)
				calibration_code = 0xb6d;
			else if (drive_strength == 0x1)
				calibration_code = 0xb6d;
			else if (drive_strength == 0x2)
				calibration_code = 0x924;
			else if (drive_strength == 0x3)
				calibration_code = 0x924;
		}
	} else if (ddr_voltage_index & 0x1) {
		/* 1.5V */
		/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 50 */
		if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
			/* DDR3-667 - DDR3-800 */
			if (drive_strength == 0x0)
				calibration_code = 0x492;
			else if (drive_strength == 0x1)
				calibration_code = 0x492;
			else if (drive_strength == 0x2)
				calibration_code = 0x492;
			else if (drive_strength == 0x3)
				calibration_code = 0x492;
		} else if ((mem_clk_freq == 0xa) || (mem_clk_freq == 0xe)) {
			/* DDR3-1066 - DDR3-1333 */
			if (drive_strength == 0x0)
				calibration_code = 0x6db;
			else if (drive_strength == 0x1)
				calibration_code = 0x6db;
			else if (drive_strength == 0x2)
				calibration_code = 0x6db;
			else if (drive_strength == 0x3)
				calibration_code = 0x6db;
		} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
			/* DDR3-1600 - DDR3-1866 */
			if (drive_strength == 0x0)
				calibration_code = 0xb6d;
			else if (drive_strength == 0x1)
				calibration_code = 0xb6d;
			else if (drive_strength == 0x2)
				calibration_code = 0xb6d;
			else if (drive_strength == 0x3)
				calibration_code = 0xb6d;
		}
	}

	return calibration_code;
}

static u32 fam15h_phy_predriver_clk_calibration_code(struct DCTStatStruc *p_dct_stat, u8 dct,
						u8 drive_strength)
{
	u8 ddr_voltage_index;
	u32 calibration_code = 0;
	u16 mem_clk_freq = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x1f;

	ddr_voltage_index = dct_ddr_voltage_index(p_dct_stat, dct);

	if (ddr_voltage_index & 0x4) {
		/* 1.25V */
		/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 55 */
		if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
			/* DDR3-667 - DDR3-800 */
			if (drive_strength == 0x0)
				calibration_code = 0xdad;
			else if (drive_strength == 0x1)
				calibration_code = 0xdad;
			else if (drive_strength == 0x2)
				calibration_code = 0x924;
			else if (drive_strength == 0x3)
				calibration_code = 0x924;
		} else if ((mem_clk_freq == 0xa) || (mem_clk_freq == 0xe)) {
			/* DDR3-1066 - DDR3-1333 */
			if (drive_strength == 0x0)
				calibration_code = 0xff6;
			else if (drive_strength == 0x1)
				calibration_code = 0xff6;
			else if (drive_strength == 0x2)
				calibration_code = 0xff6;
			else if (drive_strength == 0x3)
				calibration_code = 0xff6;
		} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
			/* DDR3-1600 - DDR3-1866 */
			if (drive_strength == 0x0)
				calibration_code = 0xff6;
			else if (drive_strength == 0x1)
				calibration_code = 0xff6;
			else if (drive_strength == 0x2)
				calibration_code = 0xff6;
			else if (drive_strength == 0x3)
				calibration_code = 0xff6;
		}
	} else if (ddr_voltage_index & 0x2) {
		/* 1.35V */
		/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 54 */
		if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
			/* DDR3-667 - DDR3-800 */
			if (drive_strength == 0x0)
				calibration_code = 0xdad;
			else if (drive_strength == 0x1)
				calibration_code = 0xdad;
			else if (drive_strength == 0x2)
				calibration_code = 0x924;
			else if (drive_strength == 0x3)
				calibration_code = 0x924;
		} else if ((mem_clk_freq == 0xa) || (mem_clk_freq == 0xe)) {
			/* DDR3-1066 - DDR3-1333 */
			if (drive_strength == 0x0)
				calibration_code = 0xff6;
			else if (drive_strength == 0x1)
				calibration_code = 0xff6;
			else if (drive_strength == 0x2)
				calibration_code = 0xff6;
			else if (drive_strength == 0x3)
				calibration_code = 0xdad;
		} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
			/* DDR3-1600 - DDR3-1866 */
			if (drive_strength == 0x0)
				calibration_code = 0xff6;
			else if (drive_strength == 0x1)
				calibration_code = 0xff6;
			else if (drive_strength == 0x2)
				calibration_code = 0xff6;
			else if (drive_strength == 0x3)
				calibration_code = 0xdad;
		}
	} else if (ddr_voltage_index & 0x1) {
		/* 1.5V */
		/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 53 */
		if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
			/* DDR3-667 - DDR3-800 */
			if (drive_strength == 0x0)
				calibration_code = 0x924;
			else if (drive_strength == 0x1)
				calibration_code = 0x924;
			else if (drive_strength == 0x2)
				calibration_code = 0x924;
			else if (drive_strength == 0x3)
				calibration_code = 0x924;
		} else if ((mem_clk_freq == 0xa) || (mem_clk_freq == 0xe)) {
			/* DDR3-1066 - DDR3-1333 */
			if (drive_strength == 0x0)
				calibration_code = 0xff6;
			else if (drive_strength == 0x1)
				calibration_code = 0xff6;
			else if (drive_strength == 0x2)
				calibration_code = 0xff6;
			else if (drive_strength == 0x3)
				calibration_code = 0xb6d;
		} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
			/* DDR3-1600 - DDR3-1866 */
			if (drive_strength == 0x0)
				calibration_code = 0xff6;
			else if (drive_strength == 0x1)
				calibration_code = 0xff6;
			else if (drive_strength == 0x2)
				calibration_code = 0xff6;
			else if (drive_strength == 0x3)
				calibration_code = 0xff6;
		}
	}

	return calibration_code;
}

u32 fam15h_output_driver_compensation_code(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	u8 package_type;
	u32 calibration_code = 0;

	package_type = mct_get_nv_bits(NV_PACK_TYPE);
	u16 mem_clk_freq = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x1f;

	/* Obtain number of DIMMs on channel */
	u8 dimm_count = p_dct_stat->ma_dimms[dct];
	u8 rank_count_dimm0;
	u8 rank_count_dimm1;

	if (package_type == PT_GR) {
		/* Socket G34 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 74 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (mem_clk_freq == 0x4) {
					/* DDR3-667 */
					calibration_code = 0x00112222;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-800 */
					calibration_code = 0x10112222;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					calibration_code = 0x20112222;
				} else if ((mem_clk_freq == 0xe) || (mem_clk_freq == 0x12)) {
					/* DDR3-1333 - DDR3-1600 */
					calibration_code = 0x30112222;
				} else if (mem_clk_freq == 0x16) {
					/* DDR3-1866 */
					calibration_code = 0x30332222;
				}

				if (rank_count_dimm0 == 4) {
					calibration_code &= ~(0xff << 16);
					calibration_code |= 0x22 << 16;
				}
			} else if (max_dimms_installable == 2) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
				rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (dimm_count == 1) {
					/* 1 dimm detected */
					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x00112222;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x10112222;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x20112222;
					} else if ((mem_clk_freq == 0xe)
					|| (mem_clk_freq == 0x12)) {
						/* DDR3-1333 - DDR3-1600 */
						calibration_code = 0x30112222;
					}

					if ((rank_count_dimm0 == 4)
					|| (rank_count_dimm1 == 4)) {
						calibration_code &= ~(0xff << 16);
						calibration_code |= 0x22 << 16;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x10222222;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x20222222;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x30222222;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x30222222;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						calibration_code = 0x30222222;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */
			/* TODO
			 * LRDIMM support unimplemented
			 */
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 73 */
			if (max_dimms_installable == 1) {
				if (mem_clk_freq == 0x4) {
					/* DDR3-667 */
					calibration_code = 0x00112222;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-800 */
					calibration_code = 0x10112222;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					calibration_code = 0x20112222;
				} else if ((mem_clk_freq == 0xe) || (mem_clk_freq == 0x12)) {
					/* DDR3-1333 - DDR3-1600 */
					calibration_code = 0x30112222;
				} else if (mem_clk_freq == 0x16) {
					/* DDR3-1866 */
					calibration_code = 0x30332222;
				}
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x00112222;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x10112222;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x20112222;
					} else if ((mem_clk_freq == 0xe)
					|| (mem_clk_freq == 0x12)) {
						/* DDR3-1333 - DDR3-1600 */
						calibration_code = 0x30112222;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x10222222;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x20222222;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x30222222;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x30222222;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						if ((rank_count_dimm0 == 1)
						&& (rank_count_dimm1 == 1))
							calibration_code = 0x30222222;
						else
							calibration_code = 0x30112222;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_C3) {
		/* Socket C32 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 77 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (mem_clk_freq == 0x4) {
					/* DDR3-667 */
					calibration_code = 0x00112222;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-800 */
					calibration_code = 0x10112222;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					calibration_code = 0x20112222;
				} else if ((mem_clk_freq == 0xe) || (mem_clk_freq == 0x12)) {
					/* DDR3-1333 - DDR3-1600 */
					calibration_code = 0x30112222;
				}

				if (rank_count_dimm0 == 4) {
					calibration_code &= ~(0xff << 16);
					calibration_code |= 0x22 << 16;
				}
			} else if (max_dimms_installable == 2) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
				rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (dimm_count == 1) {
					/* 1 dimm detected */
					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x00112222;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x10112222;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x20112222;
					} else if ((mem_clk_freq == 0xe)
						|| (mem_clk_freq == 0x12)) {
						/* DDR3-1333 - DDR3-1600 */
						calibration_code = 0x30112222;
					}

					if ((rank_count_dimm0 == 4)
					|| (rank_count_dimm1 == 4)) {
						calibration_code &= ~(0xff << 16);
						calibration_code |= 0x22 << 16;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x10222222;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x20222222;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x30222222;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x30222222;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						calibration_code = 0x30222222;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */
			/* TODO
			 * LRDIMM support unimplemented
			 */
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 73 */
			if (max_dimms_installable == 1) {
				if (mem_clk_freq == 0x4) {
					/* DDR3-667 */
					calibration_code = 0x00112222;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-800 */
					calibration_code = 0x10112222;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					calibration_code = 0x20112222;
				} else if ((mem_clk_freq == 0xe) || (mem_clk_freq == 0x12)) {
					/* DDR3-1333 - DDR3-1600 */
					calibration_code = 0x30112222;
				}
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x00112222;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x10112222;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x20112222;
					} else if ((mem_clk_freq == 0xe)
						|| (mem_clk_freq == 0x12)) {
						/* DDR3-1333 - DDR3-1600 */
						calibration_code = 0x30112222;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x10222222;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x20222222;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x30222222;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x30222222;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						if ((rank_count_dimm0 == 1)
						&& (rank_count_dimm1 == 1))
							calibration_code = 0x30222222;
						else
							calibration_code = 0x30112222;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_FM2) {
		/* Socket FM2 */
		/* Assume UDIMM */
		/* Fam15h Model10h BKDG Rev. 3.12 section 2.9.5.6.6 Table 32 */
		if (max_dimms_installable == 1) {
			rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

			if (mem_clk_freq == 0x4) {
				/* DDR3-667 */
				calibration_code = 0x00112222;
			} else if (mem_clk_freq == 0x6) {
				/* DDR3-800 */
				calibration_code = 0x10112222;
			} else if (mem_clk_freq == 0xa) {
				/* DDR3-1066 */
				calibration_code = 0x20112222;
			} else if (mem_clk_freq >= 0xe) {
				/* DDR3-1333 or higher */
				calibration_code = 0x30112222;
			}
		} else if (max_dimms_installable == 2) {
			rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
			rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

			if (dimm_count == 1) {
				/* 1 dimm detected */
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 or DDR3-800 */
					calibration_code = 0x00112222;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					calibration_code = 0x10112222;
				} else if (mem_clk_freq == 0xe) {
					/* DDR3-1333 */
					calibration_code = 0x20112222;
				} else if (mem_clk_freq >= 0x12) {
					/* DDR3-1600 or higher */
					calibration_code = 0x30112222;
				}
			} else if (dimm_count == 2) {
				/* 2 DIMMs detected */
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
				rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (mem_clk_freq == 0x4) {
					/* DDR3-667 */
					calibration_code = 0x10222322;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-800 */
					calibration_code = 0x20222322;
				} else if (mem_clk_freq >= 0xa) {
					/* DDR3-1066 or higher */
					calibration_code = 0x30222322;
				}
			}
		} else if (max_dimms_installable == 3) {
			/* TODO
			 * 3 dimm/channel support unimplemented
			 */
		}
	} else {
		/* TODO
		 * Other socket support unimplemented
		 */
	}

	return calibration_code;
}

u32 fam15h_address_timing_compensation_code(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	u8 package_type;
	u32 calibration_code = 0;

	package_type = mct_get_nv_bits(NV_PACK_TYPE);
	u16 mem_clk_freq = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x1f;

	/* Obtain number of DIMMs on channel */
	u8 dimm_count = p_dct_stat->ma_dimms[dct];
	u8 rank_count_dimm0;
	u8 rank_count_dimm1;

	if (package_type == PT_GR) {
		/* Socket G34 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 74 */
			if (max_dimms_installable == 1) {
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 - DDR3-800 */
					calibration_code = 0x00000000;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					calibration_code = 0x003c3c3c;
				} else if (mem_clk_freq == 0xe) {
					/* DDR3-1333 */
					calibration_code = 0x003a3a3a;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					calibration_code = 0x00393939;
				}
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						calibration_code = 0x00000000;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x00393c39;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x00373a37;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						calibration_code = 0x00363936;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						calibration_code = 0x00000000;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x003a3c3a;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x00383a38;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						calibration_code = 0x00353935;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */
			/* TODO
			 * LRDIMM support unimplemented
			 */
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 76 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (mem_clk_freq == 0x4) {
					/* DDR3-667 */
					if (rank_count_dimm0 == 1)
						calibration_code = 0x00000000;
					else
						calibration_code = 0x003b0000;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-800 */
					if (rank_count_dimm0 == 1)
						calibration_code = 0x00000000;
					else
						calibration_code = 0x003b0000;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					calibration_code = 0x00383837;
				} else if (mem_clk_freq == 0xe) {
					/* DDR3-1333 */
					calibration_code = 0x00363635;
				} else if (mem_clk_freq == 0x12) {
					/* DDR3-1600 */
					if (rank_count_dimm0 == 1)
						calibration_code = 0x00353533;
					else
						calibration_code = 0x00003533;
				} else if (mem_clk_freq == 0x16) {
					/* DDR3-1866 */
					calibration_code = 0x00333330;
				}
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						if (rank_count_dimm0 == 1)
							calibration_code = 0x00000000;
						else
							calibration_code = 0x003b0000;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						if (rank_count_dimm0 == 1)
							calibration_code = 0x00000000;
						else
							calibration_code = 0x003b0000;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x00383837;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x00363635;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						if (rank_count_dimm0 == 1)
							calibration_code = 0x00353533;
						else
							calibration_code = 0x00003533;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x00390039;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x00390039;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x003a3a3a;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x00003939;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						if ((rank_count_dimm0 == 1)
						&& (rank_count_dimm1 == 1))
							calibration_code = 0x00003738;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_C3) {
		/* Socket C32 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 77 */
			if (max_dimms_installable == 1) {
				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
					/* DDR3-667 - DDR3-800 */
					calibration_code = 0x00000000;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					calibration_code = 0x003c3c3c;
				} else if (mem_clk_freq == 0xe) {
					/* DDR3-1333 */
					calibration_code = 0x003a3a3a;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					calibration_code = 0x00393939;
				}
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						calibration_code = 0x00000000;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x00393c39;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x00373a37;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						calibration_code = 0x00363936;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						calibration_code = 0x00000000;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x003a3c3a;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x00383a38;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						calibration_code = 0x00353935;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */
			/* TODO
			 * LRDIMM support unimplemented
			 */
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 76 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (mem_clk_freq == 0x4) {
					/* DDR3-667 */
					if (rank_count_dimm0 == 1)
						calibration_code = 0x00000000;
					else
						calibration_code = 0x003b0000;
				} else if (mem_clk_freq == 0x6) {
					/* DDR3-800 */
					if (rank_count_dimm0 == 1)
						calibration_code = 0x00000000;
					else
						calibration_code = 0x003b0000;
				} else if (mem_clk_freq == 0xa) {
					/* DDR3-1066 */
					calibration_code = 0x00383837;
				} else if (mem_clk_freq == 0xe) {
					/* DDR3-1333 */
					calibration_code = 0x00363635;
				} else if (mem_clk_freq == 0x12) {
					/* DDR3-1600 */
					if (rank_count_dimm0 == 1)
						calibration_code = 0x00353533;
					else
						calibration_code = 0x00003533;
				} else if (mem_clk_freq == 0x16) {
					/* DDR3-1866 */
					calibration_code = 0x00333330;
				}
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						if (rank_count_dimm0 == 1)
							calibration_code = 0x00000000;
						else
							calibration_code = 0x003b0000;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						if (rank_count_dimm0 == 1)
							calibration_code = 0x00000000;
						else
							calibration_code = 0x003b0000;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x00383837;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x00363635;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						if (rank_count_dimm0 == 1)
							calibration_code = 0x00353533;
						else
							calibration_code = 0x00003533;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (mem_clk_freq == 0x4) {
						/* DDR3-667 */
						calibration_code = 0x00390039;
					} else if (mem_clk_freq == 0x6) {
						/* DDR3-800 */
						calibration_code = 0x00390039;
					} else if (mem_clk_freq == 0xa) {
						/* DDR3-1066 */
						calibration_code = 0x003a3a3a;
					} else if (mem_clk_freq == 0xe) {
						/* DDR3-1333 */
						calibration_code = 0x00003939;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						if ((rank_count_dimm0 == 1)
						&& (rank_count_dimm1 == 1))
							calibration_code = 0x00003738;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_FM2) {
		/* Socket FM2 */
		/* Assume UDIMM */
		/* Fam15h Model10h BKDG Rev. 3.12 section 2.9.5.6.6 Table 32 */
		if (dimm_count == 1) {
			/* 1 dimm detected */
			rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

			if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
				/* DDR3-667 or DDR3-800 */
				if (rank_count_dimm0 == 1)
					calibration_code = 0x00000000;
				else
					calibration_code = 0x003b0000;
			} else if (mem_clk_freq == 0xa) {
				/* DDR3-1066 */
				if (rank_count_dimm0 == 1)
					calibration_code = 0x00000000;
				else
					calibration_code = 0x00380000;
			} else if (mem_clk_freq == 0xe) {
				/* DDR3-1333 */
				if (rank_count_dimm0 == 1)
					calibration_code = 0x00000000;
				else
					calibration_code = 0x00360000;
			} else if (mem_clk_freq >= 0x12) {
				/* DDR3-1600 or higher */
				calibration_code = 0x00000000;
			}

		} else if (dimm_count == 2) {
			/* 2 DIMMs detected */
			rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
			rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

			if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)) {
				/* DDR3-667 or DDR3-800 */
				calibration_code = 0x00390039;
			} else if (mem_clk_freq == 0xa) {
				/* DDR3-1066 */
				calibration_code = 0x00350037;
			} else if (mem_clk_freq == 0xe) {
				/* DDR3-1333 */
				calibration_code = 0x00000035;
			} else if (mem_clk_freq == 0x12) {
				/* DDR3-1600 */
				calibration_code = 0x0000002b;
			} else if (mem_clk_freq > 0x12) {
				/* DDR3-1866 or greater */
				calibration_code = 0x00000031;
			}
		} else if (max_dimms_installable == 3) {
			/* TODO
			 * 3 dimm/channel support unimplemented
			 */
		}
	} else {
		/* TODO
		 * Other socket support unimplemented
		 */
	}

	return calibration_code;
}

u8 fam15h_slow_access_mode(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	u8 package_type;
	u32 slow_access = 0;

	package_type = mct_get_nv_bits(NV_PACK_TYPE);
	u16 mem_clk_freq = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x1f;

	/* Obtain number of DIMMs on channel */
	u8 dimm_count = p_dct_stat->ma_dimms[dct];
	u8 rank_count_dimm0;
	u8 rank_count_dimm1;

	if (package_type == PT_GR) {
		/* Socket G34 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 74 */
			slow_access = 0;
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 75 */
			slow_access = 0;
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 73 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)
					|| (mem_clk_freq == 0xa) | (mem_clk_freq == 0xe)) {
					/* DDR3-667 - DDR3-1333 */
					slow_access = 0;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					if (rank_count_dimm0 == 1)
						slow_access = 0;
					else
						slow_access = 1;
				}
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)
					|| (mem_clk_freq == 0xa) | (mem_clk_freq == 0xe)) {
						/* DDR3-667 - DDR3-1333 */
						slow_access = 0;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						if (rank_count_dimm0 == 1)
							slow_access = 0;
						else
							slow_access = 1;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)
						|| (mem_clk_freq == 0xa)) {
						/* DDR3-667 - DDR3-1066 */
						slow_access = 0;
					} else if ((mem_clk_freq == 0xe)
						|| (mem_clk_freq == 0x12)) {
						/* DDR3-1333 - DDR3-1600 */
						slow_access = 1;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_C3) {
		/* Socket C32 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 77 */
			slow_access = 0;
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 78 */
			slow_access = 0;
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 Table 76 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)
					|| (mem_clk_freq == 0xa) | (mem_clk_freq == 0xe)) {
					/* DDR3-667 - DDR3-1333 */
					slow_access = 0;
				} else if ((mem_clk_freq == 0x12) || (mem_clk_freq == 0x16)) {
					/* DDR3-1600 - DDR3-1866 */
					if (rank_count_dimm0 == 1)
						slow_access = 0;
					else
						slow_access = 1;
				}
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)
					|| (mem_clk_freq == 0xa) | (mem_clk_freq == 0xe)) {
						/* DDR3-667 - DDR3-1333 */
						slow_access = 0;
					} else if (mem_clk_freq == 0x12) {
						/* DDR3-1600 */
						if (rank_count_dimm0 == 1)
							slow_access = 0;
						else
							slow_access = 1;
					}
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)
						|| (mem_clk_freq == 0xa)) {
						/* DDR3-667 - DDR3-1066 */
						slow_access = 0;
					} else if ((mem_clk_freq == 0xe)
					|| (mem_clk_freq == 0x12)) {
						/* DDR3-1333 - DDR3-1600 */
						slow_access = 1;
					}
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_FM2) {
		/* Socket FM2 */
		/* UDIMM */
		/* Fam15h Model10 BKDG Rev. 3.12 section 2.9.5.6.6 Table 32 */
		if (max_dimms_installable == 1) {
			rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

			if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)
				|| (mem_clk_freq == 0xa) | (mem_clk_freq == 0xe)) {
				/* DDR3-667 - DDR3-1333 */
				slow_access = 0;
			} else if (mem_clk_freq >= 0x12) {
				/* DDR3-1600 or higher */
				if (rank_count_dimm0 == 1)
					slow_access = 0;
				else
					slow_access = 1;
			}
		} else if (max_dimms_installable == 2) {
			if (dimm_count == 1) {
				/* 1 dimm detected */
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)
					|| (mem_clk_freq == 0xa) | (mem_clk_freq == 0xe)) {
					/* DDR3-667 - DDR3-1333 */
					slow_access = 0;
				} else if (mem_clk_freq >= 0x12) {
					/* DDR3-1600 or higher */
					if (rank_count_dimm0 == 1)
						slow_access = 0;
					else
						slow_access = 1;
				}
			} else if (dimm_count == 2) {
				/* 2 DIMMs detected */
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
				rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if ((mem_clk_freq == 0x4) || (mem_clk_freq == 0x6)
					|| (mem_clk_freq == 0xa)) {
					/* DDR3-667 - DDR3-1066 */
					slow_access = 0;
				} else if (mem_clk_freq >= 0xe) {
					/* DDR3-1333 or higher */
					slow_access = 1;
				}
			}
		} else if (max_dimms_installable == 3) {
			/* TODO
			 * 3 dimm/channel support unimplemented
			 */
		}
	} else {
		/* TODO
		 * Other socket support unimplemented
		 */
	}

	return slow_access;
}

static u8 fam15h_odt_tristate_enable_code(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	u8 package_type;
	u8 odt_tristate_code = 0;

	package_type = mct_get_nv_bits(NV_PACK_TYPE);

	/* Obtain number of DIMMs on channel */
	u8 dimm_count = p_dct_stat->ma_dimms[dct];
	u8 rank_count_dimm0;
	u8 rank_count_dimm1;

	if (package_type == PT_GR) {
		/* Socket G34 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 104 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (rank_count_dimm0 == 1)
					odt_tristate_code = 0xe;
				else
					odt_tristate_code = 0xa;
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (rank_count_dimm1 == 1)
						odt_tristate_code = 0xd;
					else
						odt_tristate_code = 0x5;
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 1))
						odt_tristate_code = 0xc;
					else if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 >= 2))
						odt_tristate_code = 0x4;
					else if ((rank_count_dimm0 >= 2)
					&& (rank_count_dimm1 == 1))
						odt_tristate_code = 0x8;
					else
						odt_tristate_code = 0x0;
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */

			/* TODO
			 * Implement LRDIMM support
			 * See Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 105
			 */
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 103 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (rank_count_dimm0 == 1)
					odt_tristate_code = 0xe;
				else
					odt_tristate_code = 0xa;
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (rank_count_dimm0 == 1)
						odt_tristate_code = 0xd;
					else
						odt_tristate_code = 0x5;
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 1))
						odt_tristate_code = 0xc;
					else if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 2))
						odt_tristate_code = 0x4;
					else if ((rank_count_dimm0 == 2)
					&& (rank_count_dimm1 == 1))
						odt_tristate_code = 0x8;
					else
						odt_tristate_code = 0x0;
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_C3) {
		/* Socket C32 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 107 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (rank_count_dimm0 == 1)
					odt_tristate_code = 0xe;
				else
					odt_tristate_code = 0xa;
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (rank_count_dimm1 == 1)
						odt_tristate_code = 0xd;
					else
						odt_tristate_code = 0x5;
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 1))
						odt_tristate_code = 0xc;
					else if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 >= 2))
						odt_tristate_code = 0x4;
					else if ((rank_count_dimm0 >= 2)
					&& (rank_count_dimm1 == 1))
						odt_tristate_code = 0x8;
					else
						odt_tristate_code = 0x0;
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */

			/* TODO
			 * Implement LRDIMM support
			 * See Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 108
			 */
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 106 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (rank_count_dimm0 == 1)
					odt_tristate_code = 0xe;
				else
					odt_tristate_code = 0xa;
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (rank_count_dimm0 == 1)
						odt_tristate_code = 0xd;
					else
						odt_tristate_code = 0x5;
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 1))
						odt_tristate_code = 0xc;
					else if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 2))
						odt_tristate_code = 0x4;
					else if ((rank_count_dimm0 == 2)
					&& (rank_count_dimm1 == 1))
						odt_tristate_code = 0x8;
					else
						odt_tristate_code = 0x0;
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_FM2) {
		/* Socket FM2 */
		/* UDIMM */
		odt_tristate_code = 0x0;
	} else {
		/* TODO
		 * Other socket support unimplemented
		 */
	}

	return odt_tristate_code;
}

static u8 fam15h_cs_tristate_enable_code(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	u8 package_type;
	u8 cs_tristate_code = 0;

	package_type = mct_get_nv_bits(NV_PACK_TYPE);

	/* Obtain number of DIMMs on channel */
	u8 dimm_count = p_dct_stat->ma_dimms[dct];
	u8 rank_count_dimm0;
	u8 rank_count_dimm1;

	if (package_type == PT_GR) {
		/* Socket G34 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 104 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (rank_count_dimm0 < 4)
					cs_tristate_code = 0xfc;
				else
					cs_tristate_code = 0xcc;
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (rank_count_dimm1 < 4)
						cs_tristate_code = 0xf3;
					else
						cs_tristate_code = 0x33;
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((rank_count_dimm0 < 4)
					&& (rank_count_dimm1 < 4))
						cs_tristate_code = 0xf0;
					else if ((rank_count_dimm0 < 4)
					&& (rank_count_dimm1 == 4))
						cs_tristate_code = 0x30;
					else if ((rank_count_dimm0 == 4)
					&& (rank_count_dimm1 < 4))
						cs_tristate_code = 0xc0;
					else
						cs_tristate_code = 0x0;
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */

			/* TODO
			 * Implement LRDIMM support
			 * See Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 105
			 */
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 103 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (rank_count_dimm0 == 1)
					cs_tristate_code = 0xfe;
				else
					cs_tristate_code = 0xfc;
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (rank_count_dimm0 == 1)
						cs_tristate_code = 0xfb;
					else
						cs_tristate_code = 0xf3;
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 1))
						cs_tristate_code = 0xfa;
					else if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 2))
						cs_tristate_code = 0xf2;
					else if ((rank_count_dimm0 == 2)
					&& (rank_count_dimm1 == 1))
						cs_tristate_code = 0xf8;
					else
						cs_tristate_code = 0xf0;
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_C3) {
		/* Socket C32 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 107 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (rank_count_dimm0 < 4)
					cs_tristate_code = 0xfc;
				else
					cs_tristate_code = 0xcc;
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (rank_count_dimm1 < 4)
						cs_tristate_code = 0xf3;
					else
						cs_tristate_code = 0x33;
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((rank_count_dimm0 < 4)
					&& (rank_count_dimm1 < 4))
						cs_tristate_code = 0xf0;
					else if ((rank_count_dimm0 < 4)
					&& (rank_count_dimm1 == 4))
						cs_tristate_code = 0x30;
					else if ((rank_count_dimm0 == 4)
					&& (rank_count_dimm1 < 4))
						cs_tristate_code = 0xc0;
					else
						cs_tristate_code = 0x0;
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */

			/* TODO
			 * Implement LRDIMM support
			 * See Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 108
			 */
		} else {
			/* UDIMM */
			/* Fam15h BKDG Rev. 3.14 section 2.10.5.10.1 Table 106 */
			if (max_dimms_installable == 1) {
				rank_count_dimm0 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

				if (rank_count_dimm0 == 1)
					cs_tristate_code = 0xfe;
				else
					cs_tristate_code = 0xfc;
			} else if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if (rank_count_dimm0 == 1)
						cs_tristate_code = 0xfb;
					else
						cs_tristate_code = 0xf3;
				} else if (dimm_count == 2) {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 1))
						cs_tristate_code = 0xfa;
					else if ((rank_count_dimm0 == 1)
					&& (rank_count_dimm1 == 2))
						cs_tristate_code = 0xf2;
					else if ((rank_count_dimm0 == 2)
					&& (rank_count_dimm1 == 1))
						cs_tristate_code = 0xf8;
					else
						cs_tristate_code = 0xf0;
				}
			} else if (max_dimms_installable == 3) {
				/* TODO
				 * 3 dimm/channel support unimplemented
				 */
			}
		}
	} else if (package_type == PT_FM2) {
		/* Socket FM2 */
		/* UDIMM */
		cs_tristate_code = 0x0;
	} else {
		/* TODO
		 * Other socket support unimplemented
		 */
	}

	return cs_tristate_code;
}

void set_2t_configuration(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	u32 dev;
	u32 reg;
	u32 dword;

	u8 enable_slow_access_mode = 0;
	dev = p_dct_stat->dev_dct;

	if (is_fam15h()) {
		if (p_dct_stat->_2t_mode)
			enable_slow_access_mode = 1;
	} else {
		if (p_dct_stat->_2t_mode == 2)
			enable_slow_access_mode = 1;
	}

	reg = 0x94;				/* DRAM Configuration High */
	dword = get_nb32_dct(dev, dct, reg);
	if (enable_slow_access_mode)
		dword |= (0x1 << 20);		/* Set 2T CMD mode */
	else
		dword &= ~(0x1 << 20);		/* Clear 2T CMD mode */
	set_nb32_dct(dev, dct, reg, dword);

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

void precise_ndelay_fam15(struct MCTStatStruc *p_mct_stat, u32 nanoseconds) {
	msr_t tsc_msr;
	u64 cycle_count = (((u64)p_mct_stat->tsc_freq) * nanoseconds) / 1000;
	u64 start_timestamp;
	u64 current_timestamp;

	tsc_msr = rdmsr(TSC_MSR);
	start_timestamp = (((u64)tsc_msr.hi) << 32) | tsc_msr.lo;
	do {
		tsc_msr = rdmsr(TSC_MSR);
		current_timestamp = (((u64)tsc_msr.hi) << 32) | tsc_msr.lo;
	} while ((current_timestamp - start_timestamp) < cycle_count);
}

void precise_memclk_delay_fam15(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct, u32 clocks) {
	u16 memclk_freq;
	u32 delay_ns;
	u16 fam15h_freq_tab[] = {0, 0, 0, 0, 333, 0, 400, 0, 0, 0, 533, 0,
				0, 0, 667, 0, 0, 0, 800, 0, 0, 0, 933};

	memclk_freq = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x1f;

	if (fam15h_freq_tab[memclk_freq] == 0) {
		printk(BIOS_DEBUG,
			"ERROR: precise_memclk_delay_fam15 for DCT %d (delay %d clocks) failed to obtain valid memory frequency!"
			" (p_dct_stat: %p p_dct_stat->dev_dct: %08x memclk_freq: %02x)\n",
			dct, clocks, p_dct_stat, p_dct_stat->dev_dct, memclk_freq);
	}
	delay_ns = (((u64)clocks * 1000) / fam15h_freq_tab[memclk_freq]);
	precise_ndelay_fam15(p_mct_stat, delay_ns);
}

static void read_spd_bytes(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dimm)
{
	u8 addr;
	u16 byte;

	addr = get_dimm_address_d(p_dct_stat, dimm);
	p_dct_stat->spd_data.spd_address[dimm] = addr;

	for (byte = 0; byte < 256; byte++) {
		p_dct_stat->spd_data.spd_bytes[dimm][byte] = mct_read_spd(addr, byte);
	}
}

#ifdef DEBUG_DIMM_SPD
static void dump_spd_bytes(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dimm)
{
	u16 byte;

	printk(BIOS_DEBUG, "SPD dump for dimm %d\n   ", dimm);
	for (byte = 0; byte < 16; byte++) {
		printk(BIOS_DEBUG, "%02x ", byte);
	}
	for (byte = 0; byte < 256; byte++) {
		if ((byte & 0xf) == 0x0) {
			printk(BIOS_DEBUG, "\n%02x ", byte >> 4);
		}
		printk(BIOS_DEBUG, "%02x ", p_dct_stat->spd_data.spd_bytes[dimm][byte]);
	}
	printk(BIOS_DEBUG, "\n");
}
#endif

static void set_up_cc6_storage_fam15(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 num_nodes)
{
	u8 interleaved;
	u8 destination_node;
	int8_t range;
	int8_t max_range;
	u8 max_node;
	u64 max_range_limit;
	u8 byte;
	u32 dword;
	u32 dword2;
	u64 qword;

	interleaved = 0;
	if (p_mct_stat->g_status & (1 << GSB_NODE_INTLV))
		interleaved = 1;

	printk(BIOS_INFO,
		"%s: Initializing CC6 DRAM storage area for node %d"
		" (interleaved: %d)\n",
		 __func__, p_dct_stat->node_id, interleaved);

	/* Find highest DRAM range (DramLimitAddr) */
	max_node = 0;
	max_range = -1;
	max_range_limit = 0;
	for (range = 0; range < 8; range++) {
		dword = get_nb32(p_dct_stat->dev_map, 0x40 + (range * 0x8));
		if (!(dword & 0x3))
			continue;

		dword = get_nb32(p_dct_stat->dev_map, 0x44 + (range * 0x8));
		dword2 = get_nb32(p_dct_stat->dev_map, 0x144 + (range * 0x8));
		qword = 0xffffff;
		qword |= ((((u64)dword) >> 16) & 0xffff) << 24;
		qword |= (((u64)dword2) & 0xff) << 40;

		if (qword > max_range_limit) {
			max_range = range;
			max_range_limit = qword;
			max_node = dword & 0x7;
		}
	}

	if (max_range >= 0) {
		printk(BIOS_INFO,
			"%s:\toriginal (node %d) max_range_limit: %16llx DRAM"
			" limit: %16llx\n",
			__func__, max_node, max_range_limit,
			(((u64)(get_nb32(p_dct_stat->dev_map, 0x124)
				 & 0x1fffff)) << 27) | 0x7ffffff);

		if (interleaved)
			/* Move upper limit down by 16M * the number of nodes */
			max_range_limit -= (0x1000000ULL * num_nodes);
		else
			/* Move upper limit down by 16M */
			max_range_limit -= 0x1000000ULL;

		printk(BIOS_INFO, "%s:\tnew max_range_limit: %16llx\n",
			__func__, max_range_limit);

		/* Disable the range */
		dword = get_nb32(p_dct_stat->dev_map, 0x40 + (max_range * 0x8));
		byte = dword & 0x3;
		dword &= ~(0x3);
		set_nb32(p_dct_stat->dev_map, 0x40 + (max_range * 0x8), dword);

		/* Store modified range */
		dword = get_nb32(p_dct_stat->dev_map, 0x44 + (max_range * 0x8));
		/* DramLimit[39:24] = max_range_limit[39:24] */
		dword &= ~(0xffff << 16);
		dword |= ((max_range_limit >> 24) & 0xffff) << 16;
		set_nb32(p_dct_stat->dev_map, 0x44 + (max_range * 0x8), dword);

		dword = get_nb32(p_dct_stat->dev_map, 0x144 + (max_range * 0x8));
		/* DramLimit[47:40] = max_range_limit[47:40] */
		dword &= ~0xff;
		dword |= (max_range_limit >> 40) & 0xff;
		set_nb32(p_dct_stat->dev_map, 0x144 + (max_range * 0x8), dword);

		/* Reenable the range */
		dword = get_nb32(p_dct_stat->dev_map, 0x40 + (max_range * 0x8));
		dword |= byte;
		set_nb32(p_dct_stat->dev_map, 0x40 + (max_range * 0x8), dword);
	}

	/* Determine save state destination node */
	if (interleaved)
		destination_node = get_nb32(p_dct_stat->dev_host, 0x60) & 0x7;
	else
		destination_node = max_node;

	/* Set save state destination node */
	dword = get_nb32(p_dct_stat->dev_link, 0x128);
	/* CoreSaveStateDestNode = destination_node */
	dword &= ~(0x3f << 12);
	dword |= (destination_node & 0x3f) << 12;
	set_nb32(p_dct_stat->dev_link, 0x128, dword);

	printk(BIOS_INFO, "%s:\tTarget node: %d\n", __func__, destination_node);

	printk(BIOS_INFO, "%s:\tDone\n", __func__);
}

static void lock_dram_config(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 dword;

	dword = get_nb32(p_dct_stat->dev_dct, 0x118);
	dword |= 0x1 << 19;		/* LockDramCfg = 1 */
	set_nb32(p_dct_stat->dev_dct, 0x118, dword);
}

static void set_cc6_save_enable(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 enable)
{
	u32 dword;

	dword = get_nb32(p_dct_stat->dev_dct, 0x118);
	dword &= ~(0x1 << 18);		/* CC6SaveEn = enable */
	dword |= (enable & 0x1) << 18;
	set_nb32(p_dct_stat->dev_dct, 0x118, dword);
}

void mct_auto_init_mct_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a)
{
	/*
	 * Memory may be mapped contiguously all the way up to 4GB (depending on setup
	 * options). It is the responsibility of PCI subsystem to create an uncacheable
	 * IO region below 4GB and to adjust TOP_MEM downward prior to any IO mapping or
	 * accesses. It is the same responsibility of the CPU sub-system prior to
	 * accessing LAPIC.
	 *
	 * Slot Number is an external convention, and is determined by OEM with accompanying
	 * silk screening. OEM may choose to use Slot number convention which is consistent
	 * with dimm number conventions. All AMD engineering platforms do.
	 *
	 * Build Requirements:
	 * 1. MCT_SEG0_START and MCT_SEG0_END macros to begin and end the code segment,
	 *    defined in mcti.inc.
	 *
	 * Run-Time Requirements:
	 * 1. Complete Hypertransport Bus Configuration
	 * 2. SMBus Controller Initialized
	 * 1. BSP in Big Real Mode
	 * 2. Stack at SS:SP, located somewhere between A000:0000 and F000:FFFF
	 * 3. Checksummed or Valid NVRAM bits
	 * 4. MCG_CTL = -1, MC4_CTL_EN = 0 for all CPUs
	 * 5. MCi_STS from shutdown/warm reset recorded (if desired) prior to entry
	 * 6. All var MTRRs reset to zero
	 * 7. State of NB_CFG.DisDatMsk set properly on all CPUs
	 * 8. All CPUs at 2GHz Speed (unless DQS training is not installed).
	 * 9. All cHT links at max Speed/Width (unless DQS training is not installed).
	 *
	 *
	 * Global relationship between index values and item values:
	 *
	 * p_dct_stat.cas_latency p_dct_stat.speed
	 * j CL(j)       k   F(k)
	 * --------------------------
	 * 0 2.0         -   -
	 * 1 3.0         1   200 MHz
	 * 2 4.0         2   266 MHz
	 * 3 5.0         3   333 MHz
	 * 4 6.0         4   400 MHz
	 * 5 7.0         5   533 MHz
	 * 6 8.0         6   667 MHz
	 * 7 9.0         7   800 MHz
	 */
	u8 node, nodes_wmem;
	u32 node_sys_base;
	u8 dimm;
	u8 ecc_enabled;
	u8 allow_config_restore;

	u8 s3resume = acpi_is_wakeup_s3();

restartinit:

	if (!mct_get_nv_bits(NV_ECC_CAP) || !mct_get_nv_bits(NV_ECC))
		p_mct_stat->try_ecc = 0;
	else
		p_mct_stat->try_ecc = 1;

	mct_init_mem_gpios_a_d();		/* Set any required GPIOs*/
	if (s3resume) {
		printk(BIOS_DEBUG, "mct_auto_init_mct_d: mct_force_nb_pstate_0_en_fam15\n");
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			struct DCTStatStruc *p_dct_stat;
			p_dct_stat = p_dct_stat_a + node;

			mct_force_nb_pstate_0_en_fam15(p_mct_stat, p_dct_stat);
		}

#if CONFIG(HAVE_ACPI_RESUME)
		printk(BIOS_DEBUG,
			"mct_auto_init_mct_d: Restoring DCT configuration from NVRAM\n");
		if (restore_mct_information_from_nvram(0) != 0)
			printk(BIOS_CRIT,
				"%s: ERROR: Unable to restore DCT configuration from NVRAM\n",
				__func__);
		p_mct_stat->g_status |= 1 << GSB_CONFIG_RESTORED;
#endif

		if (is_fam15h()) {
			printk(BIOS_DEBUG,
				"mct_auto_init_mct_d: mct_force_nb_pstate_0_dis_fam15\n");
			for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
				struct DCTStatStruc *p_dct_stat;
				p_dct_stat = p_dct_stat_a + node;

				mct_force_nb_pstate_0_dis_fam15(p_mct_stat, p_dct_stat);
			}
		}
	} else {
		nodes_wmem = 0;
		node_sys_base = 0;
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			struct DCTStatStruc *p_dct_stat;
			p_dct_stat = p_dct_stat_a + node;

			/* Zero out data structures to avoid false detection of DIMMs */
			memset(p_dct_stat, 0, sizeof(struct DCTStatStruc));

			/* Initialize data structures */
			p_dct_stat->node_id = node;
			p_dct_stat->dev_host = PA_HOST(node);
			p_dct_stat->dev_map = PA_MAP(node);
			p_dct_stat->dev_dct = PA_DCT(node);
			p_dct_stat->dev_nbmisc = PA_NBMISC(node);
			p_dct_stat->dev_link = PA_LINK(node);
			p_dct_stat->dev_nbctl = PA_NBCTL(node);
			p_dct_stat->node_sys_base = node_sys_base;

			if (mct_get_nv_bits(NV_PACK_TYPE) == PT_GR) {
				u32 dword;
				p_dct_stat->dual_node_package = 1;

				/* Get the internal node number */
				dword = get_nb32(p_dct_stat->dev_nbmisc, 0xe8);
				dword = (dword >> 30) & 0x3;
				p_dct_stat->internal_node_id = dword;
			} else {
				p_dct_stat->dual_node_package = 0;
			}

			printk(BIOS_DEBUG, "%s: mct_init node %d\n", __func__, node);
			mct_init(p_mct_stat, p_dct_stat);
			mct_node_id_debug_port_d();
			p_dct_stat->node_present = node_present_d(node);
			if (p_dct_stat->node_present) {
				p_dct_stat->logical_cpuid = get_logical_cpuid(node);

				printk(BIOS_DEBUG, "%s: mct_initial_mct_d\n", __func__);
				mct_initial_mct_d(p_mct_stat, p_dct_stat);

				printk(BIOS_DEBUG, "%s: mct_smb_hub_init\n", __func__);
				/* Switch SMBUS crossbar to proper node */
				mct_smb_hub_init(node);

				printk(BIOS_DEBUG, "%s: mct_pre_init_dct\n", __func__);
				mct_pre_init_dct(p_mct_stat, p_dct_stat);
			}
			node_sys_base = p_dct_stat->node_sys_base;
			node_sys_base += (p_dct_stat->node_sys_limit + 2) & ~0x0F;
		}
		/* If the boot fails make sure training is attempted after reset */
		set_uint_option("allow_spd_nvram_cache_restore", 0);

#if CONFIG(DIMM_VOLTAGE_SET_SUPPORT)
		printk(BIOS_DEBUG, "%s: DIMMSetVoltage\n", __func__);
		/* Set the dimm voltages (mainboard specific) */
		dimm_set_voltages(p_mct_stat, p_dct_stat_a);
#endif
		if (!CONFIG(DIMM_VOLTAGE_SET_SUPPORT)) {
			/* Assume 1.5V operation */
			for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
				struct DCTStatStruc *p_dct_stat;
				p_dct_stat = p_dct_stat_a + node;

				if (!p_dct_stat->node_present)
					continue;

				for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm++) {
					if (p_dct_stat->dimm_valid & (1 << dimm))
						p_dct_stat->dimm_configured_voltage[dimm] = 0x1;
				}
			}
		}

		/* If dimm configuration has not changed since last boot restore training values
		 */
		allow_config_restore = 1;
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			struct DCTStatStruc *p_dct_stat;
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present)
				if (!p_dct_stat->spd_data.nvram_spd_match)
					allow_config_restore = 0;
		}

		/* FIXME
		 * Stability issues have arisen on multiple Family 15h systems
		 * when configuration restoration is enabled. In all cases these
		 * stability issues resolved by allowing the RAM to go through a
		 * full training cycle.
		 *
		 * Debug and reenable this!
		 */
		allow_config_restore = 0;

		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			struct DCTStatStruc *p_dct_stat;
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present) {
				printk(BIOS_DEBUG, "%s: mct_smb_hub_init\n", __func__);
				/* Switch SMBUS crossbar to proper node*/
				mct_smb_hub_init(node);

				printk(BIOS_DEBUG, "%s: mct_init_dct\n", __func__);
				mct_init_dct(p_mct_stat, p_dct_stat);
				if (p_dct_stat->err_code == SC_FATAL_ERR) {
					/* any fatal errors?*/
					goto fatalexit;
				} else if (p_dct_stat->err_code < SC_STOP_ERR) {
					nodes_wmem++;
				}
			}
		}
		if (nodes_wmem == 0) {
			printk(BIOS_ALERT,
				"Unable to detect valid memory on any nodes. Halting!\n");
			goto fatalexit;
		}

		printk(BIOS_DEBUG, "mct_auto_init_mct_d: sync_dcts_ready_d\n");
		/* Make sure DCTs are ready for accesses.*/
		sync_dcts_ready_d(p_mct_stat, p_dct_stat_a);

		printk(BIOS_DEBUG, "mct_auto_init_mct_d: ht_mem_map_init_d\n");
		/* Map local memory into system address space.*/
		ht_mem_map_init_d(p_mct_stat, p_dct_stat_a);
		mct_hook_after_ht_map();

		if (!is_fam15h()) {
			printk(BIOS_DEBUG, "mct_auto_init_mct_d: cpu_mem_typing_d\n");
			/* Map dram into WB/UC CPU cacheability */
			cpu_mem_typing_d(p_mct_stat, p_dct_stat_a);
		}

		printk(BIOS_DEBUG, "mct_auto_init_mct_d: mct_hook_after_cpu\n");
		/* Setup external northbridge(s) */
		mct_hook_after_cpu();

		/* FIXME
		 * Previous training values should only be used if the current desired
		 * speed is the same as the speed used in the previous boot.
		 * How to get the desired speed at this point in the code?
		 */

		printk(BIOS_DEBUG, "mct_auto_init_mct_d: rqs_timing_d\n");
		/* Get receiver Enable and DQS signal timing*/
		rqs_timing_d(p_mct_stat, p_dct_stat_a, allow_config_restore);

		if (!is_fam15h()) {
			printk(BIOS_DEBUG, "mct_auto_init_mct_d: uma_mem_typing_d\n");
			/* Fix up for UMA sizing */
			uma_mem_typing_d(p_mct_stat, p_dct_stat_a);
		}

		if (!allow_config_restore) {
			printk(BIOS_DEBUG, "mct_auto_init_mct_d: :OtherTiming\n");
			mct_other_timing(p_mct_stat, p_dct_stat_a);
		}

		/* RESET# if 1st pass of dimm spare enabled*/
		if (reconfigure_dimm_spare_d(p_mct_stat, p_dct_stat_a)) {
			goto restartinit;
		}

		interleave_nodes_d(p_mct_stat, p_dct_stat_a);
		interleave_channels_d(p_mct_stat, p_dct_stat_a);

		ecc_enabled = 1;
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			struct DCTStatStruc *p_dct_stat;
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present)
				if (!is_ecc_enabled(p_mct_stat, p_dct_stat))
					ecc_enabled = 0;
		}

		if (ecc_enabled) {
			printk(BIOS_DEBUG, "mct_auto_init_mct_d: ecc_init_d\n");
			/* Setup ECC control and ECC check-bits*/
			if (!ecc_init_d(p_mct_stat, p_dct_stat_a)) {
				/* Memory was not cleared during ECC setup */
				/* mctDoWarmResetMemClr_D(); */
				printk(BIOS_DEBUG, "mct_auto_init_mct_d: mct_mem_clr_d\n");
				mct_mem_clr_d(p_mct_stat,p_dct_stat_a);
			}
		}

		if (is_fam15h()) {
			printk(BIOS_DEBUG, "mct_auto_init_mct_d: cpu_mem_typing_d\n");
			/* Map dram into WB/UC CPU cacheability */
			cpu_mem_typing_d(p_mct_stat, p_dct_stat_a);

			printk(BIOS_DEBUG, "mct_auto_init_mct_d: uma_mem_typing_d\n");
			/* Fix up for UMA sizing */
			uma_mem_typing_d(p_mct_stat, p_dct_stat_a);

			printk(BIOS_DEBUG,
				"mct_auto_init_mct_d: mct_force_nb_pstate_0_dis_fam15\n");
			for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
				struct DCTStatStruc *p_dct_stat;
				p_dct_stat = p_dct_stat_a + node;

				mct_force_nb_pstate_0_dis_fam15(p_mct_stat, p_dct_stat);
			}
		}

		if (is_fam15h()) {
			if (get_uint_option("cpu_cc6_state", 0)) {
				u8 num_nodes;

				num_nodes = 0;
				for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
					struct DCTStatStruc *p_dct_stat;
					p_dct_stat = p_dct_stat_a + node;

					if (p_dct_stat->node_present)
						num_nodes++;
				}

				for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
					struct DCTStatStruc *p_dct_stat;
					p_dct_stat = p_dct_stat_a + node;

					if (p_dct_stat->node_present)
						set_up_cc6_storage_fam15(p_mct_stat, p_dct_stat,
									num_nodes);
				}

				for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
					struct DCTStatStruc *p_dct_stat;
					p_dct_stat = p_dct_stat_a + node;

					if (p_dct_stat->node_present) {
						set_cc6_save_enable(p_mct_stat, p_dct_stat, 1);
						lock_dram_config(p_mct_stat, p_dct_stat);
					}
				}
			}
		}

		mct_final_mct_d(p_mct_stat, p_dct_stat_a);
		printk(BIOS_DEBUG,
			"mct_auto_init_mct_d Done: Global status: %x\n", p_mct_stat->g_status);
	}

	return;

fatalexit:
	die("mct_d: fatalexit");
}

void initialize_mca(u8 bsp, u8 suppress_errors) {
	u8 node;
	u32 mc4_status_high;
	u32 mc4_status_low;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		if (bsp && (node > 0))
			break;

		mc4_status_high = pci_read_config32(PCI_DEV(0, 0x18 + node, 3), 0x4c);
		mc4_status_low = pci_read_config32(PCI_DEV(0, 0x18 + node, 3), 0x48);
		if ((mc4_status_high & (0x1 << 31)) && (mc4_status_high != 0xffffffff)) {
			if (!suppress_errors)
				printk(BIOS_WARNING,
					"WARNING: MC4 Machine Check Exception detected on node %d!\n"
					"Signature: %08x%08x\n",
					node, mc4_status_high, mc4_status_low);

			/* Clear MC4 error status */
			pci_write_config32(PCI_DEV(0, 0x18 + node, 3), 0x48, 0x0);
			pci_write_config32(PCI_DEV(0, 0x18 + node, 3), 0x4c, 0x0);
		}
	}
}

static u8 reconfigure_dimm_spare_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a)
{
	u8 ret;

	if (mct_get_nv_bits(NV_CS_SPARE_CTL)) {
		if (MCT_DIMM_SPARE_NO_WARM) {
			/* Do no warm-reset dimm spare */
			if (p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW)) {
				load_dqs_sig_tmg_regs_d(p_mct_stat, p_dct_stat_a);
				ret = 0;
			} else {
				mct_reset_data_struct_d(p_mct_stat, p_dct_stat_a);
				p_mct_stat->g_status |= 1 << GSB_EN_DIMM_SPARE_NW;
				ret = 1;
			}
		} else {
			/* Do warm-reset dimm spare */
			if (mct_get_nv_bits(NV_DQS_TRAIN_CTL))
				mct_warm_reset_d();
			ret = 0;
		}
	} else {
		ret = 0;
	}

	return ret;
}

/* Enable or disable phy-assisted training mode
 * Phy-assisted training mode applies to the follow DRAM training procedures:
 * Write Levelization Training (2.10.5.8.1)
 * DQS receiver Enable Training (2.10.5.8.2)
 */
void fam_15_enable_training_mode(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct, u8 enable)
{
	u8 index;
	u32 dword;
	u32 index_reg = 0x98;
	u32 dev = p_dct_stat->dev_dct;

	if (enable) {
		/* Enable training mode */
		dword = get_nb32_dct(dev, dct, 0x78);		/* DRAM Control */
		dword &= ~(0x1 << 17);				/* AddrCmdTriEn = 0 */
		set_nb32_dct(dev, dct, 0x78, dword);		/* DRAM Control */

		dword = get_nb32_dct(dev, dct, 0x8c);		/* DRAM Timing High */
		dword |= (0x1 << 18);				/* DIS_AUTO_REFRESH = 1 */
		set_nb32_dct(dev, dct, 0x8c, dword);		/* DRAM Timing High */

		dword = get_nb32_dct(dev, dct, 0x94);		/* DRAM Configuration High */
		dword &= ~(0xf << 24);				/* DcqBypassMax = 0 */
		dword &= ~(0x1 << 22);				/* BANK_SWIZZLE_MODE = 0 */
		dword &= ~(0x1 << 15);				/* POWER_DOWN_EN = 0 */
		dword &= ~(0x3 << 10);				/* ZqcsInterval = 0 */
		set_nb32_dct(dev, dct, 0x94, dword);		/* DRAM Configuration High */

		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000d);
		dword &= ~(0xf << 16);				/* RxMaxDurDllNoLock = 0 */
		dword &= ~(0xf);				/* TxMaxDurDllNoLock = 0 */
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000d, dword);

		for (index = 0; index < 0x9; index++) {
			dword = get_nb32_index_wait_dct(
					dev, dct, index_reg, 0x0d0f0010 | (index << 8));
			dword &= ~(0x1 << 12);			/* EnRxPadStandby = 0 */
			set_nb32_index_wait_dct(
					dev, dct, index_reg, 0x0d0f0010 | (index << 8), dword);
		}

		/* DRAM Controller Temperature Throttle */
		dword = get_nb32_dct(dev, dct, 0xa4);
		/* BwCapEn = 0 */
		dword &= ~(0x1 << 11);
		/* ODTSEn = 0 */
		dword &= ~(0x1 << 8);
		/* DRAM Controller Temperature Throttle */
		set_nb32_dct(dev, dct, 0xa4, dword);

		/* DRAM Controller Select Low */
		dword = get_nb32_dct(dev, dct, 0x110);
		/* DctSelIntLvEn = 0 */
		dword &= ~(0x1 << 2);
		/* DRAM Controller Select Low */
		set_nb32_dct(dev, dct, 0x110, dword);

		/* Scrub Rate Control */
		dword = get_nb32_dct(p_dct_stat->dev_nbmisc, dct, 0x58);
		/* L3Scrub = 0 */
		dword &= ~(0x1f << 24);
		/* DramScrub = 0 */
		dword &= ~(0x1f);
		/* Scrub Rate Control */
		set_nb32_dct(p_dct_stat->dev_nbmisc, dct, 0x58, dword);

		/* DRAM Scrub Address Low */
		dword = get_nb32_dct(p_dct_stat->dev_nbmisc, dct, 0x5c);
		/* ScrubReDirEn = 0 */
		dword &= ~(0x1);
		/* DRAM Scrub Address Low */
		set_nb32_dct(p_dct_stat->dev_nbmisc, dct, 0x5c, dword);

		/* L3 Control 1 */
		dword = get_nb32_dct(p_dct_stat->dev_nbmisc, dct, 0x1b8);
		/* L3ScrbRedirDis = 1 */
		dword |= (0x1 << 4);
		/* L3 Control 1 */
		set_nb32_dct(p_dct_stat->dev_nbmisc, dct, 0x1b8, dword);

		/* Fam15h BKDG section 2.10.5.5.1 */
		/* DRAM Timing 5 */
		dword = get_nb32_dct(dev, dct, 0x218);
		/* TrdrdSdSc = 0xb */
		dword &= ~(0xf << 24);
		dword |= (0xb << 24);
		/* TrdrdSdDc = 0xb */
		dword &= ~(0xf << 16);
		dword |= (0xb << 16);
		/* TrdrdDd = 0xb */
		dword &= ~(0xf);
		dword |= 0xb;
		/* DRAM Timing 5 */
		set_nb32_dct(dev, dct, 0x218, dword);

		/* Fam15h BKDG section 2.10.5.5.2 */
		/* DRAM Timing 4 */
		dword = get_nb32_dct(dev, dct, 0x214);
		/* TwrwrSdSc = 0xb */
		dword &= ~(0xf << 16);
		dword |= (0xb << 16);
		/* TwrwrSdDc = 0xb */
		dword &= ~(0xf << 8);
		dword |= (0xb << 8);
		/* TwrwrDd = 0xb */
		dword &= ~(0xf);
		dword |= 0xb;
		/* DRAM Timing 4 */
		set_nb32_dct(dev, dct, 0x214, dword);

		/* Fam15h BKDG section 2.10.5.5.3 */
		/* DRAM Timing 5 */
		dword = get_nb32_dct(dev, dct, 0x218);
		/* Twrrd = 0xb */
		dword &= ~(0xf << 8);
		dword |= (0xb << 8);
		/* DRAM Timing 5 */
		set_nb32_dct(dev, dct, 0x218, dword);

		/* Fam15h BKDG section 2.10.5.5.4 */
		/* DRAM Timing 6 */
		dword = get_nb32_dct(dev, dct, 0x21c);
		/* TrwtTO = 0x16 */
		dword &= ~(0x1f << 8);
		dword |= (0x16 << 8);
		/* trwt_wb = TrwtTO + 1 */
		dword &= ~(0x1f << 16);
		dword |= ((((dword >> 8) & 0x1f) + 1) << 16);
		/* DRAM Timing 6 */
		set_nb32_dct(dev, dct, 0x21c, dword);
	} else {
		/* Disable training mode */
		u8 lane;
		u8 dimm;
		int16_t max_cdd_we_delta;
		int16_t cdd_trwtto_we_delta;
		u8 receiver;
		u8 lane_count;
		u8 x4_present = 0;
		u8 x8_present = 0;
		u8 memclk_index;
		u8 interleave_channels = 0;
		u16 trdrdsddc;
		u16 trdrddd;
		u16 cdd_trdrddd;
		u16 twrwrsddc;
		u16 twrwrdd;
		u16 cdd_twrwrdd;
		u16 twrrd;
		u16 cdd_twrrd;
		u16 cdd_trwtto;
		u16 trwtto;
		u8 first_dimm;
		u16 delay;
		u16 delay2;
		u8 min_value;
		u8 write_early;
		u8 read_odt_delay;
		u8 write_odt_delay;
		u8 buffer_data_delay;
		int16_t latency_difference;
		u16 difference;
		u16 current_total_delay_1[MAX_BYTE_LANES];
		u16 current_total_delay_2[MAX_BYTE_LANES];
		u8 ddr_voltage_index;
		u8 max_dimms_installable;

		/* FIXME
		 * This should be platform configurable
		 */
		u8 dimm_event_l_pin_support = 0;

		ddr_voltage_index = dct_ddr_voltage_index(p_dct_stat, dct);
		max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

		lane_count = get_available_lane_count(p_mct_stat, p_dct_stat);

		if (p_dct_stat->dimm_x4_present & ((dct) ? 0xaa : 0x55))
			x4_present = 1;
		if (p_dct_stat->dimm_x8_present & ((dct) ? 0xaa : 0x55))
			x8_present = 1;
		memclk_index = get_nb32_dct(dev, dct, 0x94) & 0x1f;

		if (p_dct_stat->dimm_valid_dct[0]
		&& p_dct_stat->dimm_valid_dct[1]
		&& mct_get_nv_bits(NV_UNGANGED))
			interleave_channels = 1;

		dword = get_nb32_dct(dev, dct, 0x240);
		delay = (dword >> 4) & 0xf;
		if (delay > 6)
			read_odt_delay = delay - 6;
		else
			read_odt_delay = 0;
		delay = (dword >> 12) & 0x7;
		if (delay > 6)
			write_odt_delay = delay - 6;
		else
			write_odt_delay = 0;

		dword = (get_nb32_dct(dev, dct, 0xa8) >> 24) & 0x3;
		write_early = dword / 2;

		latency_difference = get_nb32_dct(dev, dct, 0x200) & 0x1f;
		dword = get_nb32_dct(dev, dct, 0x20c) & 0x1f;
		latency_difference -= dword;

		if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */

			/* TODO
			 * Implement LRDIMM support
			 * See Fam15h BKDG Rev. 3.14 section 2.10.5.5
			 */
		} else {
			buffer_data_delay = 0;
		}

		/* TODO:
		 * Adjust trdrdsddc if four-rank DIMMs are installed per
		 * section 2.10.5.5.1 of the Family 15h BKDG.
		 * cdd_trdrdsddc will also need to be calculated in that process.
		 */
		trdrdsddc = 3;

		/* Calculate the Critical Delay Difference for TrdrdDd */
		cdd_trdrddd = 0;
		first_dimm = 1;
		for (receiver = 0; receiver < 8; receiver += 2) {
			dimm = (receiver >> 1);

			if (!mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, dct, receiver))
				continue;

			read_dqs_receiver_enable_control_registers(
				current_total_delay_2, dev, dct, dimm, index_reg);

			if (first_dimm) {
				memcpy(current_total_delay_1,
					current_total_delay_2,
					sizeof(current_total_delay_1));
				first_dimm = 0;
			}

			for (lane = 0; lane < lane_count; lane++) {
				if (current_total_delay_1[lane] > current_total_delay_2[lane]) {
					difference = current_total_delay_1[lane]
							- current_total_delay_2[lane];
				}
				else {
					difference = current_total_delay_2[lane]
							- current_total_delay_1[lane];
				}

				if (difference > cdd_trdrddd)
					cdd_trdrddd = difference;
			}
		}

		/* Convert the difference to MEMCLKs */
		cdd_trdrddd = (((cdd_trdrddd + (1 << 6) - 1) >> 6) & 0xf);

		/* Calculate Trdrddd */
		delay = (read_odt_delay + 3) * 2;
		delay2 = cdd_trdrddd + 7;
		if (delay2 > delay)
			delay = delay2;
		trdrddd = (delay + 1) / 2;	/* + 1 is equivalent to ceiling function here */
		if (trdrdsddc > trdrddd)
			trdrddd = trdrdsddc;

		/* TODO:
		 * Adjust twrwrsddc if four-rank DIMMs are installed per
		 * section 2.10.5.5.1 of the Family 15h BKDG.
		 * cdd_twrwrsddc will also need to be calculated in that process.
		 */
		twrwrsddc = 4;

		/* Calculate the Critical Delay Difference for TwrwrDd */
		cdd_twrwrdd = 0;
		first_dimm = 1;
		for (receiver = 0; receiver < 8; receiver += 2) {
			dimm = (receiver >> 1);

			if (!mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, dct, receiver))
				continue;

			read_dqs_write_timing_control_registers(
					current_total_delay_2, dev, dct, dimm, index_reg);

			if (first_dimm) {
				memcpy(current_total_delay_1,
					current_total_delay_2,
					sizeof(current_total_delay_1));
				first_dimm = 0;
			}

			for (lane = 0; lane < lane_count; lane++) {
				if (current_total_delay_1[lane] > current_total_delay_2[lane]) {
					difference = current_total_delay_1[lane]
							- current_total_delay_2[lane];
				}
				else {
					difference = current_total_delay_2[lane]
							- current_total_delay_1[lane];
				}

				if (difference > cdd_twrwrdd)
					cdd_twrwrdd = difference;
			}
		}

		/* Convert the difference to MEMCLKs */
		cdd_twrwrdd = (((cdd_twrwrdd + (1 << 6) - 1) >> 6) & 0xf);

		/* Calculate Twrwrdd */
		delay = (write_odt_delay + 3) * 2;
		delay2 = cdd_twrwrdd + 7;
		if (delay2 > delay)
			delay = delay2;
		twrwrdd = (delay + 1) / 2;	/* + 1 is equivalent to ceiling function here */
		if (twrwrsddc > twrwrdd)
			twrwrdd = twrwrsddc;

		/* DRAM Control */
		dword = get_nb32_dct(dev, dct, 0x78);
		/* AddrCmdTriEn = 1 */
		dword |= (0x1 << 17);
		/* DRAM Control */
		set_nb32_dct(dev, dct, 0x78, dword);

		/* DRAM Timing High */
		dword = get_nb32_dct(dev, dct, 0x8c);
		/* DIS_AUTO_REFRESH = 0 */
		dword &= ~(0x1 << 18);
		/* DRAM Timing High */
		set_nb32_dct(dev, dct, 0x8c, dword);

		/* Configure power saving options */
		/* Dram Miscellaneous 2 */
		dword = get_nb32_dct(dev, dct, 0xa8);
		/* PrtlChPDEnhEn = 0x1 */
		dword |= (0x1 << 22);
		/* AggrPDEn = 0x1 */
		dword |= (0x1 << 21);
		/* Dram Miscellaneous 2 */
		set_nb32_dct(dev, dct, 0xa8, dword);

		/* Configure partial power down delay */
		/* DRAM Controller Miscellaneous 3 */
		dword = get_nb32(dev, 0x244);
		/* PrtlChPDDynDly = 0x2 */
		dword &= ~0xf;
		dword |= 0x2;
		/* DRAM Controller Miscellaneous 3 */
		set_nb32(dev, 0x244, dword);

		/* Configure power save delays */
		delay = 0xa;
		delay2 = 0x3;

		/* Family 15h BKDG Table 214 */
		if ((p_dct_stat->status & (1 << SB_REGISTERED))
			|| (p_dct_stat->status & (1 << SB_LOAD_REDUCED))) {
			if (memclk_index <= 0x6) {
				if (ddr_voltage_index < 0x4)
					/* 1.5 or 1.35V */
					delay2 = 0x3;
				else
					/* 1.25V */
					delay2 = 0x4;
			}
			else if ((memclk_index == 0xa)
				|| (memclk_index == 0xe))
				delay2 = 0x4;
			else if (memclk_index == 0x12)
				delay2 = 0x5;
			else if (memclk_index == 0x16)
				delay2 = 0x6;
		} else {
			if (memclk_index <= 0x6)
				delay2 = 0x3;
			else if ((memclk_index == 0xa)
				|| (memclk_index == 0xe))
				delay2 = 0x4;
			else if (memclk_index == 0x12)
				delay2 = 0x5;
			else if (memclk_index == 0x16)
				delay2 = 0x6;
		}

		/* Family 15h BKDG Table 215 */
		if (memclk_index <= 0x6)
			delay = 0xa;
		else if (memclk_index == 0xa)
			delay = 0xd;
		else if (memclk_index == 0xe)
			delay = 0x10;
		else if (memclk_index == 0x12)
			delay = 0x14;
		else if (memclk_index == 0x16)
			delay = 0x17;

		/* Dram Power Management 0 */
		dword = get_nb32_dct(dev, dct, 0x248);
		/* AggrPDDelay = 0x0 */
		dword &= ~(0x3f << 24);
		/* PchgPDEnDelay = 0x1 */
		dword &= ~(0x3f << 16);
		dword |= (0x1 << 16);
		/* Txpdll = delay */
		dword &= ~(0x1f << 8);
		dword |= ((delay & 0x1f) << 8);
		/* Txp = delay2 */
		dword &= ~0xf;
		dword |= delay2 & 0xf;
		/* Dram Power Management 0 */
		set_nb32_dct(dev, dct, 0x248, dword);

		/* Family 15h BKDG Table 216 */
		if (memclk_index <= 0x6) {
			delay = 0x5;
			delay2 = 0x3;
		} else if (memclk_index == 0xa) {
			delay = 0x6;
			delay2 = 0x3;
		} else if (memclk_index == 0xe) {
			delay = 0x7;
			delay2 = 0x4;
		} else if (memclk_index == 0x12) {
			delay = 0x8;
			delay2 = 0x4;
		} else if (memclk_index == 0x16) {
			delay = 0xa;
			delay2 = 0x5;
		}

		/* Dram Power Management 1 */
		dword = get_nb32_dct(dev, dct, 0x24c);
		/* Tcksrx = delay */
		dword &= ~(0x3f << 24);
		dword |= ((delay & 0x3f) << 24);
		/* Tcksre = delay */
		dword &= ~(0x3f << 16);
		dword |= ((delay & 0x3f) << 16);
		/* Tckesr = delay2 + 1 */
		dword &= ~(0x3f << 8);
		dword |= (((delay2 + 1) & 0x3f) << 8);
		/* Tpd = delay2 */
		dword &= ~0xf;
		dword |= delay2 & 0xf;
		/* Dram Power Management 1 */
		set_nb32_dct(dev, dct, 0x24c, dword);

		/* DRAM Configuration High */
		dword = get_nb32_dct(dev, dct, 0x94);
		/* DcqBypassMax = 0xf */
		dword |= (0xf << 24);
		/* BANK_SWIZZLE_MODE = 1 */
		dword |= (0x1 << 22);
		/* POWER_DOWN_EN = 1 */
		dword |= (0x1 << 15);
		/* ZqcsInterval = 0x2 */
		dword &= ~(0x3 << 10);
		dword |= (0x2 << 10);
		/* DRAM Configuration High */
		set_nb32_dct(dev, dct, 0x94, dword);

		if (x4_present && x8_present) {
			/* Mixed channel of 4x and 8x DIMMs */
			dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000d);
			/* RxDLLWakeupTime = 0 */
			dword &= ~(0x3 << 24);
			/* RxCPUpdPeriod = 0 */
			dword &= ~(0x7 << 20);
			/* RxMaxDurDllNoLock = 0 */
			dword &= ~(0xf << 16);
			/* TxDLLWakeupTime = 0 */
			dword &= ~(0x3 << 8);
			/* TxCPUpdPeriod = 0 */
			dword &= ~(0x7 << 4);
			/* TxMaxDurDllNoLock = 0 */
			dword &= ~(0xf);
			set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000d, dword);
		} else {
			dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000d);
			/* RxDLLWakeupTime = 3 */
			dword &= ~(0x3 << 24);
			dword |= (0x3 << 24);
			/* RxCPUpdPeriod = 3 */
			dword &= ~(0x7 << 20);
			dword |= (0x3 << 20);
			/* RxMaxDurDllNoLock = 7 */
			dword &= ~(0xf << 16);
			dword |= (0x7 << 16);
			/* TxDLLWakeupTime = 3 */
			dword &= ~(0x3 << 8);
			dword |= (0x3 << 8);
			/* TxCPUpdPeriod = 3 */
			dword &= ~(0x7 << 4);
			dword |= (0x3 << 4);
			/* TxMaxDurDllNoLock = 7 */
			dword &= ~(0xf);
			dword |= 0x7;
			set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000d, dword);
		}

		if ((memclk_index <= 0x12) && (x4_present != x8_present)) {
			/* mem_clk_freq <= 800MHz
			 * Not a mixed channel of x4 and x8 DIMMs
			 */
			for (index = 0; index < 0x9; index++) {
				dword = get_nb32_index_wait_dct(
							dev, dct, index_reg,
							0x0d0f0010 | (index << 8));
				/* EnRxPadStandby = 1 */
				dword |= (0x1 << 12);
				set_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f0010 | (index << 8), dword);
			}
		} else {
			for (index = 0; index < 0x9; index++) {
				dword = get_nb32_index_wait_dct(
							dev, dct, index_reg,
							0x0d0f0010 | (index << 8));
				/* EnRxPadStandby = 0 */
				dword &= ~(0x1 << 12);
				set_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f0010 | (index << 8), dword);
			}
		}

		/* Calculate the Critical Delay Difference for Twrrd */
		cdd_twrrd = 0;
		for (receiver = 0; receiver < 8; receiver += 2) {
			dimm = (receiver >> 1);

			if (!mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, dct, receiver))
				continue;

			read_dqs_write_timing_control_registers(
					current_total_delay_1, dev, dct, dimm, index_reg);
			read_dqs_receiver_enable_control_registers(
					current_total_delay_2, dev, dct, dimm, index_reg);

			for (lane = 0; lane < lane_count; lane++) {
				if (current_total_delay_1[lane] > current_total_delay_2[lane]) {
					difference = current_total_delay_1[lane]
							- current_total_delay_2[lane];
				}
				else {
					difference = current_total_delay_2[lane]
							- current_total_delay_1[lane];
				}

				if (difference > cdd_twrrd)
					cdd_twrrd = difference;
			}
		}

		/* Convert the difference to MEMCLKs */
		cdd_twrrd = (((cdd_twrrd + (1 << 6) - 1) >> 6) & 0xf);

		/* Fam15h BKDG section 2.10.5.5.3 */
		if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* LRDIMM */

			/* TODO
			 * Implement LRDIMM support
			 * See Fam15h BKDG Rev. 3.14 section 2.10.5.5
			 */
			twrrd = 0xb;
		} else {
			max_cdd_we_delta = (((int16_t)cdd_twrrd + 1
					- ((int16_t)write_early * 2)) + 1) / 2;
			if (max_cdd_we_delta < 0)
				max_cdd_we_delta = 0;
			if (((u16)max_cdd_we_delta) > write_odt_delay)
				dword = max_cdd_we_delta;
			else
				dword = write_odt_delay;
			dword += 3;
			if (latency_difference < dword) {
				dword -= latency_difference;
				if (dword < 1)
					twrrd = 1;
				else
					twrrd = dword;
			} else {
				twrrd = 1;
			}
		}

		/* Calculate the Critical Delay Difference for TrwtTO */
		cdd_trwtto = 0;
		for (receiver = 0; receiver < 8; receiver += 2) {
			dimm = (receiver >> 1);

			if (!mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, dct, receiver))
				continue;

			read_dqs_receiver_enable_control_registers(
					current_total_delay_1, dev, dct, dimm, index_reg);
			read_dqs_write_timing_control_registers(
					current_total_delay_2, dev, dct, dimm, index_reg);

			for (lane = 0; lane < lane_count; lane++) {
				if (current_total_delay_1[lane] > current_total_delay_2[lane]) {
					difference = current_total_delay_1[lane]
							- current_total_delay_2[lane];
				}
				else {
					difference = current_total_delay_2[lane]
							- current_total_delay_1[lane];
				}

				if (difference > cdd_trwtto)
					cdd_trwtto = difference;
			}
		}

		/* Convert the difference to MEMCLKs */
		cdd_trwtto = (((cdd_trwtto + (1 << 6) - 1) >> 6) & 0xf);

		/* Fam15h BKDG section 2.10.5.5.4 */
		if (max_dimms_installable == 1)
			min_value = 0;
		else
			min_value = read_odt_delay + buffer_data_delay;
		cdd_trwtto_we_delta = (((int16_t)cdd_trwtto - 1
					+ ((int16_t)write_early * 2)) + 1) / 2;
		cdd_trwtto_we_delta += latency_difference + 3;
		if (cdd_trwtto_we_delta < 0)
			cdd_trwtto_we_delta = 0;
		if ((cdd_trwtto_we_delta) > min_value)
			trwtto = cdd_trwtto_we_delta;
		else
			trwtto = min_value;

		/* DRAM Controller Temperature Throttle */
		dword = get_nb32_dct(dev, dct, 0xa4);
		/* BwCapEn = 0 */
		dword &= ~(0x1 << 11);
		/* ODTSEn = dimm_event_l_pin_support */
		dword &= ~(0x1 << 8);
		dword |= (dimm_event_l_pin_support & 0x1) << 8;
		/* DRAM Controller Temperature Throttle */
		set_nb32_dct(dev, dct, 0xa4, dword);

		/* DRAM Controller Select Low */
		dword = get_nb32_dct(dev, dct, 0x110);
		/* DctSelIntLvEn = interleave_channels */
		dword &= ~(0x1 << 2);
		dword |= (interleave_channels & 0x1) << 2;
		/* DctSelIntLvAddr = 0x3 */
		dword |= (0x3 << 6);
		/* DRAM Controller Select Low */
		set_nb32_dct(dev, dct, 0x110, dword);

		/* NOTE
		 * ECC-related setup is performed as part of ecc_init_d and must not be located
		 * here, otherwise semi-random lockups will occur due to misconfigured scrubbing
		 * hardware!
		 */

		/* Fam15h BKDG section 2.10.5.5.2 */
		/* DRAM Timing 4 */
		dword = get_nb32_dct(dev, dct, 0x214);
		/* TwrwrSdSc = 0x1 */
		dword &= ~(0xf << 16);
		dword |= (0x1 << 16);
		/* TwrwrSdDc = twrwrsddc */
		dword &= ~(0xf << 8);
		dword |= ((twrwrsddc & 0xf) << 8);
		/* TwrwrDd = twrwrdd */
		dword &= ~(0xf);
		dword |= (twrwrdd & 0xf);
		/* DRAM Timing 4 */
		set_nb32_dct(dev, dct, 0x214, dword);

		/* Fam15h BKDG section 2.10.5.5.3 */
		/* DRAM Timing 5 */
		dword = get_nb32_dct(dev, dct, 0x218);
		/* TrdrdSdSc = 0x1 */
		dword &= ~(0xf << 24);
		dword |= (0x1 << 24);
		/* TrdrdSdDc = trdrdsddc */
		dword &= ~(0xf << 16);
		dword |= ((trdrdsddc & 0xf) << 16);
		/* Twrrd = twrrd */
		dword &= ~(0xf << 8);
		dword |= ((twrrd & 0xf) << 8);
		/* TrdrdDd = trdrddd */
		dword &= ~(0xf);
		dword |= (trdrddd & 0xf);
		/* DRAM Timing 5 */
		set_nb32_dct(dev, dct, 0x218, dword);

		/* Fam15h BKDG section 2.10.5.5.4 */
		/* DRAM Timing 6 */
		dword = get_nb32_dct(dev, dct, 0x21c);
		/* TrwtTO = trwtto */
		dword &= ~(0x1f << 8);
		dword |= ((trwtto & 0x1f) << 8);
		/* trwt_wb = TrwtTO + 1 */
		dword &= ~(0x1f << 16);
		dword |= ((((dword >> 8) & 0x1f) + 1) << 16);
		/* DRAM Timing 6 */
		set_nb32_dct(dev, dct, 0x21c, dword);

		/* Enable prefetchers */
		/* Memory Controller Configuration High */
		dword = get_nb32(dev, 0x11c);
		/* PrefIoDis = 0 */
		dword &= ~(0x1 << 13);
		/* PrefCpuDis = 0 */
		dword &= ~(0x1 << 12);
		/* Memory Controller Configuration High */
		set_nb32(dev, 0x11c, dword);
	}
}

static void exit_training_mode_fam15(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	u8 dct;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;

		if (p_dct_stat->node_present)
			for (dct = 0; dct < 2; dct++)
				fam_15_enable_training_mode(p_mct_stat, p_dct_stat, dct, 0);
	}
}

static void rqs_timing_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a, u8 allow_config_restore)
{
	u8 node;
	u8 nv_dqs_train_ctl;
	u8 retry_requested;

	if (p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW)) {
		return;
	}

	/* Set initial TCWL offset to zero */
	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		u8 dct;
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		for (dct = 0; dct < 2; dct++)
			p_dct_stat->tcwl_delay[dct] = 0;
	}

retry_dqs_training_and_levelization:
	nv_dqs_train_ctl = !allow_config_restore;

	mct_before_dqs_train_d(p_mct_stat, p_dct_stat_a);
	phy_assisted_mem_fence_training(p_mct_stat, p_dct_stat_a, -1);

	if (is_fam15h()) {
		struct DCTStatStruc *p_dct_stat;
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			p_dct_stat = p_dct_stat_a + node;
			if (p_dct_stat->node_present) {
				if (p_dct_stat->dimm_valid_dct[0])
					init_phy_compensation(p_mct_stat, p_dct_stat, 0);
				if (p_dct_stat->dimm_valid_dct[1])
					init_phy_compensation(p_mct_stat, p_dct_stat, 1);
			}
		}
	}

	mct_hook_before_any_training(p_mct_stat, p_dct_stat_a);
	if (!is_fam15h()) {
		/* TODO: should be in mct_hook_before_any_training */
		_wrmsr(MTRR_FIX_4K_E0000, 0x04040404, 0x04040404);
		_wrmsr(MTRR_FIX_4K_E8000, 0x04040404, 0x04040404);
		_wrmsr(MTRR_FIX_4K_F0000, 0x04040404, 0x04040404);
		_wrmsr(MTRR_FIX_4K_F8000, 0x04040404, 0x04040404);
	}

	if (nv_dqs_train_ctl) {
		mct_write_levelization_hw(p_mct_stat, p_dct_stat_a, FIRST_PASS);

		if (is_fam15h()) {
			/* receiver Enable Training Pass 1 */
			train_receiver_en_d(p_mct_stat, p_dct_stat_a, FIRST_PASS);
		}

		mct_write_levelization_hw(p_mct_stat, p_dct_stat_a, SECOND_PASS);

		if (is_fam15h()) {

			/* TODO:
			 * Determine why running train_receiver_en_d in SECOND_PASS
			 * mode yields less stable training values than when run
			 * in FIRST_PASS mode as in the HACK below.
			 */
			train_receiver_en_d(p_mct_stat, p_dct_stat_a, FIRST_PASS);
		} else {
			train_receiver_en_d(p_mct_stat, p_dct_stat_a, FIRST_PASS);
		}

		mct_train_dqs_pos_d(p_mct_stat, p_dct_stat_a);

		/* Determine if DQS training requested a retrain attempt */
		retry_requested = 0;
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			struct DCTStatStruc *p_dct_stat;
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present) {
				if (p_dct_stat->train_errors & (1 << SB_FATAL_ERROR)) {
					printk(BIOS_ERR,
						"dimm training FAILED! Restarting system...");
					soft_reset();
				}
				if (p_dct_stat->train_errors & (1 << SB_RETRY_CONFIG_TRAIN)) {
					retry_requested = 1;

					/* Clear previous errors */
					p_dct_stat->train_errors
							&= ~(1 << SB_RETRY_CONFIG_TRAIN);
					p_dct_stat->train_errors &= ~(1 << SB_NO_DQS_POS);
					p_dct_stat->err_status &= ~(1 << SB_RETRY_CONFIG_TRAIN);
					p_dct_stat->err_status &= ~(1 << SB_NO_DQS_POS);
				}
			}
		}

		/* Retry training and levelization if requested */
		if (retry_requested) {
			printk(BIOS_DEBUG,
				"%s: Restarting training on algorithm request\n", __func__);
			/* Reset frequency to minimum */
			for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
				struct DCTStatStruc *p_dct_stat;
				p_dct_stat = p_dct_stat_a + node;
				if (p_dct_stat->node_present) {
					u8 original_target_freq = p_dct_stat->target_freq;
					u8 original_auto_speed = p_dct_stat->dimm_auto_speed;
					p_dct_stat->target_freq
							= mhz_to_memclk_config(
								mct_get_nv_bits(NV_MIN_MEMCLK));
					p_dct_stat->speed
							= p_dct_stat->dimm_auto_speed
							= p_dct_stat->target_freq;
					set_target_freq(p_mct_stat, p_dct_stat_a, node);
					p_dct_stat->target_freq = original_target_freq;
					p_dct_stat->dimm_auto_speed = original_auto_speed;
				}
			}
			/* Apply any dimm timing changes */
			for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
				struct DCTStatStruc *p_dct_stat;
				p_dct_stat = p_dct_stat_a + node;
				if (p_dct_stat->node_present) {
					auto_cyc_timing(p_mct_stat, p_dct_stat, 0);
					if (!p_dct_stat->ganged_mode)
						if (p_dct_stat->dimm_valid_dct[1] > 0)
							auto_cyc_timing(p_mct_stat,
									p_dct_stat, 1);
				}
			}
			goto retry_dqs_training_and_levelization;
		}

		train_max_rd_latency_en_d(p_mct_stat, p_dct_stat_a);

		if (is_fam15h())
			exit_training_mode_fam15(p_mct_stat, p_dct_stat_a);
		else
			mct_set_ecc_dqs_rcvr_en_d(p_mct_stat, p_dct_stat_a);
	} else {
		mct_write_levelization_hw(p_mct_stat, p_dct_stat_a, FIRST_PASS);

		mct_write_levelization_hw(p_mct_stat, p_dct_stat_a, SECOND_PASS);

#if CONFIG(HAVE_ACPI_RESUME)
		printk(BIOS_DEBUG,
			"mct_auto_init_mct_d: Restoring dimm training configuration from NVRAM\n");
		if (restore_mct_information_from_nvram(1) != 0)
			printk(BIOS_CRIT,
				"%s: ERROR: Unable to restore DCT configuration from NVRAM\n",
				__func__);
#endif

		if (is_fam15h())
			exit_training_mode_fam15(p_mct_stat, p_dct_stat_a);

		p_mct_stat->g_status |= 1 << GSB_CONFIG_RESTORED;
	}

	if (is_fam15h()) {
		struct DCTStatStruc *p_dct_stat;

		/* Switch DCT control register to DCT 0 per Erratum 505 */
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			p_dct_stat = p_dct_stat_a + node;
			if (p_dct_stat->node_present) {
				fam15h_switch_dct(p_dct_stat->dev_map, 0);
			}
		}
	}

	/* FIXME - currently uses calculated value
	 * train_max_read_latency_d(p_mct_stat, p_dct_stat_a);
	 */
	mct_hook_after_any_training();
}

static void load_dqs_sig_tmg_regs_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a)
{
	u8 node, receiver, channel, dir, dimm;
	u32 dev;
	u32 index_reg;
	u32 reg;
	u32 index;
	u32 val;
	u8 byte_lane;
	u8 txdqs;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;

		if (p_dct_stat->dct_sys_limit) {
			dev = p_dct_stat->dev_dct;
			for (channel = 0;channel < 2; channel++) {
				/* there are four receiver pairs,
				   loosely associated with chipselects.*/
				index_reg = 0x98;
				for (receiver = 0; receiver < 8; receiver += 2) {
					/* Set receiver Enable Values */
					mct_set_rcvr_en_dly_d(p_dct_stat,
						0, /* RcvrEnDly */
						1, /* FinalValue, From stack */
						channel,
						receiver,
						dev, index_reg,
						(receiver >> 1) * 3 + 0x10, /* Addl_Index */
						2); /* Pass Second Pass ? */
					/* Restore Write levelization training data */
					for (byte_lane = 0; byte_lane < 9; byte_lane ++) {
						txdqs = p_dct_stat->persistent_data
									.ch_d_b_tx_dqs[channel]
										[receiver >> 1]
										[byte_lane];
						index = table_dqs_rcv_en_offset[byte_lane >> 1];
						/* Addl_Index */
						index += (receiver >> 1) * 3 + 0x10 + 0x20;
						val = get_nb32_index_wait_dct(
								dev, channel, 0x98, index);
						if (byte_lane & 1) { /* odd byte lane */
							val &= ~(0xFF << 16);
							val |= txdqs << 16;
						} else {
							val &= ~0xFF;
							val |= txdqs;
						}
						set_nb32_index_wait_dct(
								dev, channel, 0x98, index, val);
					}
				}
			}
			for (channel = 0; channel < 2; channel++) {
				set_ecc_dqs_rcvr_en_d(p_dct_stat, channel);
			}

			for (channel = 0; channel < 2; channel++) {
				u8 *p;
				index_reg = 0x98;

				/* NOTE:
				 * when 400, 533, 667, it will support dimm0/1/2/3,
				 * and set conf for dimm0, hw will copy to dimm1/2/3
				 * set for dimm1, hw will copy to dimm3
				 * Rev A/B only support DIMM0/1 when 800MHz and above
				 *   + 0x100 to next dimm
				 * Rev C support DIMM0/1/2/3 when 800MHz and above
				 *   + 0x100 to next dimm
				*/
				for (dimm = 0; dimm < 4; dimm++) {
					if (dimm == 0) {
						/* CHA Write Data Timing Low */
						index = 0;
					} else {
						if (p_dct_stat->speed
						>= mhz_to_memclk_config(mct_get_nv_bits(
									NV_MIN_MEMCLK))) {
							index = 0x100 * dimm;
						} else {
							break;
						}
					}
					for (dir = 0; dir < 2; dir++) {/* RD/WR */
						p = p_dct_stat->ch_d_dir_b_dqs[channel][dimm]
											[dir];
						/* CHA Read Data Timing High */
						val = stream_to_int(p);
						set_nb32_index_wait_dct(
								dev, channel, index_reg,
								index + 1, val);
						/* CHA Write Data Timing High */
						val = stream_to_int(p + 4);
						set_nb32_index_wait_dct(
								dev, channel, index_reg,
								index + 2, val);
						/* CHA Write ECC Timing */
						val = *(p + 8);
						set_nb32_index_wait_dct(
								dev, channel, index_reg,
								index + 3, val);
						index += 4;
					}
				}
			}

			for (channel = 0; channel < 2; channel++) {
				reg = 0x78;
				val = get_nb32_dct(dev, channel, reg);
				val &= ~(0x3ff << 22);
				val |= ((u32)p_dct_stat->ch_max_rd_lat[channel][0] << 22);
				val &= ~(1 << DQS_RCV_EN_TRAIN);
				/* program MaxRdLatency to correspond with current delay */
				set_nb32_dct(dev, channel, reg, val);
			}
		}
	}
}

static void ht_mem_map_init_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	u32 next_base, bottom_io;
	u8 _mem_hole_remap, dram_hole_base;
	u32 hole_size, dram_sel_base_addr;

	u32 val;
	u32 base;
	u32 limit;
	u32 dev, devx;
	struct DCTStatStruc *p_dct_stat;

	_mem_hole_remap = mct_get_nv_bits(NV_MEM_HOLE);

	if (p_mct_stat->hole_base == 0) {
		dram_hole_base = mct_get_nv_bits(NV_BOTTOM_IO);
	} else {
		dram_hole_base = p_mct_stat->hole_base >> (24-8);
	}

	bottom_io = dram_hole_base << (24-8);

	next_base = 0;
	p_dct_stat = p_dct_stat_a + 0;
	dev = p_dct_stat->dev_map;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		p_dct_stat = p_dct_stat_a + node;
		devx = p_dct_stat->dev_map;
		dram_sel_base_addr = 0;
		if (!p_dct_stat->ganged_mode) {
			dram_sel_base_addr = p_dct_stat->node_sys_limit
					- p_dct_stat->dct_sys_limit;
			/*In unganged mode, we must add DCT0 and DCT1 to dct_sys_limit */
			val = p_dct_stat->node_sys_limit;
			if ((val & 0xFF) == 0xFE) {
				dram_sel_base_addr++;
				val++;
			}
			p_dct_stat->dct_sys_limit = val;
		}

		base  = p_dct_stat->dct_sys_base;
		limit = p_dct_stat->dct_sys_limit;
		if (limit > base) {
			base  += next_base;
			limit += next_base;
			dram_sel_base_addr += next_base;
			printk(BIOS_DEBUG,
				" node: %02x  base: %02x  limit: %02x  bottom_io: %02x\n",
				node, base, limit, bottom_io);

			if (_mem_hole_remap) {
				if ((base < bottom_io) && (limit >= bottom_io)) {
					/* HW Dram Remap */
					p_dct_stat->status |= 1 << SB_HW_HOLE;
					p_mct_stat->g_status |= 1 << GSB_HW_HOLE;
					p_dct_stat->dct_sys_base = base;
					p_dct_stat->dct_sys_limit = limit;
					p_dct_stat->dct_hole_base = bottom_io;
					p_mct_stat->hole_base = bottom_io;
					hole_size = _4GB_RJ8 - bottom_io; /* hole_size[39:8] */
					if ((dram_sel_base_addr > 0)
					&& (dram_sel_base_addr < bottom_io))
						base = dram_sel_base_addr;
					val = ((base + hole_size) >> (24-8)) & 0xFF;
					val <<= 8; /* shl 16, rol 24 */
					val |= dram_hole_base << 24;
					val |= 1  << DRAM_HOLE_VALID;
					set_nb32(devx, 0xF0, val); /* Dram Hole Address Reg */
					p_dct_stat->dct_sys_limit += hole_size;
					base = p_dct_stat->dct_sys_base;
					limit = p_dct_stat->dct_sys_limit;
				} else if (base == bottom_io) {
					/* SW node Hoist */
					p_mct_stat->g_status |= 1 << GSB_SP_INTLV_REMAP_HOLE;
					p_dct_stat->status |= 1 << SB_SW_NODE_HOLE;
					p_mct_stat->g_status |= 1 << GSB_SOFT_HOLE;
					p_mct_stat->hole_base = base;
					limit -= base;
					base = _4GB_RJ8;
					limit += base;
					p_dct_stat->dct_sys_base = base;
					p_dct_stat->dct_sys_limit = limit;
				} else {
					/* No Remapping.  Normal Contiguous mapping */
					p_dct_stat->dct_sys_base = base;
					p_dct_stat->dct_sys_limit = limit;
				}
			} else {
				/*No Remapping.  Normal Contiguous mapping*/
				p_dct_stat->dct_sys_base = base;
				p_dct_stat->dct_sys_limit = limit;
			}
			base |= 3;		/* set WE,RE fields*/
			p_mct_stat->sys_limit = limit;
		}
		set_nb32(dev, 0x40 + (node << 3), base); /* [node] + Dram Base 0 */

		val = limit & 0xFFFF0000;
		val |= node;
		set_nb32(dev, 0x44 + (node << 3), val);	/* set DstNode */

		printk(BIOS_DEBUG, " node: %02x  base: %02x  limit: %02x\n", node, base, limit);
		limit = p_dct_stat->dct_sys_limit;
		if (limit) {
			next_base = (limit & 0xFFFF0000) + 0x10000;
		}
	}

	/* Copy dram map from node 0 to node 1-7 */
	for (node = 1; node < MAX_NODES_SUPPORTED; node++) {
		u32 reg;
		p_dct_stat = p_dct_stat_a + node;
		devx = p_dct_stat->dev_map;

		if (p_dct_stat->node_present) {
			printk(BIOS_DEBUG, " Copy dram map from node 0 to node %02x\n", node);
			reg = 0x40;		/*Dram Base 0*/
			do {
				val = get_nb32(dev, reg);
				set_nb32(devx, reg, val);
				reg += 4;
			} while (reg < 0x80);
		} else {
			break;			/* stop at first absent node */
		}
	}

	/*Copy dram map to F1x120/124*/
	mct_ht_mem_map_ext(p_mct_stat, p_dct_stat_a);
}

void mct_mem_clr_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{

	/* Initiates a memory clear operation for all node. The mem clr
	 * is done in parallel. After the memclr is complete, all processors
	 * status are checked to ensure that memclr has completed.
	 */
	u8 node;
	u32 dword;
	struct DCTStatStruc *p_dct_stat;

	if (!mct_get_nv_bits(NV_DQS_TRAIN_CTL)) {
		/* FIXME: callback to wrapper: mctDoWarmResetMemClr_D */
	} else {	/* NV_DQS_TRAIN_CTL == 1 */
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present) {
				dct_mem_clr_init_d(p_mct_stat, p_dct_stat);
			}
		}
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present) {
				dct_mem_clr_sync_d(p_mct_stat, p_dct_stat);
			}
		}
	}

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		p_dct_stat = p_dct_stat_a + node;

		/* Enable prefetchers */
		/* Memory Controller Configuration High */
		dword = get_nb32(p_dct_stat->dev_dct, 0x11c);
		/* PrefIoDis = 0 */
		dword &= ~(0x1 << 13);
		/* PrefCpuDis = 0 */
		dword &= ~(0x1 << 12);
		/* Memory Controller Configuration High */
		set_nb32(p_dct_stat->dev_dct, 0x11c, dword);
	}
}

void dct_mem_clr_init_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 val;
	u32 dev;
	u32 dword;

	/* Initiates a memory clear operation on one node */
	if (p_dct_stat->dct_sys_limit) {
		dev = p_dct_stat->dev_dct;

		/* Disable prefetchers */
		/* Memory Controller Configuration High */
		dword = get_nb32(dev, 0x11c);
		/* PrefIoDis = 1 */
		dword |= 0x1 << 13;
		/* PrefCpuDis = 1 */
		dword |= 0x1 << 12;
		/* Memory Controller Configuration High */
		set_nb32(dev, 0x11c, dword);

		do {
			val = get_nb32(dev, 0x110);
		} while (val & (1 << MEM_CLR_BUSY));

		val |= (1 << MEM_CLR_INIT);
		set_nb32(dev, 0x110, val);
	}
}

void dct_mem_clr_sync_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 dword;
	u32 dev = p_dct_stat->dev_dct;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	/* Ensure that a memory clear operation has completed on one node */
	if (p_dct_stat->dct_sys_limit) {
		printk(BIOS_DEBUG, "%s: Waiting for memory clear to complete\n", __func__);
		do {
			dword = get_nb32(dev, 0x110);
			if (CONFIG(CONSOLE_SERIAL)) {
				/* Indicate the process is in progress only on serial.
				 * Delay prevents printing many dots
				 */
				mdelay(10);
				printk(BIOS_DEBUG, ".");
			}
		} while (dword & (1 << MEM_CLR_BUSY));

		printk(BIOS_DEBUG, "\n");
		do {
			dword = get_nb32(dev, 0x110);
			if (CONFIG(CONSOLE_SERIAL)) {
				/* Indicate the process is in progress only on serial.
				 * Delay prevents printing many dots
				 */
				mdelay(5);
				printk(BIOS_DEBUG, ".");
			}
		} while (!(dword & (1 << DR_MEM_CLR_STATUS)));
		printk(BIOS_DEBUG, "\n");
	}

	/* Enable prefetchers */
	dword = get_nb32(dev, 0x11c);		/* Memory Controller Configuration High */
	dword &= ~(0x1 << 13);			/* PrefIoDis = 0 */
	dword &= ~(0x1 << 12);			/* PrefCpuDis = 0 */
	set_nb32(dev, 0x11c, dword);		/* Memory Controller Configuration High */

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

u8 node_present_d(u8 node)
{
	/*
	 * Determine if a single Hammer node exists within the network.
	 */
	u32 dev;
	u32 val;
	u32 dword;
	u8 ret = 0;

	dev = PA_HOST(node);		/*test device/vendor id at host bridge  */
	val = get_nb32(dev, 0);
	dword = mct_node_present_d();	/* FIXME: BOZO -11001022h rev for F */
	if (val == dword) {		/* AMD Hammer Family CPU HT Configuration */
		if (oem_node_present_d(node, &ret))
			goto finish;
		/* node ID register */
		val = get_nb32(dev, 0x60);
		val &= 0x07;
		dword = node;
		if (val  == dword)	/* current nodeID = requested nodeID ? */
			ret = 1;
	}
finish:
	return ret;
}

static void dct_pre_init_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/*
	 * Run DCT pre-initialization tasks
	 */
	u32 dword;

	/* Reset DCT registers */
	clear_dct_d(p_mct_stat, p_dct_stat, dct);
	p_dct_stat->stop_dtc[dct] = 1;	/* preload flag with 'disable' */

	if (!is_fam15h()) {
		/* Enable DDR3 support */
		dword = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94);
		dword |= 1 << DDR3_MODE;
		set_nb32_dct(p_dct_stat->dev_dct, dct, 0x94, dword);
	}

	/* Read the SPD information into the data structures */
	if (mct_dimm_presence(p_mct_stat, p_dct_stat, dct) < SC_STOP_ERR) {
		printk(BIOS_DEBUG, "\t\tDCTPreInit_D: mct_dimm_presence Done\n");
	}
}

static void dct_init_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/*
	 * Initialize DRAM on single Athlon 64/Opteron node.
	 */
	u32 dword;

	if (!is_fam15h()) {
		/* (Re)-enable DDR3 support */
		dword = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94);
		dword |= 1 << DDR3_MODE;
		set_nb32_dct(p_dct_stat->dev_dct, dct, 0x94, dword);
	}

	if (mct_spd_calc_width(p_mct_stat, p_dct_stat, dct) < SC_STOP_ERR) {
		printk(BIOS_DEBUG, "\t\tDCTInit_D: mct_spd_calc_width Done\n");
		if (auto_cyc_timing(p_mct_stat, p_dct_stat, dct) < SC_STOP_ERR) {
			printk(BIOS_DEBUG, "\t\tDCTInit_D: auto_cyc_timing Done\n");

			/* SkewMemClk must be set before MEM_CLK_FREQ_VAL is set
			 * This relies on dct_init_d being called for DCT 1 after
			 * it has already been called for DCT 0...
			 */
			if (is_fam15h()) {
				/* Set memory clock skew if needed */
				if (dct == 1) {
					if (!p_dct_stat->stop_dtc[0]) {
						printk(BIOS_DEBUG,
							"\t\tDCTInit_D: enabling intra-channel clock skew\n");
						dword = get_nb32_index_wait_dct(
									p_dct_stat->dev_dct, 0,
									0x98, 0x0d0fe00a);
						/* SkewMemClk = 1 */
						dword |= (0x1 << 4);
						set_nb32_index_wait_dct(p_dct_stat->dev_dct,
									0, 0x98, 0x0d0fe00a,
									dword);
					}
				}
			}

			if (auto_config_d(p_mct_stat, p_dct_stat, dct) < SC_STOP_ERR) {
				printk(BIOS_DEBUG, "\t\tDCTInit_D: auto_config_d Done\n");
				if (platform_spec_d(p_mct_stat, p_dct_stat, dct)
				< SC_STOP_ERR) {
					printk(BIOS_DEBUG,
						"\t\tDCTInit_D: platform_spec_d Done\n");
					p_dct_stat->stop_dtc[dct] = 0;
				}
			}
		}
	}
}

static void dct_final_init_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 dword;

	/* Finalize DRAM init on a single node */
	if (!p_dct_stat->stop_dtc[dct]) {
		if (!(p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW))) {
			printk(BIOS_DEBUG, "\t\tDCTFinalInit_D: startup_dct_d Start\n");
			startup_dct_d(p_mct_stat, p_dct_stat, dct);
			printk(BIOS_DEBUG, "\t\tDCTFinalInit_D: startup_dct_d Done\n");
		}
	}

	if (p_dct_stat->stop_dtc[dct]) {
		dword = 1 << DIS_DRAM_INTERFACE;
		set_nb32_dct(p_dct_stat->dev_dct, dct, 0x94, dword);

		dword = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x90);
		dword &= ~(1 << PAR_EN);
		set_nb32_dct(p_dct_stat->dev_dct, dct, 0x90, dword);

		/* To maximize power savings when DIS_DRAM_INTERFACE = 1b,
		 * all of the MemClkDis bits should also be set.
		 */
		set_nb32_dct(p_dct_stat->dev_dct, dct, 0x88, 0xff000000);
	} else {
		mct_en_dll_shutdown_sr(p_mct_stat, p_dct_stat, dct);
	}
}

static void sync_dcts_ready_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	/* Wait (and block further access to dram) for all DCTs to be ready,
	 * by polling all INIT_DRAM bits and waiting for possible memory clear
	 * operations to be complete. Read MEM_CLK_FREQ_VAL bit to see if
	 * the DIMMs are present in this node.
	 */
	u8 node;
	u32 val;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		mct_sync_dcts_ready(p_dct_stat);
	}

	if (!is_fam15h()) {
		/* v6.1.3 */
		/* re-enable phy compensation engine when dram init is completed on all nodes.
		 */
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			struct DCTStatStruc *p_dct_stat;
			p_dct_stat = p_dct_stat_a + node;
			if (p_dct_stat->node_present) {
				if (p_dct_stat->dimm_valid_dct[0] > 0
				|| p_dct_stat->dimm_valid_dct[1] > 0) {
					/*
					 * re-enable phy compensation engine
					 * when dram init on both DCTs is completed.
					 */
					val = get_nb32_index_wait(p_dct_stat->dev_dct,
								0x98, 0x8);
					val &= ~(1 << DIS_AUTO_COMP);
					set_nb32_index_wait(p_dct_stat->dev_dct,
								0x98, 0x8, val);
				}
			}
		}
	}

	/* wait 750us before any memory access can be made. */
	mct_wait(15000);
}

void startup_dct_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Read MEM_CLK_FREQ_VAL bit to see if the DIMMs are present in this node.
	 * If the DIMMs are present then set the DRAM Enable bit for this node.
	 *
	 * Setting dram init starts up the DCT state machine, initializes the
	 * dram devices with MRS commands, and kicks off any
	 * HW memory clear process that the chip is capable of.	The sooner
	 * that dram init is set for all nodes, the faster the memory system
	 * initialization can complete.	Thus, the init loop is unrolled into
	 * two loops so as to start the processes for non BSP nodes sooner.
	 * This procedure will not wait for the process to finish.
	 * Synchronization is handled elsewhere.
	 */
	u32 val;
	u32 dev;

	dev = p_dct_stat->dev_dct;
	val = get_nb32_dct(dev, dct, 0x94);
	if (val & (1 << MEM_CLK_FREQ_VAL)) {
		mct_hook_before_dram_init();	/* generalized Hook */
		if (!(p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW)))
		    mct_dram_init(p_mct_stat, p_dct_stat, dct);
		after_dram_init_d(p_dct_stat, dct);
		mct_hook_after_dram_init();		/* generalized Hook*/
	}
}

static void clear_dct_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 reg_end;
	u32 dev = p_dct_stat->dev_dct;
	u32 reg = 0x40;
	u32 val = 0;

	if (p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW)) {
		reg_end = 0x78;
	} else {
		reg_end = 0xA4;
	}

	while (reg < reg_end) {
		if ((reg & 0xFF) == 0x84) {
			if (is_fam15h()) {
				val = get_nb32_dct(dev, dct, reg);
				/* Clear PCHG_PD_MODE_SEL */
				val &= ~(0x1 << 23);
				/* Clear BurstCtrl */
				val &= ~0x3;
			}
		}
		if ((reg & 0xFF) == 0x90) {
			if (p_dct_stat->logical_cpuid & AMD_DR_Dx) {
				/* get DRAMConfigLow */
				val = get_nb32_dct(dev, dct, reg);
				/* preserve value of DisDllShutdownSR for only Rev.D */
				val |= 0x08000000;
			}
		}
		set_nb32_dct(dev, dct, reg, val);
		val = 0;
		reg += 4;
	}

	val = 0;
	dev = p_dct_stat->dev_map;
	reg = 0xF0;
	set_nb32(dev, reg, val);
}

void spd_2nd_timing(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 i;
	u16 twr, trtp;
	u16 trp, trrd, trcd, tras, trc;
	u8 trfc[4];
	u16 tfaw;
	u16 tcwl;	/* Fam15h only */
	u32 dram_timing_lo, dram_timing_hi;
	u8 tck_16x;
	u16 twtr;
	u8 etr[2];
	u8 l_dimm;
	u8 mtb_16x;
	u8 byte;
	u32 dword;
	u32 dev;
	u32 val;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	/* Gather all dimm mini-max values for cycle timing data */
	trp = 0;
	trrd = 0;
	trcd = 0;
	trtp = 0;
	tras = 0;
	trc = 0;
	twr = 0;
	twtr = 0;
	for (i = 0; i < 2; i++)
		etr[i] = 0;
	for (i = 0; i < 4; i++)
		trfc[i] = 0;
	tfaw = 0;

	for (i = 0; i < MAX_DIMMS_SUPPORTED; i++) {
		l_dimm = i >> 1;
		if (p_dct_stat->dimm_valid & (1 << i)) {
			/* MTB = Dividend/Divisor */
			val = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_MTB_DIVISOR];
			mtb_16x = ((p_dct_stat->spd_data.spd_bytes[dct + i][SPD_MTB_DIVIDEND]
				& 0xff) << 4);
			/* transfer to MTB*16 */
			mtb_16x /= val;

			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TRPD_MIN];
			val = byte * mtb_16x;
			if (trp < val)
				trp = val;

			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TRRD_MIN];
			val = byte * mtb_16x;
			if (trrd < val)
				trrd = val;

			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TRCD_MIN];
			val = byte * mtb_16x;
			if (trcd < val)
				trcd = val;

			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TRTP_MIN];
			val = byte * mtb_16x;
			if (trtp < val)
				trtp = val;

			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TWR_MIN];
			val = byte * mtb_16x;
			if (twr < val)
				twr = val;

			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TWTR_MIN];
			val = byte * mtb_16x;
			if (twtr < val)
				twtr = val;

			val = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_UPPER_TRAS_TRC]
				& 0xff;
			val >>= 4;
			val <<= 8;
			val |= p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TRC_MIN] & 0xff;
			val *= mtb_16x;
			if (trc < val)
				trc = val;

			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_DENSITY] & 0xf;
			if (trfc[l_dimm] < byte)
				trfc[l_dimm] = byte;

			val = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_UPPER_TRAS_TRC] & 0xf;
			val <<= 8;
			val |= (p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TRAS_MIN] & 0xff);
			val *= mtb_16x;
			if (tras < val)
				tras = val;

			val = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_UPPER_TFAW] & 0xf;
			val <<= 8;
			val |= p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TFAW_MIN] & 0xff;
			val *= mtb_16x;
			if (tfaw < val)
				tfaw = val;

			/* Determine if the DIMMs on this channel support 95°C ETR */
			if (p_dct_stat->spd_data.spd_bytes[dct + i][SPD_THERMAL] & 0x1)
				etr[dct] = 1;
		}	/* Dimm Present */
	}

	/* Convert  DRAM CycleTiming values and store into DCT structure */
	byte = p_dct_stat->dimm_auto_speed;
	if (is_fam15h()) {
		if (byte == 0x16)
			tck_16x = 17;
		else if (byte == 0x12)
			tck_16x = 20;
		else if (byte == 0xe)
			tck_16x = 24;
		else if (byte == 0xa)
			tck_16x = 30;
		else if (byte == 0x6)
			tck_16x = 40;
		else
			tck_16x = 48;
	} else {
		if (byte == 7)
			tck_16x = 20;
		else if (byte == 6)
			tck_16x = 24;
		else if (byte == 5)
			tck_16x = 30;
		else
			tck_16x = 40;
	}

	/* Notes:
	 1. All secondary time values given in SPDs are in binary with units of ns.
	 2. Some time values are scaled by 16, in order to have least count of 0.25 ns
	    (more accuracy).  JEDEC SPD spec. shows which ones are x1 and x4.
	 3. Internally to this SW, cycle time, tck_16x, is scaled by 16 to match time values
	*/

	/* tras */
	p_dct_stat->dimm_tras = (u16)tras;
	val = tras / tck_16x;
	if (tras % tck_16x) {	/* round up number of busclocks */
		val++;
	}
	if (val < MIN_TRAST)
		val = MIN_TRAST;
	else if (val > MAX_TRAST)
		val = MAX_TRAST;
	p_dct_stat->tras = val;

	/* trp */
	p_dct_stat->dimm_trp = trp;
	val = trp / tck_16x;
	if (trp % tck_16x) {	/* round up number of busclocks */
		val++;
	}
	if (val < MIN_TRPT)
		val = MIN_TRPT;
	else if (val > MAX_TRPT)
		val = MAX_TRPT;
	p_dct_stat->trp = val;

	/* trrd */
	p_dct_stat->dimm_trrd = trrd;
	val = trrd / tck_16x;
	if (trrd % tck_16x) {	/* round up number of busclocks */
		val++;
	}
	if (val < MIN_TRRDT)
		val = MIN_TRRDT;
	else if (val > MAX_TRRDT)
		val = MAX_TRRDT;
	p_dct_stat->trrd = val;

	/* trcd */
	p_dct_stat->dimm_trcd = trcd;
	val = trcd / tck_16x;
	if (trcd % tck_16x) {	/* round up number of busclocks */
		val++;
	}
	if (val < MIN_TRCDT)
		val = MIN_TRCDT;
	else if (val > MAX_TRCDT)
		val = MAX_TRCDT;
	p_dct_stat->trcd = val;

	/* trc */
	p_dct_stat->dimm_trc = trc;
	val = trc / tck_16x;
	if (trc % tck_16x) {	/* round up number of busclocks */
		val++;
	}
	if (val < MIN_TRCT)
		val = MIN_TRCT;
	else if (val > MAX_TRCT)
		val = MAX_TRCT;
	p_dct_stat->trc = val;

	/* trtp */
	p_dct_stat->dimm_trtp = trtp;
	val = trtp / tck_16x;
	if (trtp % tck_16x) {
		val ++;
	}
	if (val < MIN_TRTPT)
		val = MIN_TRTPT;
	else if (val > MAX_TRTPT)
		val = MAX_TRTPT;
	p_dct_stat->trtp = val;

	/* twr */
	p_dct_stat->dimm_twr = twr;
	val = twr / tck_16x;
	if (twr % tck_16x) {	/* round up number of busclocks */
		val++;
	}
	if (val < MIN_TWRT)
		val = MIN_TWRT;
	else if (val > MAX_TWRT)
		val = MAX_TWRT;
	p_dct_stat->twr = val;

	/* twtr */
	p_dct_stat->dimm_twtr = twtr;
	val = twtr / tck_16x;
	if (twtr % tck_16x) {	/* round up number of busclocks */
		val++;
	}
	if (val < MIN_TWTRT)
		val = MIN_TWTRT;
	else if (val > MAX_TWTRT)
		val = MAX_TWTRT;
	p_dct_stat->twtr = val;

	/* Trfc0-Trfc3 */
	for (i = 0; i < 4; i++)
		p_dct_stat->trfc[i] = trfc[i];

	/* tfaw */
	p_dct_stat->dimm_tfaw = tfaw;
	val = tfaw / tck_16x;
	if (tfaw % tck_16x) {	/* round up number of busclocks */
		val++;
	}
	if (val < Min_TfawT)
		val = Min_TfawT;
	else if (val > Max_TfawT)
		val = Max_TfawT;
	p_dct_stat->tfaw = val;

	mct_adjust_auto_cyc_tmg_d();

	if (is_fam15h()) {
		/* Compute tcwl (Fam15h BKDG v3.14 Table 203) */
		if (p_dct_stat->speed <= 0x6)
			tcwl = 0x5;
		else if (p_dct_stat->speed == 0xa)
			tcwl = 0x6;
		else if (p_dct_stat->speed == 0xe)
			tcwl = 0x7;
		else if (p_dct_stat->speed == 0x12)
			tcwl = 0x8;
		else if (p_dct_stat->speed == 0x16)
			tcwl = 0x9;
		else
			tcwl = 0x5;	/* Power-on default */

		/* Apply offset */
		tcwl += p_dct_stat->tcwl_delay[dct];
	}

	/* Program DRAM Timing values */
	if (is_fam15h()) {
		dev = p_dct_stat->dev_dct;

		/* DRAM Timing High */
		dword = get_nb32_dct(dev, dct, 0x8c);
		if (etr[dct]) {
			/* Tref = 3.9us */
			val = 3;
		}
		else {
			/* Tref = 7.8us */
			val = 2;
		}
		dword &= ~(0x3 << 16);
		dword |= (val & 0x3) << 16;
		/* DRAM Timing High */
		set_nb32_dct(dev, dct, 0x8c, dword);

		/* DRAM Timing 0 */
		dword = get_nb32_dct(dev, dct, 0x200);
		dword &= ~(0x3f1f1f1f);
		/* tras */
		dword |= (p_dct_stat->tras & 0x3f) << 24;
		val = p_dct_stat->trp;
		val = mct_adjust_spd_timings(p_mct_stat, p_dct_stat, val);
		/* trp */
		dword |= (val & 0x1f) << 16;
		/* trcd */
		dword |= (p_dct_stat->trcd & 0x1f) << 8;
		/* Tcl */
		dword |= (p_dct_stat->cas_latency & 0x1f);
		/* DRAM Timing 0 */
		set_nb32_dct(dev, dct, 0x200, dword);

		/* DRAM Timing 1 */
		dword = get_nb32_dct(dev, dct, 0x204);
		dword &= ~(0x0f3f0f3f);
		/* trtp */
		dword |= (p_dct_stat->trtp & 0xf) << 24;
		if (p_dct_stat->tfaw != 0) {
			val = p_dct_stat->tfaw;
			val = mct_adjust_spd_timings(p_mct_stat, p_dct_stat, val);
			if ((val > 0x5) && (val < 0x2b)) {
				/* FourActWindow */
				dword |= (val & 0x3f) << 16;
			}
		}
		/* trrd */
		dword |= (p_dct_stat->trrd & 0xf) << 8;
		/* trc */
		dword |= (p_dct_stat->trc & 0x3f);
		/* DRAM Timing 1 */
		set_nb32_dct(dev, dct, 0x204, dword);

		/* Trfc0-Trfc3 */
		for (i = 0; i < 4; i++)
			if (p_dct_stat->trfc[i] == 0x0)
				p_dct_stat->trfc[i] = 0x1;
		/* DRAM Timing 2 */
		dword = get_nb32_dct(dev, dct, 0x208);
		dword &= ~(0x07070707);
		/* Trfc3 */
		dword |= (p_dct_stat->trfc[3] & 0x7) << 24;
		/* Trfc2 */
		dword |= (p_dct_stat->trfc[2] & 0x7) << 16;
		/* Trfc1 */
		dword |= (p_dct_stat->trfc[1] & 0x7) << 8;
		/* Trfc0 */
		dword |= (p_dct_stat->trfc[0] & 0x7);
		/* DRAM Timing 2 */
		set_nb32_dct(dev, dct, 0x208, dword);

		/* DRAM Timing 3 */
		dword = get_nb32_dct(dev, dct, 0x20c);
		dword &= ~(0x00000f00);
		/* twtr */
		dword |= (p_dct_stat->twtr & 0xf) << 8;
		dword &= ~(0x0000001f);
		/* tcwl */
		dword |= (tcwl & 0x1f);
		/* DRAM Timing 3 */
		set_nb32_dct(dev, dct, 0x20c, dword);

		/* DRAM Timing 10 */
		dword = get_nb32_dct(dev, dct, 0x22c);
		dword &= ~(0x0000001f);
		/* twr */
		dword |= (p_dct_stat->twr & 0x1f);
		/* DRAM Timing 10 */
		set_nb32_dct(dev, dct, 0x22c, dword);

		if (p_dct_stat->speed > mhz_to_memclk_config(mct_get_nv_bits(NV_MIN_MEMCLK))) {
			/* Enable phy-assisted training mode */
			fam_15_enable_training_mode(p_mct_stat, p_dct_stat, dct, 1);
		}

		/* Other setup (not training specific) */
		/* DRAM Configuration Low */
		dword = get_nb32_dct(dev, dct, 0x90);
		/* FORCE_AUTO_PCHG = 0 */
		dword &= ~(0x1 << 23);
		/* DynPageCloseEn = 0 */
		dword &= ~(0x1 << 20);
		/* DRAM Configuration Low */
		set_nb32_dct(dev, dct, 0x90, dword);

		/* DRAM Timing 9 */
		set_nb32_dct(dev, dct, 0x228, 0x14141414);
	} else {
		/* Dram Timing Low init */
		dram_timing_lo = 0;
		/* p_dct_stat.cas_latency to reg. definition */
		val = p_dct_stat->cas_latency - 4;
		dram_timing_lo |= val;

		val = p_dct_stat->trcd - BIAS_TRCDT;
		dram_timing_lo |= val << 4;

		val = p_dct_stat->trp - BIAS_TRPT;
		val = mct_adjust_spd_timings(p_mct_stat, p_dct_stat, val);
		dram_timing_lo |= val << 7;

		val = p_dct_stat->trtp - BIAS_TRTPT;
		dram_timing_lo |= val << 10;

		val = p_dct_stat->tras - BIAS_TRAST;
		dram_timing_lo |= val << 12;

		val = p_dct_stat->trc - BIAS_TRCT;
		dram_timing_lo |= val << 16;

		val = p_dct_stat->trrd - BIAS_TRRDT;
		dram_timing_lo |= val << 22;

		/* Dram Timing High init */
		dram_timing_hi = 0;
		val = p_dct_stat->twtr - BIAS_TWTRT;
		dram_timing_hi |= val << 8;

		/* Tref = 7.8us */
		val = 2;
		dram_timing_hi |= val << 16;

		val = 0;
		for (i = 4; i > 0; i--) {
			val <<= 3;
			val |= trfc[i-1];
		}
		dram_timing_hi |= val << 20;

		dev = p_dct_stat->dev_dct;
		/* twr */
		val = p_dct_stat->twr;
		if (val == 10)
			val = 9;
		else if (val == 12)
			val = 10;
		val = mct_adjust_spd_timings(p_mct_stat, p_dct_stat, val);
		val -= BIAS_TWRT;
		val <<= 4;
		dword = get_nb32_dct(dev, dct, 0x84);
		dword &= ~0x70;
		dword |= val;
		set_nb32_dct(dev, dct, 0x84, dword);

		/* tfaw */
		val = p_dct_stat->tfaw;
		val = mct_adjust_spd_timings(p_mct_stat, p_dct_stat, val);
		val -= BIAS_TFAW_T;
		val >>= 1;
		val <<= 28;
		dword = get_nb32_dct(dev, dct, 0x94);
		dword &= ~0xf0000000;
		dword |= val;
		set_nb32_dct(dev, dct, 0x94, dword);

		/* dev = p_dct_stat->dev_dct; */

		if (p_dct_stat->speed > mhz_to_memclk_config(mct_get_nv_bits(NV_MIN_MEMCLK))) {
			val = get_nb32_dct(dev, dct, 0x88);
			val &= 0xFF000000;
			dram_timing_lo |= val;
		}
		set_nb32_dct(dev, dct, 0x88, dram_timing_lo);	/*DCT Timing Low*/

		if (p_dct_stat->speed > mhz_to_memclk_config(mct_get_nv_bits(NV_MIN_MEMCLK))) {
			dram_timing_hi |= 1 << DIS_AUTO_REFRESH;
		}
		dram_timing_hi |= 0x000018FF;
		set_nb32_dct(dev, dct, 0x8c, dram_timing_hi);	/*DCT Timing Hi*/
	}

	/* dump_pci_device(PCI_DEV(0, 0x18+p_dct_stat->node_id, 2)); */

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

static u8 auto_cyc_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Initialize  DCT Timing registers as per dimm SPD.
	 * For primary timing (T, CL) use best case T value.
	 * For secondary timing params., use most aggressive settings
	 * of slowest dimm.
	 *
	 * There are three components to determining "maximum frequency":
	 * SPD component, Bus load component, and "Preset" max frequency
	 * component.
	 *
	 * The SPD component is a function of the min cycle time specified
	 * by each dimm, and the interaction of cycle times from all DIMMs
	 * in conjunction with CAS latency. The SPD component only applies
	 * when user timing mode is 'Auto'.
	 *
	 * The Bus load component is a limiting factor determined by electrical
	 * characteristics on the bus as a result of varying number of device
	 * loads. The Bus load component is specific to each platform but may
	 * also be a function of other factors. The bus load component only
	 * applies when user timing mode is 'Auto'.
	 *
	 * The Preset component is subdivided into three items and is
	 * the minimum of the set: Silicon revision, user limit
	 * setting when user timing mode is 'Auto' and memclock mode
	 * is 'Limit', OEM build specification of the maximum
	 * frequency. The Preset component is only applies when user
	 * timing mode is 'Auto'.
	 */

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	/* Get primary timing (CAS Latency and Cycle Time) */
	if (p_dct_stat->speed == 0) {
		mct_get_max_load_freq(p_dct_stat);

		/* and Factor in presets (setup options, Si cap, etc.) */
		get_preset_max_f_d(p_mct_stat, p_dct_stat);

		/* Go get best T and CL as specified by dimm mfgs. and OEM */
		spd_get_tcl_d(p_mct_stat, p_dct_stat, dct);

		/* skip callback mctForce800to1067_D */
		p_dct_stat->speed = p_dct_stat->dimm_auto_speed;
		p_dct_stat->cas_latency = p_dct_stat->dimm_casl;

	}
	mct_after_get_clt(p_mct_stat, p_dct_stat, dct);

	spd_2nd_timing(p_mct_stat, p_dct_stat, dct);

	printk(BIOS_DEBUG, "AutoCycTiming: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "AutoCycTiming: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "AutoCycTiming: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "AutoCycTiming: Done\n\n");

	mct_hook_after_auto_cyc_tmg();

	return p_dct_stat->err_code;
}

static void get_preset_max_f_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Get max frequency from OEM platform definition, from any user
	 * override (limiting) of max frequency, and from any Si Revision
	 * Specific information.  Return the least of these three in
	 * DCTStatStruc.preset_max_freq.
	 */
	/* TODO: Set the proper max frequency in wrappers/mcti_d.c. */
	u16 proposedFreq;
	u16 word;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	/* Get CPU Si Revision defined limit (NPT) */
	if (is_fam15h())
		proposedFreq = 933;
	else
		proposedFreq = 800;	 /* Rev F0 programmable max memclock is */

	/*Get User defined limit if  "limit" mode */
	if (mct_get_nv_bits(NV_MCT_USR_TMG_MODE) == 1) {
		word = get_fk_d(mct_get_nv_bits(NV_MEM_CK_VAL) + 1);
		if (word < proposedFreq)
			proposedFreq = word;

		/* Get Platform defined limit */
		word = mct_get_nv_bits(NV_MAX_MEMCLK);
		if (word < proposedFreq)
			proposedFreq = word;

		word = p_dct_stat->preset_max_freq;
		if (word > proposedFreq)
			word = proposedFreq;

		p_dct_stat->preset_max_freq = word;
	}
	/* Check F3xE8[DdrMaxRate] for maximum DRAM data rate support */

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

static void spd_get_tcl_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Find the best T and CL primary timing parameter pair, per Mfg.,
	 * for the given set of DIMMs, and store into DCTStatStruc
	 * (.dimm_auto_speed and .dimm_casl). See "Global relationship between
	 *  index values and item values" for definition of CAS latency
	 *  index (j) and Frequency index (k).
	 */
	u8 i, cas_lat_low, cas_lat_high;
	u16 taa_min_16x;
	u8 mtb_16x;
	u16 tck_min_16x;
	u16 tck_proposed_16x;
	u8 cl_actual, cl_desired, clt_fail;
	u16 min_frequency_tck16x;

	u8 byte = 0, bytex = 0;

	cas_lat_low = 0xFF;
	cas_lat_high = 0xFF;
	taa_min_16x = 0;
	tck_min_16x = 0;
	clt_fail = 0;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	if (is_fam15h()) {
		u16 minimum_frequency_mhz = mct_get_nv_bits(NV_MIN_MEMCLK);
		if (minimum_frequency_mhz == 0)
			minimum_frequency_mhz = 333;
		min_frequency_tck16x = 16000 / minimum_frequency_mhz;
	} else {
		min_frequency_tck16x = 40;
	}

	for (i = 0; i < MAX_DIMMS_SUPPORTED; i++) {
		if (p_dct_stat->dimm_valid & (1 << i)) {
			/* Step 1: Determine the common set of supported CAS Latency
			 * values for all modules on the memory channel using the CAS
			 * Latencies Supported in SPD bytes 14 and 15.
			 */
			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_CAS_LOW];
			cas_lat_low &= byte;
			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_CAS_HIGH];
			cas_lat_high &= byte;
			/* Step 2: Determine tAAmin(all) which is the largest tAAmin
			   value for all modules on the memory channel (SPD byte 16). */
			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_MTB_DIVISOR];

			mtb_16x = ((p_dct_stat->spd_data.spd_bytes[dct + i][SPD_MTB_DIVIDEND]
					& 0xFF) << 4);
			mtb_16x /= byte; /* transfer to MTB*16 */

			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TAA_MIN];
			if (taa_min_16x < byte * mtb_16x)
				taa_min_16x = byte * mtb_16x;
			/* Step 3: Determine tCKmin(all) which is the largest tCKmin
			   value for all modules on the memory channel (SPD byte 12). */
			byte = p_dct_stat->spd_data.spd_bytes[dct + i][SPD_TCK_MIN];

			if (tck_min_16x < byte * mtb_16x)
				tck_min_16x = byte * mtb_16x;
		}
	}
	/* calculate tck_proposed_16x (proposed clock period in ns * 16) */
	tck_proposed_16x =  16000 / p_dct_stat->preset_max_freq;
	if (tck_min_16x > tck_proposed_16x)
		tck_proposed_16x = tck_min_16x;

	/* TODO: get user manual tck_16x(Freq.)
	 * and overwrite current tck_proposed_16x if manual.
	 */
	if (is_fam15h()) {
		if (tck_proposed_16x == 17)
			p_dct_stat->target_freq = 0x16;
		else if (tck_proposed_16x <= 20) {
			p_dct_stat->target_freq = 0x12;
			tck_proposed_16x = 20;
		} else if (tck_proposed_16x <= 24) {
			p_dct_stat->target_freq = 0xe;
			tck_proposed_16x = 24;
		} else if (tck_proposed_16x <= 30) {
			p_dct_stat->target_freq = 0xa;
			tck_proposed_16x = 30;
		} else if (tck_proposed_16x <= 40) {
			p_dct_stat->target_freq = 0x6;
			tck_proposed_16x = 40;
		} else {
			p_dct_stat->target_freq = 0x4;
			tck_proposed_16x = 48;
		}
	} else {
		if (tck_proposed_16x == 20)
			p_dct_stat->target_freq = 7;
		else if (tck_proposed_16x <= 24) {
			p_dct_stat->target_freq = 6;
			tck_proposed_16x = 24;
		} else if (tck_proposed_16x <= 30) {
			p_dct_stat->target_freq = 5;
			tck_proposed_16x = 30;
		} else {
			p_dct_stat->target_freq = 4;
			tck_proposed_16x = 40;
		}
	}
	/* Running through this loop twice:
	   - First time find tCL at target frequency
	   - Second time find tCL at 400MHz */

	for (;;) {
		clt_fail = 0;
		/* Step 4: For a proposed tCK value (tCKproposed) between tCKmin(all) and
		   tCKmax, determine the desired CAS Latency. If tCKproposed is not a standard
		   JEDEC value (2.5, 1.875, 1.5, or 1.25 ns) then tCKproposed must be adjusted
		   to the next lower standard tCK value for calculating cl_desired.
		   cl_desired = ceiling (tAAmin(all) / tCKproposed)
		   where tAAmin is defined in Byte 16. The ceiling function requires that the
		   quotient be rounded up always. */
		cl_desired = taa_min_16x / tck_proposed_16x;
		if (taa_min_16x % tck_proposed_16x)
			cl_desired ++;
		/* Step 5: Chose an actual CAS Latency (cl_actual) that is greather than or
		   equal to cl_desired and is supported by all modules on the memory channel as
		   determined in step 1. If no such value exists, choose a higher tCKproposed
		   value and repeat steps 4 and 5 until a solution is found. */
		for (i = 0, cl_actual = 4; i < 15; i++, cl_actual++) {
			if ((cas_lat_high << 8 | cas_lat_low) & (1 << i)) {
				if (cl_desired <= cl_actual)
					break;
			}
		}
		if (i == 15)
			clt_fail = 1;
		/* Step 6: Once the calculation of cl_actual is completed, the BIOS must also
		   verify that this CAS Latency value does not exceed tAAmax, which is 20 ns
		   for all DDR3 speed grades, by multiplying cl_actual times tCKproposed. If
		   not, choose a lower CL value and repeat steps 5 and 6 until a solution is
		   found. */
		if (cl_actual * tck_proposed_16x > 320)
			clt_fail = 1;
		/* get CL and T */
		if (!clt_fail) {
			bytex = cl_actual;
			if (is_fam15h()) {
				if (tck_proposed_16x == 17)
					byte = 0x16;
				else if (tck_proposed_16x == 20)
					byte = 0x12;
				else if (tck_proposed_16x == 24)
					byte = 0xe;
				else if (tck_proposed_16x == 30)
					byte = 0xa;
				else if (tck_proposed_16x == 40)
					byte = 0x6;
				else
					byte = 0x4;
			} else {
				if (tck_proposed_16x == 20)
					byte = 7;
				else if (tck_proposed_16x == 24)
					byte = 6;
				else if (tck_proposed_16x == 30)
					byte = 5;
				else
					byte = 4;
			}
		} else {
			/* mctHookManualCLOverride */
			/* TODO: */
		}

		if (tck_proposed_16x != min_frequency_tck16x) {
			if (p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW)) {
				p_dct_stat->dimm_auto_speed = byte;
				p_dct_stat->dimm_casl = bytex;
				break;
			} else {
				p_dct_stat->target_casl = bytex;
				tck_proposed_16x = min_frequency_tck16x;
			}
		} else {
			p_dct_stat->dimm_auto_speed = byte;
			p_dct_stat->dimm_casl = bytex;
			break;
		}
	}

	printk(BIOS_DEBUG, "spd_get_tcl_d: dimm_casl %x\n", p_dct_stat->dimm_casl);
	printk(BIOS_DEBUG, "spd_get_tcl_d: dimm_auto_speed %x\n", p_dct_stat->dimm_auto_speed);

	printk(BIOS_DEBUG, "spd_get_tcl_d: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "spd_get_tcl_d: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "spd_get_tcl_d: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "spd_get_tcl_d: Done\n\n");
}

u8 platform_spec_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	if (!is_fam15h()) {
		mct_get_ps_cfg_d(p_mct_stat, p_dct_stat, dct);

		if (p_dct_stat->ganged_mode == 1) {
			mct_get_ps_cfg_d(p_mct_stat, p_dct_stat, 1);
			mct_before_platform_spec(p_mct_stat, p_dct_stat, 1);
		}

		set_2t_configuration(p_mct_stat, p_dct_stat, dct);

		mct_before_platform_spec(p_mct_stat, p_dct_stat, dct);
		mct_platform_spec(p_mct_stat, p_dct_stat, dct);
		if (p_dct_stat->dimm_auto_speed
		== mhz_to_memclk_config(mct_get_nv_bits(NV_MIN_MEMCLK)))
			init_phy_compensation(p_mct_stat, p_dct_stat, dct);
	}
	mct_hook_after_ps_cfg();

	return p_dct_stat->err_code;
}

static u8 auto_config_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 dram_control, dram_timing_lo, status;
	u32 dram_config_lo, dram_config_hi, dram_config_misc, dram_config_misc_2;
	u32 val;
	u32 dev;
	u16 word;
	u32 dword;
	u8 byte;
	u32 offset;

	dram_config_lo = 0;
	dram_config_hi = 0;
	dram_config_misc = 0;
	dram_config_misc_2 = 0;

	/* set bank addressing and Masks, plus CS pops */
	spd_set_banks_d(p_mct_stat, p_dct_stat, dct);
	if (p_dct_stat->err_code == SC_STOP_ERR)
		goto AutoConfig_exit;

	/* map chip-selects into local address space */
	stitch_memory_d(p_mct_stat, p_dct_stat, dct);
	interleave_banks_d(p_mct_stat, p_dct_stat, dct);

	/* temp image of status (for convenience). RO usage! */
	status = p_dct_stat->status;

	dev = p_dct_stat->dev_dct;

	/* Build Dram Control Register Value */
	dram_config_misc_2 = get_nb32_dct(dev, dct, 0xa8);	/* Dram Miscellaneous 2 */
	dram_control = get_nb32_dct(dev, dct, 0x78);		/* Dram Control */

	/* FIXME: Skip mct_checkForDxSupport */
	/* REV_CALL mct_DoRdPtrInit if not Dx */
	if (p_dct_stat->logical_cpuid & AMD_DR_Bx)
		val = 5;
	else
		val = 6;
	dram_control &= ~0xFF;
	dram_control |= val;	/* RdPtrInit = 6 for Cx CPU */

	if (mct_get_nv_bits(NV_CLK_HZ_ALT_VID_C3))
		dram_control |= 1 << 16; /* check */

	dram_control |= 0x00002A00;

	/* FIXME: Skip for Ax versions */
	/* callback not required - if (!mctParityControl_D()) */
	if (status & (1 << SB_128_BIT_MODE))
		dram_config_lo |= 1 << WIDTH_128;	/* 128-bit mode (normal) */

	word = dct;
	dword = X4_DIMM;
	while (word < 8) {
		if (p_dct_stat->dimm_x4_present & (1 << word))
			dram_config_lo |= 1 << dword;	/* X4_DIMM[3:0] */
		word++;
		word++;
		dword++;
	}

	if (status & (1 << SB_REGISTERED)) {
		/* Registered DIMMs */
		if (!is_fam15h()) {
			dram_config_lo |= 1 << PAR_EN;
		}
	} else {
		/* Unbuffered DIMMs */
		dram_config_lo |= 1 << UN_BUFF_DIMM;
	}

	if (mct_get_nv_bits(NV_ECC_CAP))
		if (status & (1 << SB_ECC_DIMMS))
			if (mct_get_nv_bits(NV_ECC))
				dram_config_lo |= 1 << DIMM_EC_EN;

	dram_config_lo = mct_dis_dll_shutdown_sr(p_mct_stat, p_dct_stat, dram_config_lo, dct);

	/* Build Dram Config Hi Register Value */
	if (is_fam15h())
		offset = 0x0;
	else
		offset = 0x1;
	dword = p_dct_stat->speed;
	dram_config_hi |= dword - offset;	/* get MemClk encoding */
	dram_config_hi |= 1 << MEM_CLK_FREQ_VAL;

	if (!is_fam15h())
		if (status & (1 << SB_REGISTERED))
			if ((p_dct_stat->dimm_x4_present != 0)
			&& (p_dct_stat->dimm_x8_present != 0))
				/* set only if x8 Registered DIMMs in System*/
				dram_config_hi |= 1 << RDQS_EN;

	if (p_dct_stat->logical_cpuid & AMD_FAM15_ALL) {
		dram_config_lo |= 1 << 25;	/* PendRefPaybackS3En = 1 */
		dram_config_lo |= 1 << 24;	/* StagRefEn = 1 */
		dram_config_hi |= 1 << 16;	/* PowerDownMode = 1 */
	} else {
		if (mct_get_nv_bits(NV_CKE_CTL))
			/*Chip Select control of CKE*/
			dram_config_hi |= 1 << 16;
	}

	if (!is_fam15h()) {
		/* Control Bank Swizzle */
		if (0) {
			/* call back not needed mctBankSwizzleControl_D()) */
			dram_config_hi &= ~(1 << BANK_SWIZZLE_MODE);
		}
		else {
			/* recommended setting (default) */
			dram_config_hi |= 1 << BANK_SWIZZLE_MODE;
		}
	}

	/* Check for Quadrank dimm presence */
	if (p_dct_stat->dimm_qr_present != 0) {
		byte = mct_get_nv_bits(NV_4_RANK_TYPE);
		if (byte == 2)
			dram_config_hi |= 1 << 17;	/* S4 (4-Rank SO-DIMMs) */
		else if (byte == 1)
			dram_config_hi |= 1 << 18;	/* R4 (4-Rank Registered DIMMs) */
	}

	if (0) /* call back not needed mctOverrideDcqBypMax_D) */
		val = mct_get_nv_bits(NV_BYP_MAX);
	else
		val = 0x0f; /* recommended setting (default) */
	dram_config_hi |= val << 24;

	if (p_dct_stat->logical_cpuid & (AMD_DR_Dx | AMD_DR_Cx | AMD_DR_Bx | AMD_FAM15_ALL))
		dram_config_hi |= 1 << DCQ_ARB_BYPASS_EN;

	/* Build MemClkDis Value from Dram Timing Lo and
	   Dram Config Misc Registers
	 1. We will assume that MemClkDis field has been preset prior to this
	    point.
	 2. We will only set MemClkDis bits if a dimm is NOT present AND if:
	    NV_ALL_MEM_CLKS <>0 AND SB_DIAG_CLKS == 0 */

	/* Dram Timing Low (owns Clock Enable bits) */
	dram_timing_lo = get_nb32_dct(dev, dct, 0x88);
	if (mct_get_nv_bits(NV_ALL_MEM_CLKS) == 0) {
		/* Special Jedec SPD diagnostic bit - "enable all clocks" */
		if (!(p_dct_stat->status & (1 << SB_DIAG_CLKS))) {
			const u8 *p;
			p = tab_manual_clk_dis;

			byte = mct_get_nv_bits(NV_PACK_TYPE);
			if (byte == PT_L1)
				p = tab_l1_clk_dis;
			else if (byte == PT_M2 || byte == PT_AS)
				p = tab_am3_clk_dis;
			else if (byte == PT_C3)
				p = tab_c32_clk_dis;
			else if (byte == PT_GR)
				p = tab_g34_clk_dis;
			else if (byte == PT_FM2)
				p = tab_fm2_clk_dis;
			else
				p = tab_s1_clk_dis;

			dword = 0;
			byte = 0xFF;
			while (dword < MAX_CS_SUPPORTED) {
				if (p_dct_stat->cs_present & (1 << dword)) {
					/* re-enable clocks for the enabled CS */
					val = p[dword];
					byte &= ~val;
				}
				dword++;
			}
			dram_timing_lo &= ~(0xff << 24);
			dram_timing_lo |= byte << 24;
		}
	}

	dram_config_misc_2 = mct_set_dram_config_misc_2(
					p_dct_stat, dct, dram_config_misc_2, dram_control);

	printk(BIOS_DEBUG, "auto_config_d: dram_control:     %08x\n", dram_control);
	printk(BIOS_DEBUG, "auto_config_d: dram_timing_lo:    %08x\n", dram_timing_lo);
	printk(BIOS_DEBUG, "auto_config_d: dram_config_misc:  %08x\n", dram_config_misc);
	printk(BIOS_DEBUG, "auto_config_d: dram_config_misc_2: %08x\n", dram_config_misc_2);
	printk(BIOS_DEBUG, "auto_config_d: dram_config_lo:    %08x\n", dram_config_lo);
	printk(BIOS_DEBUG, "auto_config_d: dram_config_hi:    %08x\n", dram_config_hi);

	/* Write Values to the registers */
	set_nb32_dct(dev, dct, 0x78, dram_control);
	set_nb32_dct(dev, dct, 0x88, dram_timing_lo);
	set_nb32_dct(dev, dct, 0xa0, dram_config_misc);
	set_nb32_dct(dev, dct, 0xa8, dram_config_misc_2);
	set_nb32_dct(dev, dct, 0x90, dram_config_lo);
	prog_dram_mrs_reg_d(p_mct_stat, p_dct_stat, dct);

	if (is_fam15h())
		init_ddr_phy(p_mct_stat, p_dct_stat, dct);

	/* Write the DRAM Configuration High register, including memory frequency change */
	dword = get_nb32_dct(dev, dct, 0x94);
	dram_config_hi |= dword;
	mct_set_dram_config_hi_d(p_mct_stat, p_dct_stat, dct, dram_config_hi);
	mct_early_arb_en_d(p_mct_stat, p_dct_stat, dct);
	mct_hook_after_auto_cfg();

	/* dump_pci_device(PCI_DEV(0, 0x18+p_dct_stat->node_id, 2)); */

	printk(BIOS_DEBUG, "AutoConfig: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "AutoConfig: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "AutoConfig: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "AutoConfig: Done\n\n");

AutoConfig_exit:
	return p_dct_stat->err_code;
}

static void spd_set_banks_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Set bank addressing, program Mask values and build a chip-select
	 * population map. This routine programs PCI 0:24N:2x80 config register
	 * and PCI 0:24N:2x60,64,68,6C config registers (CS Mask 0-3).
	 */
	u8 chip_sel, rows, cols, ranks, banks;
	u32 bank_addr_reg, cs_mask;

	u32 val;
	u32 reg;
	u32 dev;
	u8 byte;
	u16 word;
	u32 dword;

	dev = p_dct_stat->dev_dct;

	bank_addr_reg = 0;
	for (chip_sel = 0; chip_sel < MAX_CS_SUPPORTED; chip_sel+=2) {
		byte = chip_sel;
		if ((p_dct_stat->status & (1 << SB_64_MUXED_MODE)) && chip_sel >=4)
			byte -= 3;

		if (p_dct_stat->dimm_valid & (1 << byte)) {
			byte = p_dct_stat->spd_data.spd_bytes[chip_sel + dct][SPD_ADDRESSING];
			rows = (byte >> 3) & 0x7; /* rows:0b = 12-bit,... */
			cols = byte & 0x7; /* cols:0b = 9-bit,... */

			byte = p_dct_stat->spd_data.spd_bytes[chip_sel + dct][SPD_DENSITY];
			banks = (byte >> 4) & 7; /* banks:0b = 3-bit,... */

			byte = p_dct_stat->spd_data.spd_bytes[chip_sel + dct][SPD_ORGANIZATION];
			ranks = ((byte >> 3) & 7) + 1;

			/* Configure Bank encoding
			 * Use a 6-bit key into a lookup table.
			 * Key (index) = RRRBCC, where CC is the number of Columns minus 9,
			 * RRR is the number of rows minus 12, and B is the number of banks
			 * minus 3.
			 */
			byte = cols;
			if (banks == 1)
				byte |= 4;

			byte |= rows << 3;	/* RRRBCC internal encode */

			for (dword = 0; dword < 13; dword++) {
				if (byte == tab_bank_addr[dword])
					break;
			}

			if (dword > 12)
				continue;

			/* bit no. of CS field in address mapping reg.*/
			dword <<= (chip_sel << 1);
			bank_addr_reg |= dword;

			/* Mask value=(2pow(rows+cols+banks+3)-1)>>8,
			   or 2pow(rows+cols+banks-5)-1*/
			cs_mask = 0;

			byte = rows + cols;		/* cl = rows+cols*/
			byte += 21;			/* row:12+col:9 */
			byte -= 2;			/* 3 banks - 5 */

			if (p_dct_stat->status & (1 << SB_128_BIT_MODE))
				byte++;		/* double mask size if in 128-bit mode*/

			cs_mask |= 1 << byte;
			cs_mask--;

			/*set ChipSelect population indicator even bits*/
			p_dct_stat->cs_present |= (1 << chip_sel);
			if (ranks >= 2)
				/*set ChipSelect population indicator odd bits*/
				p_dct_stat->cs_present |= 1 << (chip_sel + 1);

			reg = 0x60+(chip_sel << 1);	/*Dram CS Mask Register */
			val = cs_mask;
			val &= 0x1FF83FE0;	/* Mask out reserved bits.*/
			set_nb32_dct(dev, dct, reg, val);
		} else {
			if (p_dct_stat->dimm_spd_checksum_err & (1 << chip_sel))
				p_dct_stat->cs_test_fail |= (1 << chip_sel);
		}	/* if dimm_valid*/
	}	/* while chip_sel*/

	set_cs_tristate(p_mct_stat, p_dct_stat, dct);
	set_cke_tristate(p_mct_stat, p_dct_stat, dct);
	set_odt_tristate(p_mct_stat, p_dct_stat, dct);

	if (p_dct_stat->status & (1 << SB_128_BIT_MODE)) {
		set_cs_tristate(p_mct_stat, p_dct_stat, 1); /* force dct1) */
		set_cke_tristate(p_mct_stat, p_dct_stat, 1); /* force dct1) */
		set_odt_tristate(p_mct_stat, p_dct_stat, 1); /* force dct1) */
	}

	word = p_dct_stat->cs_present;
	mct_get_cs_exclude_map();		/* mask out specified chip-selects */
	word ^= p_dct_stat->cs_present;
	p_dct_stat->cs_test_fail |= word;	/* enable ODT to disabled DIMMs */
	if (!p_dct_stat->cs_present)
		p_dct_stat->err_code = SC_STOP_ERR;

	reg = 0x80;		/* Bank Addressing Register */
	set_nb32_dct(dev, dct, reg, bank_addr_reg);

	p_dct_stat->cs_present_dct[dct] = p_dct_stat->cs_present;
	/* dump_pci_device(PCI_DEV(0, 0x18+p_dct_stat->node_id, 2)); */

	printk(BIOS_DEBUG, "SPDSetBanks: cs_present %x\n", p_dct_stat->cs_present_dct[dct]);
	printk(BIOS_DEBUG, "SPDSetBanks: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "SPDSetBanks: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "SPDSetBanks: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "SPDSetBanks: Done\n\n");
}

static void spd_calc_width_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Per SPDs, check the symmetry of dimm pairs (dimm on channel A
	 *  matching with dimm on channel B), the overall dimm population,
	 * and determine the width mode: 64-bit, 64-bit muxed, 128-bit.
	 */
	u8 i;
	u8 byte, byte1;

	/* Check Symmetry of channel A and channel B DIMMs
	  (must be matched for 128-bit mode).*/
	for (i = 0; i < MAX_DIMMS_SUPPORTED; i += 2) {
		if ((p_dct_stat->dimm_valid & (1 << i))
		&& (p_dct_stat->dimm_valid & (1 << (i + 1)))) {
			byte = p_dct_stat->spd_data.spd_bytes[i][SPD_ADDRESSING] & 0x7;
			byte1 = p_dct_stat->spd_data.spd_bytes[i + 1][SPD_ADDRESSING] & 0x7;
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

			byte =	 p_dct_stat->spd_data.spd_bytes[i][SPD_DENSITY] & 0x0f;
			byte1 =	 p_dct_stat->spd_data.spd_bytes[i + 1][SPD_DENSITY] & 0x0f;
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

			byte = p_dct_stat->spd_data.spd_bytes[i][SPD_ORGANIZATION] & 0x7;
			byte1 = p_dct_stat->spd_data.spd_bytes[i + 1][SPD_ORGANIZATION] & 0x7;
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

			byte = (p_dct_stat->spd_data.spd_bytes[i][SPD_ORGANIZATION] >> 3) & 0x7;
			byte1 = (p_dct_stat->spd_data.spd_bytes[i + 1][SPD_ORGANIZATION] >> 3)
											& 0x7;
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

			/* #ranks-1 */
			byte = p_dct_stat->spd_data.spd_bytes[i][SPD_DMBANKS] & 7;
			/* #ranks-1 */
			byte1 = p_dct_stat->spd_data.spd_bytes[i + 1][SPD_DMBANKS] & 7;
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

		}
	}

}

static void stitch_memory_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Requires that Mask values for each bank be programmed first and that
	 * the chip-select population indicator is correctly set.
	 */
	u8 b = 0;
	u32 nxt_cs_base, cur_cs_base;
	u8 p, q;
	u32 biggest_bank;
	u8 _d_spare_en;

	u16 word;
	u32 dev;
	u32 reg;
	u32 val;

	dev = p_dct_stat->dev_dct;

	_d_spare_en = 0;

	/* CS Sparing 1 = enabled, 0 = disabled */
	if (mct_get_nv_bits(NV_CS_SPARE_CTL) & 1) {
		if (MCT_DIMM_SPARE_NO_WARM) {
			/* Do no warm-reset dimm spare */
			if (p_mct_stat->g_status & 1 << GSB_EN_DIMM_SPARE_NW) {
				word = p_dct_stat->cs_present;
				val = bsf(word);
				word &= ~(1 << val);
				if (word)
					/* Make sure at least two chip-selects are available */
					_d_spare_en = 1;
				else
					p_dct_stat->err_status |= 1 << SB_SPARE_DIS;
			}
		} else {
			/*DQS Training 1 = enabled, 0 = disabled */
			if (!mct_get_nv_bits(NV_DQS_TRAIN_CTL)) {
				word = p_dct_stat->cs_present;
				val = bsf(word);
				word &= ~(1 << val);
				if (word)
					/* Make sure at least two chip-selects are available */
					_d_spare_en = 1;
				else
					p_dct_stat->err_status |= 1 << SB_SPARE_DIS;
			}
		}
	}

	nxt_cs_base = 0;		/* Next available cs base ADDR[39:8] */
	for (p = 0; p < MAX_DIMMS_SUPPORTED; p++) {
		biggest_bank = 0;
		for (q = 0; q < MAX_CS_SUPPORTED; q++) { /* from DIMMS to CS */
			if (p_dct_stat->cs_present & (1 << q)) {  /* bank present? */
				reg  = 0x40 + (q << 2);  /* Base[q] reg.*/
				val = get_nb32_dct(dev, dct, reg);
				/* (CS_ENABLE|SPARE == 1)bank is enabled already? */
				if (!(val & 3)) {
					reg = 0x60 + (q << 1); /*Mask[q] reg.*/
					val = get_nb32_dct(dev, dct, reg);
					val >>= 19;
					val++;
					val <<= 19;
					if (val > biggest_bank) {
						/*Bingo! possibly Map this chip-select next! */
						biggest_bank = val;
						b = q;
					}
				}
			}	/*if bank present */
		}	/* while q */
		if (biggest_bank !=0) {
			cur_cs_base = nxt_cs_base;		/* cur_cs_base = nxt_cs_base*/
			/* DRAM CS Base b Address Register offset */
			reg = 0x40 + (b << 2);
			if (_d_spare_en) {
				biggest_bank = 0;
				val = 1 << SPARE;	/* spare Enable*/
			} else {
				val = cur_cs_base;
				val |= 1 << CS_ENABLE;	/* Bank Enable */
			}
			if (((reg - 0x40) >> 2) & 1) {
				if (!(p_dct_stat->status & (1 << SB_REGISTERED))) {
					u16  dimValid;
					dimValid = p_dct_stat->dimm_valid;
					if (dct & 1)
						dimValid <<= 1;
					if ((dimValid & p_dct_stat->mirr_pres_u_num_reg_r) != 0) {
						val |= 1 << ON_DIMM_MIRROR;
					}
				}
			}
			set_nb32_dct(dev, dct, reg, val);
			if (_d_spare_en)
				_d_spare_en = 0;
			else
				/* let nxt_cs_base+=Size[b] */
				nxt_cs_base += biggest_bank;
		}

		/* bank present but disabled?*/
		if (p_dct_stat->cs_test_fail & (1 << p)) {
			/* DRAM CS Base b Address Register offset */
			reg = (p << 2) + 0x40;
			val = 1 << TEST_FAIL;
			set_nb32_dct(dev, dct, reg, val);
		}
	}

	if (nxt_cs_base) {
		p_dct_stat->dct_sys_limit = nxt_cs_base - 1;
		mct_after_stitch_memory(p_mct_stat, p_dct_stat, dct);
	}

	/* dump_pci_device(PCI_DEV(0, 0x18+p_dct_stat->node_id, 2)); */

	printk(BIOS_DEBUG, "StitchMemory: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "StitchMemory: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "StitchMemory: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "StitchMemory: Done\n\n");
}

static u16 get_fk_d(u8 k)
{
	return table_f_k[k]; /* FIXME: k or k<<1 ? */
}

static u8 dimm_presence_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Check DIMMs present, verify checksum, flag SDRAM type,
	 * build population indicator bitmaps, and preload bus loading
	 * of DIMMs into DCTStatStruc.
	 * MAAload = number of devices on the "A" bus.
	 * MABload = number of devices on the "B" bus.
	 * maa_dimms = number of DIMMs on the "A" bus slots.
	 * MABdimms = number of DIMMs on the "B" bus slots.
	 * DATAAload = number of ranks on the "A" bus slots.
	 * DATABload = number of ranks on the "B" bus slots.
	 */
	u16 i, j, k;
	u8 smb_addr;
	u8 spd_ctrl;
	u16 reg_dimm_present, lr_dimm_present, max_dimms;
	u8 dev_width;
	u16 dimm_slots;
	u8 byte = 0, bytex;
	u8 crc_status;

	/* preload data structure with addrs */
	mct_get_dimm_addr(p_dct_stat, p_dct_stat->node_id);

	dimm_slots = max_dimms = mct_get_nv_bits(NV_MAX_DIMMS);

	spd_ctrl = mct_get_nv_bits(NV_SPDCHK_RESTRT);

	reg_dimm_present = 0;
	lr_dimm_present = 0;
	p_dct_stat->dimm_qr_present = 0;

	for (i = 0; i < MAX_DIMMS_SUPPORTED; i++) {
		if (i >= max_dimms)
			break;

		if ((p_dct_stat->dimm_qr_present & (1 << i)) || (i < dimm_slots)) {
			int status;
			smb_addr = get_dimm_address_d(p_dct_stat, i);
			status = mct_read_spd(smb_addr, SPD_BYTE_USE);
			if (status >= 0) {
				/* Verify result */
				status = mct_read_spd(smb_addr, SPD_BYTE_USE);
			}
			if (status >= 0) { /* SPD access is ok */
				p_dct_stat->dimm_present |= 1 << i;
				read_spd_bytes(p_mct_stat, p_dct_stat, i);
#ifdef DEBUG_DIMM_SPD
				dump_spd_bytes(p_mct_stat, p_dct_stat, i);
#endif
				crc_status = crc_check(p_dct_stat, i);
				if (!crc_status) {
					/* Try again in case there was a transient glitch */
					read_spd_bytes(p_mct_stat, p_dct_stat, i);
					crc_status = crc_check(p_dct_stat, i);
				}
				if ((crc_status) || (spd_ctrl == 2)) { /* CRC is OK */
					byte = p_dct_stat->spd_data.spd_bytes[i][SPD_TYPE];
					if (byte == JED_DDR3SDRAM) {
						/*Dimm is 'Present'*/
						p_dct_stat->dimm_valid |= 1 << i;
					}
				} else {
					printk(BIOS_WARNING,
						"node %d dimm %d: SPD checksum invalid\n",
						p_dct_stat->node_id, i);
					p_dct_stat->dimm_spd_checksum_err = 1 << i;
					if (spd_ctrl == 0) {
						p_dct_stat->err_status |= 1 << SB_DIMM_CHKSUM;
						p_dct_stat->err_code = SC_STOP_ERR;
					} else {
						/* if NV_SPDCHK_RESTRT is set to 1,
						 * ignore faulty SPD checksum
						 */
						p_dct_stat->err_status |= 1 << SB_DIMM_CHKSUM;
						byte = p_dct_stat->spd_data.spd_bytes[i]
										[SPD_TYPE];
						if (byte == JED_DDR3SDRAM)
							p_dct_stat->dimm_valid |= 1 << i;
					}
				}

				/* Zero dimm SPD data cache if dimm not present / valid */
				if (!(p_dct_stat->dimm_valid & (1 << i)))
					memset(p_dct_stat->spd_data.spd_bytes[i],
						0, sizeof(p_dct_stat->spd_data.spd_bytes[i]));

				/* Get module information for SMBIOS */
				if (p_dct_stat->dimm_valid & (1 << i)) {
					p_dct_stat->dimm_manufacturer_id[i] = 0;
					for (k = 0; k < 8; k++)
						p_dct_stat->dimm_manufacturer_id[i]
							|= ((u64)p_dct_stat->spd_data.spd_bytes
									[i][SPD_MANID_START + k]
									) << (k * 8);
					for (k = 0; k < SPD_PARTN_LENGTH; k++) {
						p_dct_stat->dimm_part_number[i][k]
							= p_dct_stat->spd_data.spd_bytes
								[i][SPD_PARTN_START + k];
					}
					p_dct_stat->dimm_part_number[i][SPD_PARTN_LENGTH] = 0;
					p_dct_stat->dimm_revision_number[i] = 0;
					for (k = 0; k < 2; k++)
						p_dct_stat->dimm_revision_number[i]
							|= ((u16)p_dct_stat->spd_data.spd_bytes
									[i][SPD_REVNO_START + k]
									) << (k * 8);
					p_dct_stat->dimm_serial_number[i] = 0;
					for (k = 0; k < 4; k++)
						p_dct_stat->dimm_serial_number[i]
							|= ((u32)p_dct_stat->spd_data.spd_bytes
								[i][SPD_SERIAL_START + k])
								<< (k * 8);
					p_dct_stat->dimm_rows[i]
						= (p_dct_stat->spd_data.spd_bytes
									[i][SPD_ADDRESSING]
								& 0x38) >> 3;
					p_dct_stat->dimm_cols[i]
						= p_dct_stat->spd_data.spd_bytes
									[i][SPD_ADDRESSING]
								& 0x7;
					p_dct_stat->dimm_ranks[i]
						= ((p_dct_stat->spd_data.spd_bytes
									[i][SPD_ORGANIZATION]
								& 0x38) >> 3) + 1;
					p_dct_stat->dimm_banks[i]
						= 1ULL << (((p_dct_stat->spd_data.spd_bytes
									[i][SPD_DENSITY]
								& 0x70) >> 4) + 3);
					p_dct_stat->dimm_width[i]
						= 1ULL << ((p_dct_stat->spd_data.spd_bytes
									[i][SPD_BUS_WIDTH]
								& 0x7) + 3);
					p_dct_stat->dimm_chip_size[i]
						= 1ULL << ((p_dct_stat->spd_data.spd_bytes
									[i][SPD_DENSITY]
								& 0xf) + 28);
					p_dct_stat->dimm_chip_width[i]
						= 1ULL << ((p_dct_stat->spd_data.spd_bytes
									[i][SPD_ORGANIZATION]
								& 0x7) + 2);
				}
				/* Check supported voltage(s) */
				p_dct_stat->dimm_supported_voltages[i]
					= p_dct_stat->spd_data.spd_bytes[i][SPD_VOLTAGE] & 0x7;
				/* Invert LSB to convert from SPD format to internal bitmap
				 * format
				 */
				p_dct_stat->dimm_supported_voltages[i] ^= 0x1;
				/* Check module type */
				byte = p_dct_stat->spd_data.spd_bytes[i][SPD_DIMMTYPE] & 0x7;
				if (byte == JED_RDIMM || byte == JED_MiniRDIMM) {
					reg_dimm_present |= 1 << i;
					p_dct_stat->dimm_registered[i] = 1;
				} else {
					p_dct_stat->dimm_registered[i] = 0;
				}
				if (byte == JED_LRDIMM) {
					lr_dimm_present |= 1 << i;
					p_dct_stat->dimm_load_reduced[i] = 1;
				} else {
					p_dct_stat->dimm_load_reduced[i] = 0;
				}
				/* Check ECC capable */
				byte = p_dct_stat->spd_data.spd_bytes[i][SPD_BUS_WIDTH];
				if (byte & JED_ECC) {
					/* dimm is ECC capable */
					p_dct_stat->dimm_ecc_present |= 1 << i;
				}
				/* Check if x4 device */
				dev_width = p_dct_stat->spd_data.spd_bytes[i][SPD_ORGANIZATION]
								& 0x7; /* 0:x4,1:x8,2:x16 */
				if (dev_width == 0) {
					/* dimm is made with x4 or x16 drams */
					p_dct_stat->dimm_x4_present |= 1 << i;
				} else if (dev_width == 1) {
					p_dct_stat->dimm_x8_present |= 1 << i;
				} else if (dev_width == 2) {
					p_dct_stat->dimm_x16_present |= 1 << i;
				}

				byte = (p_dct_stat->spd_data.spd_bytes[i][SPD_ORGANIZATION]
						>> 3);
				byte &= 7;
				if (byte == 3) { /* 4ranks */
					/* if any DIMMs are QR, we have to make two passes
					 * through DIMMs
					 */
					if (p_dct_stat->dimm_qr_present == 0) {
						max_dimms <<= 1;
					}
					if (i < dimm_slots) {
						p_dct_stat->dimm_qr_present |= (1 << i) | (1 << (i + 4));
					} else {
						p_dct_stat->ma_dimms[i & 1] --;
					}
					/* upper two ranks of QR dimm will be counted on another
					 * dimm number iteration
					 */
					byte = 1;
				} else if (byte == 1) { /* 2ranks */
					p_dct_stat->dimm_dr_present |= 1 << i;
				}
				bytex = dev_width;
				if (dev_width == 0)
					bytex = 16;
				else if (dev_width == 1)
					bytex = 8;
				else if (dev_width == 2)
					bytex = 4;

				byte++;		/* al+1 = rank# */
				if (byte == 2) {
					/* double Addr bus load value for dual rank DIMMs */
					bytex <<= 1;
				}

				j = i & (1 << 0);
				/*number of ranks on DATA bus*/
				p_dct_stat->data_load[j] += byte;
				/*number of devices on CMD/ADDR bus*/
				p_dct_stat->ma_load[j] += bytex;
				/*number of DIMMs on A bus */
				p_dct_stat->ma_dimms[j]++;

				/* check address mirror support for unbuffered dimm */
				/* check number of registers on a dimm for registered dimm */
				byte = p_dct_stat->spd_data.spd_bytes[i][SPD_ADDRESS_MIRROR];
				if (reg_dimm_present & (1 << i)) {
					if ((byte & 3) > 1)
						p_dct_stat->mirr_pres_u_num_reg_r |= 1 << i;
				} else {
					if ((byte & 1) == 1)
						p_dct_stat->mirr_pres_u_num_reg_r |= 1 << i;
				}
				/* Get byte62: Reference Raw Card information. We dont need it
				 * now.
				 */
				/* byte = p_dct_stat->spd_data.spd_bytes[i][SPD_REF_RAW_CARD];
				 */
				/* Get Byte65/66 for register manufacture ID code */
				if ((0x97
				== p_dct_stat->spd_data.spd_bytes[i][SPD_REG_MANUFACTURE_ID_H])
				&& (0x80
				== p_dct_stat->spd_data.spd_bytes[i][SPD_REG_MANUFACTURE_ID_L])) {
					if (0x16 == p_dct_stat->spd_data.spd_bytes[i]
									[SPD_REG_MAN_REV_ID])
						p_dct_stat->reg_man_2_present |= 1 << i;
					else
						p_dct_stat->reg_man_1_present |= 1 << i;
				}
				/* Get control word value for RC3 */
				byte = p_dct_stat->spd_data.spd_bytes[i][70];
				/* RC3 = SPD byte 70 [7:4] */
				p_dct_stat->ctrl_wrd_3 |= ((byte >> 4) & 0xf) << (i << 2);
				/* Get control word values for RC4 and RC5 */
				byte = p_dct_stat->spd_data.spd_bytes[i][71];
				/* RC4 = SPD byte 71 [3:0] */
				p_dct_stat->ctrl_wrd_4 |= (byte & 0xf) << (i << 2);
				/* RC5 = SPD byte 71 [7:4] */
				p_dct_stat->ctrl_wrd_5 |= ((byte >> 4) & 0xf) << (i << 2);
			}
		}
	}
	printk(BIOS_DEBUG,
		"\t DIMMPresence: dimm_valid=%x\n",
		p_dct_stat->dimm_valid);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: dimm_present=%x\n",
		p_dct_stat->dimm_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: reg_dimm_present=%x\n",
		reg_dimm_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: lr_dimm_present=%x\n",
		lr_dimm_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: dimm_ecc_present=%x\n",
		p_dct_stat->dimm_ecc_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: dimm_parity_present=%x\n",
		p_dct_stat->dimm_parity_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: dimm_x4_present=%x\n",
		p_dct_stat->dimm_x4_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: dimm_x8_present=%x\n",
		p_dct_stat->dimm_x8_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: dimm_x16_present=%x\n",
		p_dct_stat->dimm_x16_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: dimm_pl_present=%x\n",
		p_dct_stat->dimm_pl_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: DimmDRPresent=%x\n",
		p_dct_stat->dimm_dr_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: DimmQRPresent=%x\n",
		p_dct_stat->dimm_qr_present);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: data_load[0]=%x\n",
		p_dct_stat->data_load[0]);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: ma_load[0]=%x\n",
		p_dct_stat->ma_load[0]);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: ma_dimms[0]=%x\n",
		p_dct_stat->ma_dimms[0]);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: data_load[1]=%x\n",
		p_dct_stat->data_load[1]);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: ma_load[1]=%x\n",
		p_dct_stat->ma_load[1]);
	printk(BIOS_DEBUG,
		"\t DIMMPresence: ma_dimms[1]=%x\n",
		p_dct_stat->ma_dimms[1]);

	if (p_dct_stat->dimm_valid != 0) {	/* If any DIMMs are present...*/
		if (reg_dimm_present != 0) {
			if ((reg_dimm_present ^ p_dct_stat->dimm_valid) !=0) {
				/* module type dimm mismatch (reg'ed, unbuffered) */
				p_dct_stat->err_status |= 1 << SB_DIMM_MISMATCH_M;
				p_dct_stat->err_code = SC_STOP_ERR;
			} else{
				/* all DIMMs are registered */
				p_dct_stat->status |= 1 << SB_REGISTERED;
			}
		}
		if (lr_dimm_present != 0) {
			if ((lr_dimm_present ^ p_dct_stat->dimm_valid) !=0) {
				/* module type dimm mismatch (reg'ed, unbuffered) */
				p_dct_stat->err_status |= 1 << SB_DIMM_MISMATCH_M;
				p_dct_stat->err_code = SC_STOP_ERR;
			} else{
				/* all DIMMs are registered */
				p_dct_stat->status |= 1 << SB_LOAD_REDUCED;
			}
		}
		if (p_dct_stat->dimm_ecc_present != 0) {
			if ((p_dct_stat->dimm_ecc_present ^ p_dct_stat->dimm_valid) == 0) {
				/* all DIMMs are ECC capable */
				p_dct_stat->status |= 1 << SB_ECC_DIMMS;
			}
		}
		if (p_dct_stat->dimm_parity_present != 0) {
			if ((p_dct_stat->dimm_parity_present ^ p_dct_stat->dimm_valid) == 0) {
				/*all DIMMs are Parity capable */
				p_dct_stat->status |= 1 << SB_PAR_DIMMS;
			}
		}
	} else {
		/* no DIMMs present or no DIMMs that qualified. */
		p_dct_stat->err_status |= 1 << SB_NO_DIMMS;
		p_dct_stat->err_code = SC_STOP_ERR;
	}

	printk(BIOS_DEBUG, "\t DIMMPresence: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "\t DIMMPresence: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "\t DIMMPresence: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "\t DIMMPresence: Done\n\n");

	mct_hook_after_dimm_pre();

	return p_dct_stat->err_code;
}

static u8 get_dimm_address_d(struct DCTStatStruc *p_dct_stat, u8 i)
{
	u8 *p;

	p = p_dct_stat->dimm_addr;
	/* mct_BeforeGetDIMMAddress(); */
	return p[i];
}

static void mct_pre_init_dct(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u8 err_code;
	u8 allow_config_restore;

	/* Preconfigure DCT0 */
	dct_pre_init_d(p_mct_stat, p_dct_stat, 0);

	/* Configure DCT1 if unganged and enabled*/
	if (!p_dct_stat->ganged_mode) {
		if (p_dct_stat->dimm_valid_dct[1] > 0) {
			/* save DCT0 errors */
			err_code = p_dct_stat->err_code;
			p_dct_stat->err_code = 0;
			dct_pre_init_d(p_mct_stat, p_dct_stat, 1);
			/* DCT1 is not Running */
			if (p_dct_stat->err_code == 2) {
				/* Using DCT0 Error code to update p_dct_stat.err_code */
				p_dct_stat->err_code = err_code;
			}
		}
	}

#if CONFIG(HAVE_ACPI_RESUME)
	calculate_and_store_spd_hashes(p_mct_stat, p_dct_stat);

	if (load_spd_hashes_from_nvram(p_mct_stat, p_dct_stat) < 0) {
		p_dct_stat->spd_data.nvram_spd_match = 0;
	} else {
		compare_nvram_spd_hashes(p_mct_stat, p_dct_stat);
	}
#else
	p_dct_stat->spd_data.nvram_spd_match = 0;
#endif

	/* Check to see if restoration of SPD data from NVRAM is allowed */
	allow_config_restore = get_uint_option("allow_spd_nvram_cache_restore", 0);

#if CONFIG(HAVE_ACPI_RESUME)
	if (p_mct_stat->nvram_checksum != calculate_nvram_mct_hash())
		allow_config_restore = 0;
#else
	allow_config_restore = 0;
#endif

	if (!allow_config_restore)
		p_dct_stat->spd_data.nvram_spd_match = 0;
}

static void mct_init_dct(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 val;
	u8 err_code;

	/* Config. DCT0 for Ganged or unganged mode */
	dct_init_d(p_mct_stat, p_dct_stat, 0);
	dct_final_init_d(p_mct_stat, p_dct_stat, 0);
	if (p_dct_stat->err_code == SC_FATAL_ERR) {
		/* Do nothing goto exitDCTInit; any fatal errors? */
	} else {
		/* Configure DCT1 if unganged and enabled */
		if (!p_dct_stat->ganged_mode) {
			if (p_dct_stat->dimm_valid_dct[1] > 0) {
				/* save DCT0 errors */
				err_code = p_dct_stat->err_code;
				p_dct_stat->err_code = 0;
				dct_init_d(p_mct_stat, p_dct_stat, 1);
				dct_final_init_d(p_mct_stat, p_dct_stat, 1);
				/* DCT1 is not Running */
				if (p_dct_stat->err_code == 2) {
					/* Using DCT0 Error code to update p_dct_stat.err_code */
					p_dct_stat->err_code = err_code;
				}
			} else {
				val = 1 << DIS_DRAM_INTERFACE;
				set_nb32_dct(p_dct_stat->dev_dct, 1, 0x94, val);

				val = get_nb32_dct(p_dct_stat->dev_dct, 1, 0x90);
				val &= ~(1 << PAR_EN);
				set_nb32_dct(p_dct_stat->dev_dct, 1, 0x90, val);

				/* To maximize power savings when DIS_DRAM_INTERFACE = 1b,
				 * all of the MemClkDis bits should also be set.
				 */
				set_nb32_dct(p_dct_stat->dev_dct, 1, 0x88, 0xff000000);
			}
		}
	}
}

static void mct_dram_init(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	mct_before_dram_init__prod_d(p_mct_stat, p_dct_stat, dct);
	mct_dram_init_sw_d(p_mct_stat, p_dct_stat, dct);
	/* mct_dram_init_hw_d(p_mct_stat, p_dct_stat, dct); */
}

static u8 mct_set_mode(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u8 byte;
	u8 bytex;
	u32 val;
	u32 reg;

	byte = bytex = p_dct_stat->dimm_valid;
	bytex &= 0x55;		/* CHA dimm pop */
	p_dct_stat->dimm_valid_dct[0] = bytex;

	byte &= 0xAA;		/* CHB dimm popa */
	byte >>= 1;
	p_dct_stat->dimm_valid_dct[1] = byte;

	if (byte != bytex) {
		p_dct_stat->err_status &= ~(1 << SB_DIMM_MISMATCH_O);
	} else {
		byte = mct_get_nv_bits(NV_UNGANGED);
		if (byte) {
			/* Set temp. to avoid setting of ganged mode */
			p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
		}

		if ((!(p_dct_stat->err_status & (1 << SB_DIMM_MISMATCH_O)))
		&& (p_dct_stat->logical_cpuid & AMD_FAM10_ALL)) {
			/* Ganged channel mode not supported on Family 15h or higher */
			p_dct_stat->ganged_mode = 1;
			/* valid 128-bit mode population. */
			p_dct_stat->status |= 1 << SB_128_BIT_MODE;
			reg = 0x110;
			val = get_nb32(p_dct_stat->dev_dct, reg);
			val |= 1 << DCT_GANG_EN;
			set_nb32(p_dct_stat->dev_dct, reg, val);
		}
		/* NV_UNGANGED */
		if (byte) {
			/* Clear so that there is no dimm mismatch error */
			p_dct_stat->err_status &= ~(1 << SB_DIMM_MISMATCH_O);
		}
	}

	return p_dct_stat->err_code;
}

u32 get_nb32(u32 dev, u32 reg)
{
	return pci_read_config32(dev, reg);
}

void set_nb32(u32 dev, u32 reg, u32 val)
{
	pci_write_config32(dev, reg, val);
}


u32 get_nb32_index(u32 dev, u32 index_reg, u32 index)
{
	u32 dword;

	set_nb32(dev, index_reg, index);
	dword = get_nb32(dev, index_reg + 0x4);

	return dword;
}

void set_nb32_index(u32 dev, u32 index_reg, u32 index, u32 data)
{
	set_nb32(dev, index_reg, index);
	set_nb32(dev, index_reg + 0x4, data);
}

u32 get_nb32_index_wait(u32 dev, u32 index_reg, u32 index)
{
	u32 dword;

	index &= ~(1 << DCT_ACCESS_WRITE);
	set_nb32(dev, index_reg, index);
	do {
		dword = get_nb32(dev, index_reg);
	} while (!(dword & (1 << DCT_ACCESS_DONE)));
	dword = get_nb32(dev, index_reg + 0x4);

	return dword;
}

void set_nb32_index_wait(u32 dev, u32 index_reg, u32 index, u32 data)
{
	u32 dword;

	set_nb32(dev, index_reg + 0x4, data);
	index |= (1 << DCT_ACCESS_WRITE);
	set_nb32(dev, index_reg, index);
	do {
		dword = get_nb32(dev, index_reg);
	} while (!(dword & (1 << DCT_ACCESS_DONE)));

}

u8 mct_before_platform_spec(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	/* mct_checkForCxDxSupport_D */
	if (p_dct_stat->logical_cpuid & AMD_DR_GT_Bx) {
		/* Family 10h Errata 322:
		 * Address and Command Fine Delay Values May Be Incorrect
		 */
		/* 1. Write 00000000h to F2x[1,0]9C_xD08E000 */
		set_nb32_index_wait_dct(p_dct_stat->dev_dct, dct, 0x98, 0x0D08E000, 0);
		/* 2. If DRAM Configuration Register[mem_clk_freq] (F2x[1,0]94[2:0]) is
		   greater than or equal to 011b (DDR-800 and higher),
		   then write 00000080h to F2x[1,0]9C_xD02E001,
		   else write 00000090h to F2x[1,0]9C_xD02E001. */
		if (p_dct_stat->speed >= mhz_to_memclk_config(mct_get_nv_bits(NV_MIN_MEMCLK))) {
			set_nb32_index_wait_dct(
					p_dct_stat->dev_dct, dct, 0x98, 0x0D02E001, 0x80);
		}
		else {
			set_nb32_index_wait_dct(
					p_dct_stat->dev_dct, dct, 0x98, 0x0D02E001, 0x90);
		}
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);

	return p_dct_stat->err_code;
}

u8 mct_platform_spec(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Get platform specific config/timing values from the interface layer
	 * and program them into DCT.
	 */

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	u32 dev = p_dct_stat->dev_dct;
	u32 index_reg;
	u8 i, i_start, i_end;

	if (p_dct_stat->ganged_mode) {
		sync_settings(p_dct_stat);
		/* mct_setup_sync_d */
		i_start = 0;
		i_end = 2;
	} else {
		i_start = dct;
		i_end = dct + 1;
	}
	for (i = i_start; i < i_end; i++) {
		index_reg = 0x98;
		/* channel A/B Output Driver Compensation Control */
		set_nb32_index_wait_dct(dev, i, index_reg, 0x00, p_dct_stat->ch_odc_ctl[i]);
		/* channel A/B Output Driver Compensation Control */
		set_nb32_index_wait_dct(dev, i, index_reg, 0x04, p_dct_stat->ch_addr_tmg[i]);
		printk(BIOS_SPEW,
			"Programmed DCT %d timing/termination pattern %08x %08x\n",
			dct, p_dct_stat->ch_addr_tmg[i], p_dct_stat->ch_odc_ctl[i]);
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);

	return p_dct_stat->err_code;
}

static void mct_sync_dcts_ready(struct DCTStatStruc *p_dct_stat)
{
	u32 dev;
	u32 val;

	if (p_dct_stat->node_present) {
		dev = p_dct_stat->dev_dct;

		if ((p_dct_stat->dimm_valid_dct[0]) || (p_dct_stat->dimm_valid_dct[1])) {
			/* This node has DRAM */
			do {
				val = get_nb32(dev, 0x110);
			} while (!(val & (1 << DRAM_ENABLED)));
		}
	}	/* node is present */
}

static void mct_after_get_clt(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	if (!p_dct_stat->ganged_mode) {
		if (dct == 0) {
			p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[dct];
			if (p_dct_stat->dimm_valid_dct[dct] == 0)
				p_dct_stat->err_code = SC_STOP_ERR;
		} else {
			p_dct_stat->cs_present = 0;
			p_dct_stat->cs_test_fail = 0;
			p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[dct];
			if (p_dct_stat->dimm_valid_dct[dct] == 0)
				p_dct_stat->err_code = SC_STOP_ERR;
		}
	}
}

static u8 mct_spd_calc_width(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 ret;
	u32 val;

	if (dct == 0) {
		spd_calc_width_d(p_mct_stat, p_dct_stat);
		ret = mct_set_mode(p_mct_stat, p_dct_stat);
	} else {
		ret = p_dct_stat->err_code;
	}

	if (p_dct_stat->dimm_valid_dct[0] == 0) {
		val = get_nb32_dct(p_dct_stat->dev_dct, 0, 0x94);
		val |= 1 << DIS_DRAM_INTERFACE;
		set_nb32_dct(p_dct_stat->dev_dct, 0, 0x94, val);

		val = get_nb32_dct(p_dct_stat->dev_dct, 0, 0x90);
		val &= ~(1 << PAR_EN);
		set_nb32_dct(p_dct_stat->dev_dct, 0, 0x90, val);
	}
	if (p_dct_stat->dimm_valid_dct[1] == 0) {
		val = get_nb32_dct(p_dct_stat->dev_dct, 1, 0x94);
		val |= 1 << DIS_DRAM_INTERFACE;
		set_nb32_dct(p_dct_stat->dev_dct, 1, 0x94, val);

		val = get_nb32_dct(p_dct_stat->dev_dct, 1, 0x90);
		val &= ~(1 << PAR_EN);
		set_nb32_dct(p_dct_stat->dev_dct, 1, 0x90, val);
	}

	printk(BIOS_DEBUG, "SPDCalcWidth: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "SPDCalcWidth: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "SPDCalcWidth: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "SPDCalcWidth: Done\n");
	/* Disable dram interface before DRAM init */

	return ret;
}

static void mct_after_stitch_memory(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 dword;
	u32 dev;
	u32 reg;
	u8 _mem_hole_remap;
	u32 dram_hole_base;

	_mem_hole_remap = mct_get_nv_bits(NV_MEM_HOLE);
	dram_hole_base = mct_get_nv_bits(NV_BOTTOM_IO);
	dram_hole_base <<= 8;
	/* Increase hole size so;[31:24]to[31:16]
	 * it has granularity of 128MB shl eax,8
	 * Set 'effective' bottom IOmov dram_hole_base,eax
	 */
	p_mct_stat->hole_base = (dram_hole_base & 0xFFFFF800) << 8;

	/* In unganged mode, we must add DCT0 and DCT1 to dct_sys_limit */
	if (!p_dct_stat->ganged_mode) {
		dev = p_dct_stat->dev_dct;
		p_dct_stat->node_sys_limit += p_dct_stat->dct_sys_limit;
		/* if DCT0 and DCT1 both exist, set DctSelBaseAddr[47:27] to the top of DCT0 */
		if (dct == 0) {
			if (p_dct_stat->dimm_valid_dct[1] > 0) {
				dword = p_dct_stat->dct_sys_limit + 1;
				dword += p_dct_stat->node_sys_base;
				dword >>= 8; /* scale [39:8] to [47:27],and to F2x110[31:11] */
				if ((dword >= dram_hole_base) && _mem_hole_remap) {
					p_mct_stat->hole_base
							= (dram_hole_base & 0xFFFFF800) << 8;
					val = p_mct_stat->hole_base;
					val >>= 16;
					val = (((~val) & 0xFF) + 1);
					val <<= 8;
					dword += val;
				}
				reg = 0x110;
				val = get_nb32(dev, reg);
				val &= 0x7F;
				val |= dword;
				val |= 3;  /* Set F2x110[DctSelHiRngEn], F2x110[DctSelHi] */
				set_nb32(dev, reg, val);

				reg = 0x114;
				val = dword;
				set_nb32(dev, reg, val);
			}
		} else {
			/* Program the DctSelBaseAddr value to 0
			   if DCT 0 is disabled */
			if (p_dct_stat->dimm_valid_dct[0] == 0) {
				dword = p_dct_stat->node_sys_base;
				dword >>= 8;
				if ((dword >= dram_hole_base) && _mem_hole_remap) {
					p_mct_stat->hole_base
							= (dram_hole_base & 0xFFFFF800) << 8;
					val = p_mct_stat->hole_base;
					val >>= 8;
					val &= ~(0xFFFF);
					val |= (((~val) & 0xFFFF) + 1);
					dword += val;
				}
				reg = 0x114;
				val = dword;
				set_nb32(dev, reg, val);

				reg = 0x110;
				/* Set F2x110[DctSelHiRngEn], F2x110[DctSelHi] */
				val |= 3;
				set_nb32(dev, reg, val);
			}
		}
	} else {
		p_dct_stat->node_sys_limit += p_dct_stat->dct_sys_limit;
	}
	printk(BIOS_DEBUG,
		"AfterStitch p_dct_stat->node_sys_base = %x\n",
		p_dct_stat->node_sys_base);
	printk(BIOS_DEBUG,
		"mct_after_stitch_memory: p_dct_stat->node_sys_limit = %x\n",
		p_dct_stat->node_sys_limit);
}

static u8 mct_dimm_presence(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 ret;

	if (dct == 0)
		ret = dimm_presence_d(p_mct_stat, p_dct_stat);
	else
		ret = p_dct_stat->err_code;

	return ret;
}

/* mct_BeforeGetDIMMAddress inline in C */

static void mct_other_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		if (p_dct_stat->node_present) {
			if (p_dct_stat->dimm_valid_dct[0]) {
				p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[0];
				set_other_timing(p_mct_stat, p_dct_stat, 0);
			}
			if (p_dct_stat->dimm_valid_dct[1] && !p_dct_stat->ganged_mode) {
				p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[1];
				set_other_timing(p_mct_stat, p_dct_stat, 1);
			}
		}	/* node is present*/
	}	/* while node */
}

static void set_other_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 reg;
	u32 val;
	u32 dword;
	u32 dev = p_dct_stat->dev_dct;

	get_dqs_rcv_en_gross_diff(p_dct_stat, dev, dct, 0x98);
	get_wr_dat_gross_diff(p_dct_stat, dct, dev, 0x98);
	get_trdrd(p_mct_stat, p_dct_stat, dct);
	get_twrwr(p_mct_stat, p_dct_stat, dct);
	get_twrrd(p_mct_stat, p_dct_stat, dct);
	get_trwt_to(p_mct_stat, p_dct_stat, dct);
	get_trwt_wb(p_mct_stat, p_dct_stat);

	if (!is_fam15h()) {
		reg = 0x8c;		/* Dram Timing Hi */
		val = get_nb32_dct(dev, dct, reg);
		val &= 0xffff0300;
		dword = p_dct_stat->trwt_to;
		val |= dword << 4;
		dword = p_dct_stat->twrrd & 3;
		val |= dword << 10;
		dword = p_dct_stat->twrwr & 3;
		val |= dword << 12;
		dword = (p_dct_stat->trdrd - 0x3) & 3;
		val |= dword << 14;
		dword = p_dct_stat->trwt_wb;
		val |= dword;
		set_nb32_dct(dev, dct, reg, val);

		reg = 0x78;
		val = get_nb32_dct(dev, dct, reg);
		val &= 0xffffc0ff;
		dword = p_dct_stat->twrrd >> 2;
		val |= dword << 8;
		dword = p_dct_stat->twrwr >> 2;
		val |= dword << 10;
		dword = (p_dct_stat->trdrd - 0x3) >> 2;
		val |= dword << 12;
		set_nb32_dct(dev, dct, reg, val);
	}
}

static void get_trdrd(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	int8_t Trdrd;

	Trdrd = ((int8_t)(p_dct_stat->dqs_rcv_en_gross_max
			- p_dct_stat->dqs_rcv_en_gross_min) >> 1) + 1;
	if (Trdrd > 8)
		Trdrd = 8;
	if (Trdrd < 3)
		Trdrd = 3;
	p_dct_stat->trdrd = Trdrd;
}

static void get_twrwr(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	int8_t Twrwr = 0;

	Twrwr = ((int8_t)(p_dct_stat->wr_dat_gross_max
			- p_dct_stat->wr_dat_gross_min) >> 1) + 2;

	if (Twrwr < 2)
		Twrwr = 2;
	else if (Twrwr > 9)
		Twrwr = 9;

	p_dct_stat->twrwr = Twrwr;
}

static void get_twrrd(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 LDplus1;
	int8_t Twrrd;

	LDplus1 = get_latency_diff(p_mct_stat, p_dct_stat, dct);

	Twrrd = ((int8_t)(p_dct_stat->wr_dat_gross_max
			- p_dct_stat->dqs_rcv_en_gross_min) >> 1) + 4 - LDplus1;

	if (Twrrd < 2)
		Twrrd = 2;
	else if (Twrrd > 10)
		Twrrd = 10;
	p_dct_stat->twrrd = Twrrd;
}

static void get_trwt_to(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 LDplus1;
	int8_t TrwtTO;

	LDplus1 = get_latency_diff(p_mct_stat, p_dct_stat, dct);

	TrwtTO = ((int8_t)(p_dct_stat->dqs_rcv_en_gross_max
			- p_dct_stat->wr_dat_gross_min) >> 1) + LDplus1;

	p_dct_stat->trwt_to = TrwtTO;
}

static void get_trwt_wb(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat)
{
	/* trwt_wb ensures read-to-write data-bus turnaround.
	   This value should be one more than the programmed TrwtTO.*/
	p_dct_stat->trwt_wb = p_dct_stat->trwt_to;
}

static u8 get_latency_diff(struct MCTStatStruc *p_mct_stat,
			   struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 val1, val2;

	val1 = get_nb32_dct(dev, dct, 0x88) & 0xF;
	val2 = (get_nb32_dct(dev, dct, 0x84) >> 20) & 7;

	return val1 - val2;
}

static void get_dqs_rcv_en_gross_diff(struct DCTStatStruc *p_dct_stat,
					u32 dev, u8 dct, u32 index_reg)
{
	u8 smallest, largest;
	u32 val;
	u8 byte, bytex;

	/* The largest DqsRcvEnGrossDelay of any dimm minus the
	   DqsRcvEnGrossDelay of any other dimm is equal to the Critical
	   Gross Delay Difference (CGDD) */
	/* DqsRcvEn byte 1,0 */
	val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, dct, index_reg, 0x10);
	largest = val & 0xFF;
	smallest = (val >> 8) & 0xFF;

	/* DqsRcvEn byte 3,2 */
	val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, dct, index_reg, 0x11);
	byte = val & 0xFF;
	bytex = (val >> 8) & 0xFF;
	if (bytex < smallest)
		smallest = bytex;
	if (byte > largest)
		largest = byte;

	/* DqsRcvEn byte 5,4 */
	val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, dct, index_reg, 0x20);
	byte = val & 0xFF;
	bytex = (val >> 8) & 0xFF;
	if (bytex < smallest)
		smallest = bytex;
	if (byte > largest)
		largest = byte;

	/* DqsRcvEn byte 7,6 */
	val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, dct, index_reg, 0x21);
	byte = val & 0xFF;
	bytex = (val >> 8) & 0xFF;
	if (bytex < smallest)
		smallest = bytex;
	if (byte > largest)
		largest = byte;

	if (p_dct_stat->dimm_ecc_present > 0) {
		/*DqsRcvEn Ecc */
		val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, dct, index_reg, 0x12);
		byte = val & 0xFF;
		bytex = (val >> 8) & 0xFF;
		if (bytex < smallest)
			smallest = bytex;
		if (byte > largest)
			largest = byte;
	}

	p_dct_stat->dqs_rcv_en_gross_max = largest;
	p_dct_stat->dqs_rcv_en_gross_min = smallest;
}

static void get_wr_dat_gross_diff(struct DCTStatStruc *p_dct_stat,
					u8 dct, u32 dev, u32 index_reg)
{
	u8 smallest = 0, largest = 0;
	u32 val;
	u8 byte, bytex;

	/* The largest WrDatGrossDlyByte of any dimm minus the
	  WrDatGrossDlyByte of any other dimm is equal to CGDD */
	if (p_dct_stat->dimm_valid & (1 << 0)) {
		/* WrDatGrossDlyByte byte 0,1,2,3 for DIMM0 */
		val = get_wr_dat_gross_max_min(p_dct_stat, dct, dev, index_reg, 0x01);
		largest = val & 0xFF;
		smallest = (val >> 8) & 0xFF;
	}
	if (p_dct_stat->dimm_valid & (1 << 2)) {
		/* WrDatGrossDlyByte byte 0,1,2,3 for DIMM1 */
		val = get_wr_dat_gross_max_min(p_dct_stat, dct, dev, index_reg, 0x101);
		byte = val & 0xFF;
		bytex = (val >> 8) & 0xFF;
		if (bytex < smallest)
			smallest = bytex;
		if (byte > largest)
			largest = byte;
	}

	/* If Cx, 2 more dimm need to be checked to find out the largest and smallest */
	if (p_dct_stat->logical_cpuid & AMD_DR_Cx) {
		if (p_dct_stat->dimm_valid & (1 << 4)) {
			/* WrDatGrossDlyByte byte 0,1,2,3 for DIMM2 */
			val = get_wr_dat_gross_max_min(p_dct_stat, dct, dev, index_reg, 0x201);
			byte = val & 0xFF;
			bytex = (val >> 8) & 0xFF;
			if (bytex < smallest)
				smallest = bytex;
			if (byte > largest)
				largest = byte;
		}
		if (p_dct_stat->dimm_valid & (1 << 6)) {
			/* WrDatGrossDlyByte byte 0,1,2,3 for DIMM2 */
			val = get_wr_dat_gross_max_min(p_dct_stat, dct, dev, index_reg, 0x301);
			byte = val & 0xFF;
			bytex = (val >> 8) & 0xFF;
			if (bytex < smallest)
				smallest = bytex;
			if (byte > largest)
				largest = byte;
		}
	}

	p_dct_stat->wr_dat_gross_max = largest;
	p_dct_stat->wr_dat_gross_min = smallest;
}

static u16 get_dqs_rcv_en_gross_max_min(struct DCTStatStruc *p_dct_stat,
					u32 dev, u8 dct, u32 index_reg,
					u32 index)
{
	u8 smallest, largest;
	u8 i;
	u8 byte;
	u32 val;
	u16 word;
	u8 ecc_reg = 0;

	smallest = 7;
	largest = 0;

	if (index == 0x12)
		ecc_reg = 1;

	for (i = 0; i < 8; i+=2) {
		if (p_dct_stat->dimm_valid & (1 << i)) {
			val = get_nb32_index_wait_dct(dev, dct, index_reg, index);
			val &= 0x00E000E0;
			byte = (val >> 5) & 0xFF;
			if (byte < smallest)
				smallest = byte;
			if (byte > largest)
				largest = byte;
			if (!(ecc_reg)) {
				byte = (val >> (16 + 5)) & 0xFF;
				if (byte < smallest)
					smallest = byte;
				if (byte > largest)
					largest = byte;
			}
		}
		index += 3;
	}	/* while ++i */

	word = smallest;
	word <<= 8;
	word |= largest;

	return word;
}

static u16 get_wr_dat_gross_max_min(struct DCTStatStruc *p_dct_stat,
					u8 dct, u32 dev, u32 index_reg,
					u32 index)
{
	u8 smallest, largest;
	u8 i, j;
	u32 val;
	u8 byte;
	u16 word;

	smallest = 3;
	largest = 0;
	for (i = 0; i < 2; i++) {
		val = get_nb32_index_wait_dct(dev, dct, index_reg, index);
		val &= 0x60606060;
		val >>= 5;
		for (j = 0; j < 4; j++) {
			byte = val & 0xFF;
			if (byte < smallest)
				smallest = byte;
			if (byte > largest)
				largest = byte;
			val >>= 8;
		}	/* while ++j */
		index++;
	}	/*while ++i*/

	if (p_dct_stat->dimm_ecc_present > 0) {
		index++;
		val = get_nb32_index_wait_dct(dev, dct, index_reg, index);
		val &= 0x00000060;
		val >>= 5;
		byte = val & 0xFF;
		if (byte < smallest)
			smallest = byte;
		if (byte > largest)
			largest = byte;
	}

	word = smallest;
	word <<= 8;
	word |= largest;

	return word;
}

static void mct_PhyController_Config(struct MCTStatStruc *p_mct_stat,
				     struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 index;
	u32 dword;
	u32 index_reg = 0x98;
	u32 dev = p_dct_stat->dev_dct;

	if (p_dct_stat->logical_cpuid & (AMD_DR_DAC2_OR_C3 | AMD_RB_C3 | AMD_FAM15_ALL)) {
		if (is_fam15h()) {
			/* Set F2x[1, 0]98_x0D0F0F13 DllDisEarlyU and DllDisEarlyL
			 * to save power
			 */
			for (index = 0; index < 0x9; index++) {
				dword = get_nb32_index_wait_dct(dev, dct, index_reg,
								0x0d0f0013 | (index << 8));
				/* DllDisEarlyU = 1 */
				dword |= (0x1 << 1);
				/* DllDisEarlyL = 1 */
				dword |= 0x1;
				set_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f0013 | (index << 8), dword);
			}
		}

		if (p_dct_stat->dimm_x4_present == 0) {
			/* Set bit7 RxDqsUDllPowerDown to register F2x[1, 0]98_x0D0F0F13 for
			 * additional power saving when x4 DIMMs are not present.
			 */
			for (index = 0; index < 0x9; index++) {
				dword = get_nb32_index_wait_dct(dev, dct, index_reg,
								0x0d0f0013 | (index << 8));
				/* RxDqsUDllPowerDown = 1 */
				dword |= (0x1 << 7);
				set_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f0013 | (index << 8), dword);
			}
		}
	}

	if (p_dct_stat->logical_cpuid & (AMD_DR_DAC2_OR_C3 | AMD_FAM15_ALL)) {
		if (p_dct_stat->dimm_ecc_present == 0) {
			/* Set bit4 PwrDn to register F2x[1, 0]98_x0D0F0830 for power saving */
			dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f0830);
			/* BIOS should set this bit if ECC DIMMs are not present */
			dword |= 1 << 4;
			set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f0830, dword);
		}
	}

}

static void mct_final_mct_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	struct DCTStatStruc *p_dct_stat;
	u32 val;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		p_dct_stat = p_dct_stat_a + node;

		if (p_dct_stat->node_present) {
			mct_PhyController_Config(p_mct_stat, p_dct_stat, 0);
			mct_PhyController_Config(p_mct_stat, p_dct_stat, 1);

			if (!is_fam15h()) {
				/* Family 10h CPUs */
				mct_ext_mct_config_cx(p_dct_stat);
				mct_ext_mct_config_bx(p_dct_stat);
				mct_ext_mct_config_dx(p_dct_stat);
			} else {
				/* Family 15h CPUs */
				u8 enable_experimental_memory_speed_boost;

				/* Check to see if cache partitioning is allowed */
				enable_experimental_memory_speed_boost =
					get_uint_option("experimental_memory_speed_boost", 0);

				val = 0x0ce00f00;		/* FLUSH_WR_ON_STP_GNT = 0x0 */
				val |= 0x10 << 2;		/* MctWrLimit = 0x10 */
				val |= 0x1;			/* DctWrLimit = 0x1 */
				set_nb32(p_dct_stat->dev_dct, 0x11c, val);

				val = get_nb32(p_dct_stat->dev_dct, 0x1b0);
				val &= ~0x3;			/* AdapPrefMissRatio = 0x1 */
				val |= 0x1;
				val &= ~(0x3 << 2);		/* AdapPrefPositiveStep = 0x0 */
				val &= ~(0x3 << 4);		/* AdapPrefNegativeStep = 0x0 */
				val &= ~(0x7 << 8);		/* CohPrefPrbLmt = 0x1 */
				val |= (0x1 << 8);
				val |= (0x1 << 12);		/* EnSplitDctLimits = 0x1 */
				if (enable_experimental_memory_speed_boost)
					val |= (0x1 << 20);	/* DblPrefEn = 0x1 */
				val |= (0x7 << 22);		/* PrefFourConf = 0x7 */
				val |= (0x7 << 25);		/* PrefFiveConf = 0x7 */
				val &= ~(0xf << 28);		/* DcqBwThrotWm = 0x0 */
				set_nb32(p_dct_stat->dev_dct, 0x1b0, val);

				u8 wm1;
				u8 wm2;

				switch (p_dct_stat->speed) {
				case 0x4:
					wm1 = 0x3;
					wm2 = 0x4;
					break;
				case 0x6:
					wm1 = 0x3;
					wm2 = 0x5;
					break;
				case 0xa:
					wm1 = 0x4;
					wm2 = 0x6;
					break;
				case 0xe:
					wm1 = 0x5;
					wm2 = 0x8;
					break;
				case 0x12:
					wm1 = 0x6;
					wm2 = 0x9;
					break;
				default:
					wm1 = 0x7;
					wm2 = 0xa;
					break;
				}

				val = get_nb32(p_dct_stat->dev_dct, 0x1b4);
				val &= ~(0x3ff);
				val |= ((wm2 & 0x1f) << 5);
				val |= (wm1 & 0x1f);
				set_nb32(p_dct_stat->dev_dct, 0x1b4, val);
			}
		}
	}

	/* ClrClToNB_D postponed until we're done executing from ROM */
	mct_clr_wb_enh_wsb_dis_d(p_mct_stat, p_dct_stat);

	/* set F3x8C[DisFastTprWr] on all DR, if L3Size = 0 */
	if (p_dct_stat->logical_cpuid & AMD_DR_ALL) {
		if (!(cpuid_edx(0x80000006) & 0xFFFC0000)) {
			val = get_nb32(p_dct_stat->dev_nbmisc, 0x8C);
			val |= 1 << 24;
			set_nb32(p_dct_stat->dev_nbmisc, 0x8C, val);
		}
	}
}

void mct_force_nb_pstate_0_en_fam15(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Force the NB P-state to P0 */
	u32 dword;
	u32 dword2;

	dword = get_nb32(p_dct_stat->dev_nbctl, 0x174);
	if (!(dword & 0x1)) {
		dword = get_nb32(p_dct_stat->dev_nbctl, 0x170);
		p_dct_stat->sw_nb_pstate_lo_dis = (dword >> 14) & 0x1;
		p_dct_stat->nb_pstate_dis_on_p0 = (dword >> 13) & 0x1;
		p_dct_stat->nb_pstate_threshold = (dword >> 9) & 0x7;
		p_dct_stat->nb_pstate_hi = (dword >> 6) & 0x3;
		dword &= ~(0x1 << 14);		/* sw_nb_pstate_lo_dis = 0 */
		dword &= ~(0x1 << 13);		/* nb_pstate_dis_on_p0 = 0 */
		dword &= ~(0x7 << 9);		/* nb_pstate_threshold = 0 */
		dword &= ~(0x3 << 3);		/* nb_pstate_lo = nb_pstate_max_val */
		dword |= ((dword & 0x3) << 3);
		set_nb32(p_dct_stat->dev_nbctl, 0x170, dword);

		if (!is_model10_1f()) {
			/* Wait until cur_nb_pstate == nb_pstate_lo */
			do {
				dword2 = get_nb32(p_dct_stat->dev_nbctl, 0x174);
			} while (((dword2 >> 19) & 0x7) != (dword & 0x3));
		}
		dword = get_nb32(p_dct_stat->dev_nbctl, 0x170);
		dword &= ~(0x3 << 6);		/* nb_pstate_hi = 0 */
		dword |= (0x3 << 14);		/* sw_nb_pstate_lo_dis = 1 */
		set_nb32(p_dct_stat->dev_nbctl, 0x170, dword);

		if (!is_model10_1f()) {
			/* Wait until cur_nb_pstate == 0 */
			do {
				dword2 = get_nb32(p_dct_stat->dev_nbctl, 0x174);
			} while (((dword2 >> 19) & 0x7) != 0);
		}
	}
}

void mct_force_nb_pstate_0_dis_fam15(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Restore normal NB P-state functionailty */
	u32 dword;

	dword = get_nb32(p_dct_stat->dev_nbctl, 0x174);
	if (!(dword & 0x1)) {
		dword = get_nb32(p_dct_stat->dev_nbctl, 0x170);
		/* sw_nb_pstate_lo_dis*/
		dword &= ~(0x1 << 14);
		dword |= ((p_dct_stat->sw_nb_pstate_lo_dis & 0x1) << 14);
		/* nb_pstate_dis_on_p0 */
		dword &= ~(0x1 << 13);
		dword |= ((p_dct_stat->nb_pstate_dis_on_p0 & 0x1) << 13);
		/* nb_pstate_threshold */
		dword &= ~(0x7 << 9);
		dword |= ((p_dct_stat->nb_pstate_threshold & 0x7) << 9);
		/* nb_pstate_hi */
		dword &= ~(0x3 << 6);
		dword |= ((p_dct_stat->nb_pstate_hi & 0x3) << 3);
		set_nb32(p_dct_stat->dev_nbctl, 0x170, dword);
	}
}

static void mct_initial_mct_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat)
{
	if (is_fam15h()) {
		msr_t p0_state_msr;
		u8 cpu_fid;
		u8 cpu_did;
		u32 cpu_divisor;
		u8 boost_states;

		/* Retrieve the number of boost states */
		boost_states = (get_nb32(p_dct_stat->dev_link, 0x15c) >> 2) & 0x7;

		/* Retrieve and store the TSC frequency (P0 COF) */
		p0_state_msr = rdmsr(PSTATE_0_MSR + boost_states);
		cpu_fid = p0_state_msr.lo & 0x3f;
		cpu_did = (p0_state_msr.lo >> 6) & 0x7;
		cpu_divisor = (0x1 << cpu_did);
		p_mct_stat->tsc_freq = (100 * (cpu_fid + 0x10)) / cpu_divisor;

		printk(BIOS_DEBUG, "mct_initial_mct_d: mct_force_nb_pstate_0_en_fam15\n");
		mct_force_nb_pstate_0_en_fam15(p_mct_stat, p_dct_stat);
	} else {
		/* K10 BKDG v3.62 section 2.8.9.2 */
		printk(BIOS_DEBUG, "mct_initial_mct_d: clear_legacy_mode\n");
		clear_legacy_mode(p_mct_stat, p_dct_stat);

		/* Northbridge configuration */
		mct_set_cl_to_nb_d(p_mct_stat, p_dct_stat);
		mct_set_wb_enh_wsb_dis_d(p_mct_stat, p_dct_stat);
	}
}

static u32 mct_node_present_d(void)
{
	u32 val;
	if (is_fam15h()) {
		if (is_model10_1f()) {
			val = 0x14001022;
		} else {
			val = 0x16001022;
		}
	} else {
		val = 0x12001022;
	}
	return val;
}

static void mct_init(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat)
{
	u32 lo, hi;
	u32 addr;

	p_dct_stat->ganged_mode = 0;
	p_dct_stat->dr_present = 1;

	/* enable extend PCI configuration access */
	addr = NB_CFG_MSR;
	_rdmsr(addr, &lo, &hi);
	if (hi & (1 << (46-32))) {
		p_dct_stat->status |= 1 << SB_EXT_CONFIG;
	} else {
		hi |= 1 << (46-32);
		_wrmsr(addr, lo, hi);
	}
}

static void clear_legacy_mode(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 reg;
	u32 val;
	u32 dev = p_dct_stat->dev_dct;

	/* Clear Legacy BIOS Mode bit */
	reg = 0x94;
	val = get_nb32_dct(dev, 0, reg);
	val &= ~(1 << LEGACY_BIOS_MODE);
	set_nb32_dct(dev, 0, reg, val);

	val = get_nb32_dct(dev, 1, reg);
	val &= ~(1 << LEGACY_BIOS_MODE);
	set_nb32_dct(dev, 1, reg, val);
}

static void mct_ht_mem_map_ext(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	u32 dram_base, dram_limit;
	u32 val;
	u32 reg;
	u32 dev;
	u32 devx;
	u32 dword;
	struct DCTStatStruc *p_dct_stat;

	p_dct_stat = p_dct_stat_a + 0;
	dev = p_dct_stat->dev_map;

	/* Copy dram map from F1x40/44,F1x48/4c,
	  to F1x120/124(Node0),F1x120/124(Node1),...*/
	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		p_dct_stat = p_dct_stat_a + node;
		devx = p_dct_stat->dev_map;

		/* get base/limit from Node0 */
		reg = 0x40 + (node << 3);		/* Node0/Dram Base 0 */
		val = get_nb32(dev, reg);
		dram_base = val >> (16 + 3);

		reg = 0x44 + (node << 3);		/* Node0/Dram Base 0 */
		val = get_nb32(dev, reg);
		dram_limit = val >> (16 + 3);

		/* set base/limit to F1x120/124 per node */
		if (p_dct_stat->node_present) {
			reg = 0x120;		/* F1x120,DramBase[47:27] */
			val = get_nb32(devx, reg);
			val &= 0xFFE00000;
			val |= dram_base;
			set_nb32(devx, reg, val);

			reg = 0x124;
			val = get_nb32(devx, reg);
			val &= 0xFFE00000;
			val |= dram_limit;
			set_nb32(devx, reg, val);

			if (p_mct_stat->g_status & (1 << GSB_HW_HOLE)) {
				reg = 0xF0;
				val = get_nb32(devx, reg);
				val |= (1 << DRAM_MEM_HOIST_VALID);
				val &= ~(0xFF << 24);
				dword = (p_mct_stat->hole_base >> (24 - 8)) & 0xFF;
				dword <<= 24;
				val |= dword;
				set_nb32(devx, reg, val);
			}

		}
	}
}

static void set_cs_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 dev = p_dct_stat->dev_dct;
	u32 index_reg = 0x98;
	u16 word;

	if (is_fam15h()) {
		word = fam15h_cs_tristate_enable_code(p_dct_stat, dct);
	} else {
		/* Tri-state unused chipselects when motherboard
		termination is available */

		/* FIXME: skip for Ax */

		word = p_dct_stat->cs_present;
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			word |= (word & 0x55) << 1;
		}
		word = (~word) & 0xff;
	}

	val = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c);
	val &= ~0xff;
	val |= word;
	set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c, val);
}

static void set_cke_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 dev;
	u32 index_reg = 0x98;
	u16 word;

	/* Tri-state unused CKEs when motherboard termination is available */

	/* FIXME: skip for Ax */

	dev = p_dct_stat->dev_dct;
	word = p_dct_stat->cs_present;

	val = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c);
	val &= ~(0x3 << 12);
	if ((word & 0x55) == 0)
		val |= 1 << 12;
	if ((word & 0xaa) == 0)
		val |= 1 << 13;
	set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c, val);
}

static void set_odt_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 dev;
	u32 index_reg = 0x98;
	u8 cs;
	u8 odt;

	dev = p_dct_stat->dev_dct;

	if (is_fam15h()) {
		odt = fam15h_odt_tristate_enable_code(p_dct_stat, dct);
	} else {
		/* FIXME: skip for Ax */

		/* Tri-state unused ODTs when motherboard termination is available */
		odt = 0x0f;	/* ODT tri-state setting */

		if (p_dct_stat->status & (1 <<SB_REGISTERED)) {
			for (cs = 0; cs < 8; cs += 2) {
				if (p_dct_stat->cs_present & (1 << cs)) {
					odt &= ~(1 << (cs / 2));
					/* quad-rank capable platform */
					if (mct_get_nv_bits(NV_4_RANK_TYPE) != 0) {
						if (p_dct_stat->cs_present & (1 << (cs + 1)))
							odt &= ~(4 << (cs / 2));
					}
				}
			}
		} else {		/* AM3 package */
			val = ~(p_dct_stat->cs_present);
			odt = val & 9;	/* swap bits 1 and 2 */
			if (val & (1 << 1))
				odt |= 1 << 2;
			if (val & (1 << 2))
				odt |= 1 << 1;
		}
	}

	val = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c);
	val &= ~(0xf << 8);		/* ODTTri = odt */
	val |= (odt & 0xf) << 8;
	set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c, val);
}

/* Family 15h */
static void init_ddr_phy(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 index;
	u32 dword;
	u8 ddr_voltage_index;
	u8 amd_voltage_level_index = 0;
	u32 index_reg = 0x98;
	u32 dev = p_dct_stat->dev_dct;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	/* Find current DDR supply voltage for this DCT */
	ddr_voltage_index = dct_ddr_voltage_index(p_dct_stat, dct);

	/* Fam15h BKDG v3.14 section 2.10.5.3
	 * The remainder of the Phy Initialization algorithm picks up in
	 * phy_assisted_mem_fence_training
	 */
	set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000b, 0x80000000);
	set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fe013, 0x00000118);

	/* Program desired VDDIO level */
	if (ddr_voltage_index & 0x4) {
		/* 1.25V */
		amd_voltage_level_index = 0x2;
	} else if (ddr_voltage_index & 0x2) {
		/* 1.35V */
		amd_voltage_level_index = 0x1;
	} else if (ddr_voltage_index & 0x1) {
		/* 1.50V */
		amd_voltage_level_index = 0x0;
	}

	/* D18F2x9C_x0D0F_0[F,8:0]1F_dct[1:0][RxVioLvl] */
	for (index = 0; index < 0x9; index++) {
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f001f | (index << 8));
		dword &= ~(0x3 << 3);
		dword |= (amd_voltage_level_index << 3);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f001f | (index << 8), dword);
	}

	/* D18F2x9C_x0D0F_[C,8,2][2:0]1F_dct[1:0][RxVioLvl] */
	for (index = 0; index < 0x3; index++) {
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f201f | (index << 8));
		dword &= ~(0x3 << 3);
		dword |= (amd_voltage_level_index << 3);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f201f | (index << 8), dword);
	}
	for (index = 0; index < 0x2; index++) {
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f801f | (index << 8));
		dword &= ~(0x3 << 3);
		dword |= (amd_voltage_level_index << 3);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f801f | (index << 8), dword);
	}
	for (index = 0; index < 0x1; index++) {
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc01f | (index << 8));
		dword &= ~(0x3 << 3);
		dword |= (amd_voltage_level_index << 3);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc01f | (index << 8), dword);
	}

	/* D18F2x9C_x0D0F_4009_dct[1:0][CmpVioLvl, ComparatorAdjust] */
	/* NOTE: CmpVioLvl and ComparatorAdjust only take effect when set on DCT 0 */
	dword = get_nb32_index_wait_dct(dev, 0, index_reg, 0x0d0f4009);
	dword &= ~(0x0000c00c);
	dword |= (amd_voltage_level_index << 14);
	dword |= (amd_voltage_level_index << 2);
	set_nb32_index_wait_dct(dev, 0, index_reg, 0x0d0f4009, dword);

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

void init_phy_compensation(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 i;
	u32 index_reg = 0x98;
	u32 dev = p_dct_stat->dev_dct;
	u32 valx = 0;
	u8 index;
	u32 dword;
	const u8 *p;

	printk(BIOS_DEBUG, "%s: DCT %d: Start\n", __func__, dct);

	if (is_fam15h()) {
		/* Algorithm detailed in the Fam15h BKDG Rev. 3.14 section 2.10.5.3.4 */
		u32 tx_pre;
		u32 drive_strength;

		/* Program D18F2x9C_x0D0F_E003_dct[1:0][DIS_AUTO_COMP] */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fe003);
		dword |= (0x1 << 14);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fe003, dword);

		/* Program D18F2x9C_x0D0F_E003_dct[1:0][DisablePredriverCal] */
		/* NOTE: DisablePredriverCal only takes effect when set on DCT 0 */
		dword = get_nb32_index_wait_dct(dev, 0, index_reg, 0x0d0fe003);
		dword |= (0x1 << 13);
		set_nb32_index_wait_dct(dev, 0, index_reg, 0x0d0fe003, dword);

		/* Determine TxPreP/TxPreN for data lanes (Stage 1) */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000000);
		drive_strength = (dword >> 20) & 0x7;	/* DqsDrvStren */
		tx_pre = fam15h_phy_predriver_calibration_code(p_dct_stat, dct, drive_strength);

		/* Program TxPreP/TxPreN for data lanes (Stage 1) */
		for (index = 0; index < 0x9; index++) {
			dword = get_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f0006 | (index << 8));
			dword &= ~(0xffff);
			dword |= tx_pre;
			set_nb32_index_wait_dct(dev, dct, index_reg,
						0x0d0f0006 | (index << 8), dword);
		}

		/* Determine TxPreP/TxPreN for data lanes (Stage 2) */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000000);
		drive_strength = (dword >> 16) & 0x7;	/* DataDrvStren */
		tx_pre = fam15h_phy_predriver_calibration_code(p_dct_stat, dct, drive_strength);

		/* Program TxPreP/TxPreN for data lanes (Stage 2) */
		for (index = 0; index < 0x9; index++) {
			dword = get_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f000a | (index << 8));
			dword &= ~(0xffff);
			dword |= tx_pre;
			set_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f000a | (index << 8), dword);
		}
		for (index = 0; index < 0x9; index++) {
			dword = get_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f0002 | (index << 8));
			dword &= ~(0xffff);
			dword |= (0x8000 | tx_pre);
			set_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f0002 | (index << 8), dword);
		}

		/* Determine TxPreP/TxPreN for command/address lines (Stage 1) */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000000);
		drive_strength = (dword >> 4) & 0x7;	/* CsOdtDrvStren */
		tx_pre = fam15h_phy_predriver_cmd_addr_calibration_code(p_dct_stat, dct,
									drive_strength);

		/* Program TxPreP/TxPreN for command/address lines (Stage 1) */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8006);
		dword &= ~(0xffff);
		dword |= tx_pre;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8006, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f800a);
		dword &= ~(0xffff);
		dword |= tx_pre;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f800a, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8002);
		dword &= ~(0xffff);
		dword |= (0x8000 | tx_pre);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8002, dword);

		/* Determine TxPreP/TxPreN for command/address lines (Stage 2) */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000000);
		drive_strength = (dword >> 8) & 0x7;	/* AddrCmdDrvStren */
		tx_pre = fam15h_phy_predriver_cmd_addr_calibration_code(p_dct_stat, dct,
									drive_strength);

		/* Program TxPreP/TxPreN for command/address lines (Stage 2) */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8106);
		dword &= ~(0xffff);
		dword |= tx_pre;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8106, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f810a);
		dword &= ~(0xffff);
		dword |= tx_pre;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f810a, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc006);
		dword &= ~(0xffff);
		dword |= tx_pre;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc006, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc00a);
		dword &= ~(0xffff);
		dword |= tx_pre;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc00a, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc00e);
		dword &= ~(0xffff);
		dword |= tx_pre;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc00e, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc012);
		dword &= ~(0xffff);
		dword |= tx_pre;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc012, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8102);
		dword &= ~(0xffff);
		dword |= (0x8000 | tx_pre);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8102, dword);

		/* Determine TxPreP/TxPreN for command/address lines (Stage 3) */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000000);
		drive_strength = (dword >> 0) & 0x7;	/* CkeDrvStren */
		tx_pre = fam15h_phy_predriver_cmd_addr_calibration_code(p_dct_stat, dct,
									drive_strength);

		/* Program TxPreP/TxPreN for command/address lines (Stage 3) */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc002);
		dword &= ~(0xffff);
		dword |= (0x8000 | tx_pre);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc002, dword);

		/* Determine TxPreP/TxPreN for clock lines */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000000);
		drive_strength = (dword >> 12) & 0x7;	/* ClkDrvStren */
		tx_pre = fam15h_phy_predriver_clk_calibration_code(p_dct_stat, dct,
									drive_strength);

		/* Program TxPreP/TxPreN for clock lines */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f2002);
		dword &= ~(0xffff);
		dword |= (0x8000 | tx_pre);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f2002, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f2102);
		dword &= ~(0xffff);
		dword |= (0x8000 | tx_pre);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f2102, dword);
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f2202);
		dword &= ~(0xffff);
		dword |= (0x8000 | tx_pre);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f2202, dword);

		if (!is_model10_1f()) {
			/* Be extra safe and wait for the predriver calibration
			 * to be applied to the hardware.  The BKDG does not
			 * require this, but it does take some time for the
			 * data to propagate, so it's probably a good idea.
			 */
			u8 predriver_cal_pending = 1;
			printk(BIOS_DEBUG,
				"Waiting for predriver calibration to be applied...");
			while (predriver_cal_pending) {
				predriver_cal_pending = 0;
				for (index = 0; index < 0x9; index++) {
					if (get_nb32_index_wait_dct(dev, dct, index_reg,
							0x0d0f0002 | (index << 8)) & 0x8000)
						predriver_cal_pending = 1;
				}
			}
			printk(BIOS_DEBUG, "done!\n");
		}
	} else {
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00);
		dword = 0;
		for (i = 0; i < 6; i++) {
			switch (i) {
				case 0:
				case 4:
					p = table_comp_rise_slew_15x;
					valx = p[(dword >> 16) & 3];
					break;
				case 1:
				case 5:
					p = table_comp_fall_slew_15x;
					valx = p[(dword >> 16) & 3];
					break;
				case 2:
					p = table_comp_rise_slew_20x;
					valx = p[(dword >> 8) & 3];
					break;
				case 3:
					p = table_comp_fall_slew_20x;
					valx = p[(dword >> 8) & 3];
					break;
			}
			dword |= valx << (5 * i);
		}

		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0a, dword);
	}

	printk(BIOS_DEBUG, "%s: DCT %d: Done\n", __func__, dct);
}

static void mct_early_arb_en_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	if (!is_fam15h()) {
		u32 reg;
		u32 val;
		u32 dev = p_dct_stat->dev_dct;

		/* GhEnhancement #18429 modified by askar: For low NB CLK :
		* Memclk ratio, the DCT may need to arbitrate early to avoid
		* unnecessary bubbles.
		* bit 19 of F2x[1,0]78 Dram  Control Register, set this bit only when
		* NB CLK : Memclk ratio is between 3:1 (inclusive) to 4:5 (inclusive)
		*/
		reg = 0x78;
		val = get_nb32_dct(dev, dct, reg);

		if (p_dct_stat->logical_cpuid & (AMD_DR_Cx | AMD_DR_Dx))
			val |= (1 << EARLY_ARB_EN);
		else if (check_nb_cof_early_arb_en(p_mct_stat, p_dct_stat))
			val |= (1 << EARLY_ARB_EN);

		set_nb32_dct(dev, dct, reg, val);
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

static u8 check_nb_cof_early_arb_en(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u32 reg;
	u32 val;
	u32 tmp;
	u32 rem;
	u32 dev = p_dct_stat->dev_dct;
	u32 hi, lo;
	u8 nb_did = 0;

	/* Check if NB COF >= 4*Memclk, if it is not, return a fatal error
	 */

	/* 3*(Fn2xD4[NBFid]+4)/(2^nb_did)/(3+Fn2x94[mem_clk_freq]) */
	_rdmsr(MSR_COFVID_STS, &lo, &hi);
	if (lo & (1 << 22))
		nb_did |= 1;

	reg = 0x94;
	val = get_nb32_dct(dev, 0, reg);
	if (!(val & (1 << MEM_CLK_FREQ_VAL)))
		val = get_nb32_dct(dev, 1, reg);	/* get the DCT1 value */

	val &= 0x07;
	val += 3;
	if (nb_did)
		val <<= 1;
	tmp = val;

	dev = p_dct_stat->dev_nbmisc;
	reg = 0xD4;
	val = get_nb32(dev, reg);
	val &= 0x1F;
	val += 3;
	val *= 3;
	val = val / tmp;
	rem = val % tmp;
	tmp >>= 1;

	/* Yes this could be nicer but this was how the asm was.... */
	if (val < 3) {				/* NClk:MemClk < 3:1 */
		return 0;
	} else if (val > 4) {			/* NClk:MemClk >= 5:1 */
		return 0;
	} else if ((val == 4) && (rem > tmp)) { /* NClk:MemClk > 4.5:1 */
		return 0;
	} else {
		return 1;			/* 3:1 <= NClk:MemClk <= 4.5:1*/
	}
}

static void mct_reset_data_struct_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	struct DCTStatStruc *p_dct_stat;

	/* Initialize Data structures by clearing all entries to 0 */
	memset(p_mct_stat, 0, sizeof(*p_mct_stat));

	for (node = 0; node < 8; node++) {
		p_dct_stat = p_dct_stat_a + node;
		memset(p_dct_stat, 0,
			sizeof(*p_dct_stat) - sizeof(p_dct_stat->persistent_data));
	}
}

static void mct_before_dram_init__prod_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	mct_ProgramODT_D(p_mct_stat, p_dct_stat, dct);

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

static void mct_ProgramODT_D(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 i;
	u32 dword;
	u32 dev = p_dct_stat->dev_dct;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	if (is_fam15h()) {
		/* Obtain number of DIMMs on channel */
		u8 dimm_count = p_dct_stat->ma_dimms[dct];
		u8 rank_count_dimm0;
		u8 rank_count_dimm1;
		u32 odt_pattern_0;
		u32 odt_pattern_1;
		u32 odt_pattern_2;
		u32 odt_pattern_3;
		u8 write_odt_duration;
		u8 read_odt_duration;
		u8 write_odt_delay;
		u8 read_odt_delay;

		/* NOTE
		 * Rank count per dimm and DCT is encoded by
		 * p_dct_stat->dimm_ranks[(<dimm number> * 2) + dct]
		 */

		/* Select appropriate ODT pattern for installed DIMMs
		 * Refer to the Fam15h BKDG Rev. 3.14, page 149 onwards
		 */
		if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];
					if (rank_count_dimm1 == 1) {
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x00000000;
						odt_pattern_2 = 0x00000000;
						odt_pattern_3 = 0x00020000;
					} else if (rank_count_dimm1 == 2) {
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x00000000;
						odt_pattern_2 = 0x00000000;
						odt_pattern_3 = 0x08020000;
					} else if (rank_count_dimm1 == 4) {
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x00000000;
						odt_pattern_2 = 0x020a0000;
						odt_pattern_3 = 0x080a0000;
					} else {
						/* Fallback */
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x00000000;
						odt_pattern_2 = 0x00000000;
						odt_pattern_3 = 0x08020000;
					}
				} else {
					/* 2 DIMMs detected */
					rank_count_dimm0
						= p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];
					if ((rank_count_dimm0 < 4)
					&& (rank_count_dimm1 < 4)) {
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x01010202;
						odt_pattern_2 = 0x00000000;
						odt_pattern_3 = 0x09030603;
					} else if ((rank_count_dimm0 < 4)
					&& (rank_count_dimm1 == 4)) {
						odt_pattern_0 = 0x01010000;
						odt_pattern_1 = 0x01010a0a;
						odt_pattern_2 = 0x01090000;
						odt_pattern_3 = 0x01030e0b;
					} else if ((rank_count_dimm0 == 4)
					&& (rank_count_dimm1 < 4)) {
						odt_pattern_0 = 0x00000202;
						odt_pattern_1 = 0x05050202;
						odt_pattern_2 = 0x00000206;
						odt_pattern_3 = 0x0d070203;
					} else if ((rank_count_dimm0 == 4)
					&& (rank_count_dimm1 == 4)) {
						odt_pattern_0 = 0x05050a0a;
						odt_pattern_1 = 0x05050a0a;
						odt_pattern_2 = 0x050d0a0e;
						odt_pattern_3 = 0x05070a0b;
					} else {
						/* Fallback */
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x00000000;
						odt_pattern_2 = 0x00000000;
						odt_pattern_3 = 0x00000000;
					}
				}
			} else {
				/* FIXME
				 * 3 DIMMs per channel UNIMPLEMENTED
				 */
				odt_pattern_0 = 0x00000000;
				odt_pattern_1 = 0x00000000;
				odt_pattern_2 = 0x00000000;
				odt_pattern_3 = 0x00000000;
			}
		} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* TODO
			 * Load reduced dimms UNIMPLEMENTED
			 */
			odt_pattern_0 = 0x00000000;
			odt_pattern_1 = 0x00000000;
			odt_pattern_2 = 0x00000000;
			odt_pattern_3 = 0x00000000;
		} else {
			if (max_dimms_installable == 2) {
				if (dimm_count == 1) {
					/* 1 dimm detected */
					rank_count_dimm1
						= p_dct_stat->dimm_ranks[(1 * 2) + dct];
					if (rank_count_dimm1 == 1) {
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x00000000;
						odt_pattern_2 = 0x00000000;
						odt_pattern_3 = 0x00020000;
					} else if (rank_count_dimm1 == 2) {
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x00000000;
						odt_pattern_2 = 0x00000000;
						odt_pattern_3 = 0x08020000;
					} else {
						/* Fallback */
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x00000000;
						odt_pattern_2 = 0x00000000;
						odt_pattern_3 = 0x08020000;
					}
				} else {
					/* 2 DIMMs detected */
					odt_pattern_0 = 0x00000000;
					odt_pattern_1 = 0x01010202;
					odt_pattern_2 = 0x00000000;
					odt_pattern_3 = 0x09030603;
				}
			} else {
				/* FIXME
				 * 3 DIMMs per channel UNIMPLEMENTED
				 */
				odt_pattern_0 = 0x00000000;
				odt_pattern_1 = 0x00000000;
				odt_pattern_2 = 0x00000000;
				odt_pattern_3 = 0x00000000;
			}
		}

		if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* TODO
			 * Load reduced dimms UNIMPLEMENTED
			 */
			write_odt_duration = 0x0;
			read_odt_duration = 0x0;
			write_odt_delay = 0x0;
			read_odt_delay = 0x0;
		} else {
			u8 tcl;
			u8 tcwl;
			tcl = get_nb32_dct(dev, dct, 0x200) & 0x1f;
			tcwl = get_nb32_dct(dev, dct, 0x20c) & 0x1f;

			write_odt_duration = 0x6;
			read_odt_duration = 0x6;
			write_odt_delay = 0x0;
			if (tcl > tcwl)
				read_odt_delay = tcl - tcwl;
			else
				read_odt_delay = 0x0;
		}

		/* Program ODT pattern */
		set_nb32_dct(dev, dct, 0x230, odt_pattern_1);
		set_nb32_dct(dev, dct, 0x234, odt_pattern_0);
		set_nb32_dct(dev, dct, 0x238, odt_pattern_3);
		set_nb32_dct(dev, dct, 0x23c, odt_pattern_2);
		dword = get_nb32_dct(dev, dct, 0x240);
		/* WrOdtOnDuration = write_odt_duration */
		dword &= ~(0x7 << 12);
		dword |= (write_odt_duration & 0x7) << 12;
		/* WrOdtTrnOnDly = write_odt_delay */
		dword &= ~(0x7 << 8);
		dword |= (write_odt_delay & 0x7) << 8;
		/* RdOdtOnDuration = read_odt_duration */
		dword &= ~(0xf << 4);
		dword |= (read_odt_duration & 0xf) << 4;
		/* RdOdtTrnOnDly = read_odt_delay */
		dword &= ~(0xf);
		dword |= (read_odt_delay & 0xf);
		set_nb32_dct(dev, dct, 0x240, dword);

		printk(BIOS_SPEW,
			"Programmed DCT %d ODT pattern %08x %08x %08x %08x\n",
			dct, odt_pattern_0, odt_pattern_1, odt_pattern_2, odt_pattern_3);
	} else if (p_dct_stat->logical_cpuid & AMD_DR_Dx) {
		if (p_dct_stat->speed == 3)
			dword = 0x00000800;
		else
			dword = 0x00000000;
		for (i = 0; i < 2; i++) {
			set_nb32_dct(dev, i, 0x98, 0x0D000030);
			set_nb32_dct(dev, i, 0x9C, dword);
			set_nb32_dct(dev, i, 0x98, 0x4D040F30);

			/* Obtain number of DIMMs on channel */
			u8 dimm_count = p_dct_stat->ma_dimms[i];
			u8 rank_count_dimm0;
			u8 rank_count_dimm1;
			u32 odt_pattern_0;
			u32 odt_pattern_1;
			u32 odt_pattern_2;
			u32 odt_pattern_3;

			/* Select appropriate ODT pattern for installed DIMMs
			 * Refer to the Fam10h BKDG Rev. 3.62, page 120 onwards
			 */
			if (p_dct_stat->status & (1 << SB_REGISTERED)) {
				if (max_dimms_installable == 2) {
					if (dimm_count == 1) {
						/* 1 dimm detected */
						rank_count_dimm1
							= p_dct_stat->dimm_ranks[(1 * 2) + i];
						if (rank_count_dimm1 == 1) {
							odt_pattern_0 = 0x00000000;
							odt_pattern_1 = 0x00000000;
							odt_pattern_2 = 0x00000000;
							odt_pattern_3 = 0x00020000;
						} else if (rank_count_dimm1 == 2) {
							odt_pattern_0 = 0x00000000;
							odt_pattern_1 = 0x00000000;
							odt_pattern_2 = 0x00000000;
							odt_pattern_3 = 0x02080000;
						} else if (rank_count_dimm1 == 4) {
							odt_pattern_0 = 0x00000000;
							odt_pattern_1 = 0x00000000;
							odt_pattern_2 = 0x020a0000;
							odt_pattern_3 = 0x080a0000;
						} else {
							/* Fallback */
							odt_pattern_0 = 0x00000000;
							odt_pattern_1 = 0x00000000;
							odt_pattern_2 = 0x00000000;
							odt_pattern_3 = 0x00000000;
						}
					} else {
						/* 2 DIMMs detected */
						rank_count_dimm0
							= p_dct_stat->dimm_ranks[(0 * 2) + i];
						rank_count_dimm1
							= p_dct_stat->dimm_ranks[(1 * 2) + i];
						if ((rank_count_dimm0 < 4)
						&& (rank_count_dimm1 < 4)) {
							odt_pattern_0 = 0x00000000;
							odt_pattern_1 = 0x01010202;
							odt_pattern_2 = 0x00000000;
							odt_pattern_3 = 0x09030603;
						} else if ((rank_count_dimm0 < 4)
						&& (rank_count_dimm1 == 4)) {
							odt_pattern_0 = 0x01010000;
							odt_pattern_1 = 0x01010a0a;
							odt_pattern_2 = 0x01090000;
							odt_pattern_3 = 0x01030e0b;
						} else if ((rank_count_dimm0 == 4)
						&& (rank_count_dimm1 < 4)) {
							odt_pattern_0 = 0x00000202;
							odt_pattern_1 = 0x05050202;
							odt_pattern_2 = 0x00000206;
							odt_pattern_3 = 0x0d070203;
						} else if ((rank_count_dimm0 == 4)
						&& (rank_count_dimm1 == 4)) {
							odt_pattern_0 = 0x05050a0a;
							odt_pattern_1 = 0x05050a0a;
							odt_pattern_2 = 0x050d0a0e;
							odt_pattern_3 = 0x05070a0b;
						} else {
							/* Fallback */
							odt_pattern_0 = 0x00000000;
							odt_pattern_1 = 0x00000000;
							odt_pattern_2 = 0x00000000;
							odt_pattern_3 = 0x00000000;
						}
					}
				} else {
					/* FIXME
					 * 3 DIMMs per channel UNIMPLEMENTED
					 */
					odt_pattern_0 = 0x00000000;
					odt_pattern_1 = 0x00000000;
					odt_pattern_2 = 0x00000000;
					odt_pattern_3 = 0x00000000;
				}
			} else {
				if (max_dimms_installable == 2) {
					if (dimm_count == 1) {
						/* 1 dimm detected */
						rank_count_dimm1
							= p_dct_stat->dimm_ranks[(1 * 2) + i];
						if (rank_count_dimm1 == 1) {
							odt_pattern_0 = 0x00000000;
							odt_pattern_1 = 0x00000000;
							odt_pattern_2 = 0x00000000;
							odt_pattern_3 = 0x00020000;
						} else if (rank_count_dimm1 == 2) {
							odt_pattern_0 = 0x00000000;
							odt_pattern_1 = 0x00000000;
							odt_pattern_2 = 0x00000000;
							odt_pattern_3 = 0x02080000;
						} else {
							/* Fallback */
							odt_pattern_0 = 0x00000000;
							odt_pattern_1 = 0x00000000;
							odt_pattern_2 = 0x00000000;
							odt_pattern_3 = 0x00000000;
						}
					} else {
						/* 2 DIMMs detected */
						odt_pattern_0 = 0x00000000;
						odt_pattern_1 = 0x01010202;
						odt_pattern_2 = 0x00000000;
						odt_pattern_3 = 0x09030603;
					}
				} else {
					/* FIXME
					 * 3 DIMMs per channel UNIMPLEMENTED
					 */
					odt_pattern_0 = 0x00000000;
					odt_pattern_1 = 0x00000000;
					odt_pattern_2 = 0x00000000;
					odt_pattern_3 = 0x00000000;
				}
			}

			/* Program ODT pattern */
			set_nb32_index_wait_dct(dev, i, 0xf0, 0x180, odt_pattern_1);
			set_nb32_index_wait_dct(dev, i, 0xf0, 0x181, odt_pattern_0);
			set_nb32_index_wait_dct(dev, i, 0xf0, 0x182, odt_pattern_3);
			set_nb32_index_wait_dct(dev, i, 0xf0, 0x183, odt_pattern_2);

			printk(BIOS_SPEW,
				"Programmed ODT pattern %08x %08x %08x %08x\n",
				odt_pattern_0, odt_pattern_1, odt_pattern_2, odt_pattern_3);
		}
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

static void mct_en_dll_shutdown_sr(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 dev = p_dct_stat->dev_dct, val;

	/* Write 0000_07D0h to register F2x[1, 0]98_x4D0FE006 */
	if (p_dct_stat->logical_cpuid & (AMD_DR_DAC2_OR_C3)) {
		set_nb32_dct(dev, dct, 0x9C, 0x1C);
		set_nb32_dct(dev, dct, 0x98, 0x4D0FE006);
		set_nb32_dct(dev, dct, 0x9C, 0x13D);
		set_nb32_dct(dev, dct, 0x98, 0x4D0FE007);

		val = get_nb32_dct(dev, dct, 0x90);
		val &= ~(1 << 27/* DisDllShutdownSR */);
		set_nb32_dct(dev, dct, 0x90, val);
	}
}

static u32 mct_dis_dll_shutdown_sr(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u32 dram_config_lo, u8 dct)
{
	u32 dev = p_dct_stat->dev_dct;

	/* Write 0000_07D0h to register F2x[1, 0]98_x4D0FE006 */
	if (p_dct_stat->logical_cpuid & (AMD_DR_DAC2_OR_C3)) {
		set_nb32_dct(dev, dct, 0x9C, 0x7D0);
		set_nb32_dct(dev, dct, 0x98, 0x4D0FE006);
		set_nb32_dct(dev, dct, 0x9C, 0x190);
		set_nb32_dct(dev, dct, 0x98, 0x4D0FE007);

		dram_config_lo |=  /* DisDllShutdownSR */ 1 << 27;
	}

	return dram_config_lo;
}

void mct_set_cl_to_nb_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat)
{
	u32 lo, hi;
	u32 msr;

	/* FIXME: Maybe check the CPUID? - not for now. */
	/* p_dct_stat->logical_cpuid; */

	msr = BU_CFG2_MSR;
	_rdmsr(msr, &lo, &hi);
	lo |= 1 << CL_LINES_TO_NB_DIS;
	_wrmsr(msr, lo, hi);
}

void mct_clr_cl_to_nb_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat)
{

	u32 lo, hi;
	u32 msr;

	/* FIXME: Maybe check the CPUID? - not for now. */
	/* p_dct_stat->logical_cpuid; */

	msr = BU_CFG2_MSR;
	_rdmsr(msr, &lo, &hi);
	if (!p_dct_stat->cl_to_nb_tag)
		lo &= ~(1 << CL_LINES_TO_NB_DIS);
	_wrmsr(msr, lo, hi);

}

void mct_set_wb_enh_wsb_dis_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 lo, hi;
	u32 msr;

	/* FIXME: Maybe check the CPUID? - not for now. */
	/* p_dct_stat->logical_cpuid; */

	msr = BU_CFG_MSR;
	_rdmsr(msr, &lo, &hi);
	hi |= (1 << WB_ENH_WSB_DIS_D);
	_wrmsr(msr, lo, hi);
}

void mct_clr_wb_enh_wsb_dis_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 lo, hi;
	u32 msr;

	/* FIXME: Maybe check the CPUID? - not for now. */
	/* p_dct_stat->logical_cpuid; */

	msr = BU_CFG_MSR;
	_rdmsr(msr, &lo, &hi);
	hi &= ~(1 << WB_ENH_WSB_DIS_D);
	_wrmsr(msr, lo, hi);
}

void prog_dram_mrs_reg_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 dram_mrs, dword;
	u8 byte;

	dram_mrs = 0;

	/* Set chip select CKE control mode */
	if (mct_get_nv_bits(NV_CKE_CTL)) {
		if (p_dct_stat->cs_present == 3) {
			u16 word;
			word = p_dct_stat->dimm_spd_checksum_err;
			if (dct == 0)
				word &= 0b01010100;
			else
				word &= 0b10101000;
			if (word == 0)
				dram_mrs |= 1 << 23;
		}
	}

	if (is_fam15h()) {
		dram_mrs |= (0x1 << 23);		/* PCHG_PD_MODE_SEL = 1 */
	} else {
		/*
		DRAM MRS Register
		DrvImpCtrl: drive impedance control.01b(34 ohm driver; Ron34 = Rzq/7)
		*/
		dram_mrs |= 1 << 2;
		/* Dram nominal termination: */
		byte = p_dct_stat->ma_dimms[dct];
		if (!(p_dct_stat->status & (1 << SB_REGISTERED))) {
			dram_mrs |= 1 << 7; /* 60 ohms */
			if (byte & 2) {
				if (p_dct_stat->speed < 6)
					dram_mrs |= 1 << 8; /* 40 ohms */
				else
					dram_mrs |= 1 << 9; /* 30 ohms */
			}
		}
		/* Dram dynamic termination: Disable(1DIMM), 120ohm(>=2DIMM) */
		if (!(p_dct_stat->status & (1 << SB_REGISTERED))) {
			if (byte >= 2) {
				if (p_dct_stat->speed == 7)
					dram_mrs |= 1 << 10;
				else
					dram_mrs |= 1 << 11;
			}
		} else {
			dram_mrs |= mct_dram_term_dyn_r_dimm(p_mct_stat, p_dct_stat, byte);
		}

		/* QOFF = 0, output buffers enabled */
		/* tcwl */
		dram_mrs |= (p_dct_stat->speed - 4) << 20;
		/* ASR = 1, auto self refresh */
		/* SRT = 0 */
		dram_mrs |= 1 << 18;
	}

	/* burst length control */
	if (p_dct_stat->status & (1 << SB_128_BIT_MODE))
		dram_mrs |= 1 << 1;

	dword = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x84);
	if (is_fam15h()) {
		dword &= ~0x00800003;
		dword |= dram_mrs;
	} else {
		dword &= ~0x00fc2f8f;
		dword |= dram_mrs;
	}
	set_nb32_dct(p_dct_stat->dev_dct, dct, 0x84, dword);
}

void mct_set_dram_config_hi_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u32 dct, u32 dram_config_hi)
{
	/* Bug#15114: Comp. update interrupted by Freq. change can cause
	 * subsequent update to be invalid during any MemClk frequency change:
	 * Solution: From the bug report:
	 *  1. A software-initiated frequency change should be wrapped into the
	 *     following sequence :
	 *	a) Disable Compensation (F2[1, 0]9C_x08[30])
	 *	b) Reset the Begin Compensation bit (D3CMP->COMP_CONFIG[0]) in
	 *	   all the compensation engines
	 *	c) Do frequency change
	 *	d) Enable Compensation (F2[1, 0]9C_x08[30])
	 *  2. A software-initiated Disable Compensation should always be
	 *     followed by step b) of the above steps.
	 * Silicon status: Fixed In Rev B0
	 *
	 * Errata#177: DRAM Phy Automatic Compensation Updates May Be Invalid
	 * Solution: BIOS should disable the phy automatic compensation prior
	 * to initiating a memory clock frequency change as follows:
	 *  1. Disable PhyAutoComp by writing 1'b1 to F2x[1, 0]9C_x08[30]
	 *  2. Reset the Begin Compensation bits by writing 32'h0 to
	 *     F2x[1, 0]9C_x4D004F00
	 *  3. Perform frequency change
	 *  4. Enable PhyAutoComp by writing 1'b0 to F2x[1, 0]9C_08[30]
	 *  In addition, any time software disables the automatic phy
	 *   compensation it should reset the begin compensation bit per step 2.
	 *   Silicon status: Fixed in DR-B0
	 */

	u32 dev = p_dct_stat->dev_dct;
	u32 index_reg = 0x98;
	u32 index;

	u32 dword;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	if (is_fam15h()) {
		/* Initial setup for frequency change
		 * 9C_x0000_0004 must be configured before MEM_CLK_FREQ_VAL is set
		 */

		/* Program D18F2x9C_x0D0F_E006_dct[1:0][PllLockTime] = 0x190 */
		dword = get_nb32_index_wait_dct(
				p_dct_stat->dev_dct, dct, index_reg, 0x0d0fe006);
		dword &= ~(0x0000ffff);
		dword |= 0x00000190;
		set_nb32_index_wait_dct(p_dct_stat->dev_dct, dct, index_reg, 0x0d0fe006, dword);

		dword = get_nb32_dct(dev, dct, 0x94);
		dword &= ~(1 << MEM_CLK_FREQ_VAL);
		set_nb32_dct(dev, dct, 0x94, dword);

		dword = dram_config_hi;
		dword &= ~(1 << MEM_CLK_FREQ_VAL);
		set_nb32_dct(dev, dct, 0x94, dword);

		mct_get_ps_cfg_d(p_mct_stat, p_dct_stat, dct);
		set_2t_configuration(p_mct_stat, p_dct_stat, dct);
		mct_before_platform_spec(p_mct_stat, p_dct_stat, dct);
		mct_platform_spec(p_mct_stat, p_dct_stat, dct);
	} else {
		index = 0x08;
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, index);
		if (!(dword & (1 << DIS_AUTO_COMP)))
			set_nb32_index_wait_dct(dev, dct, index_reg, index,
						dword | (1 << DIS_AUTO_COMP));

		mct_wait(100);
	}

	printk(BIOS_DEBUG,
		"mct_set_dram_config_hi_d: dram_config_hi:    %08x\n",
		dram_config_hi);

	/* Program the DRAM Configuration High register */
	set_nb32_dct(dev, dct, 0x94, dram_config_hi);

	if (is_fam15h()) {
		/* Wait until F2x[1, 0]94[FREQ_CHG_IN_PROG]=0. */
		do {
			printk(BIOS_DEBUG, "*");
			dword = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94);
		} while (dword & (1 << FREQ_CHG_IN_PROG));
		printk(BIOS_DEBUG, "\n");

		/* Program D18F2x9C_x0D0F_E006_dct[1:0][PllLockTime] = 0xf */
		dword = get_nb32_index_wait_dct(
					p_dct_stat->dev_dct, dct, index_reg, 0x0d0fe006);
		dword &= ~(0x0000ffff);
		dword |= 0x0000000f;
		set_nb32_index_wait_dct(p_dct_stat->dev_dct, dct, index_reg, 0x0d0fe006, dword);
	}

	/* Clear MC4 error status */
	pci_write_config32(p_dct_stat->dev_nbmisc, 0x48, 0x0);
	pci_write_config32(p_dct_stat->dev_nbmisc, 0x4c, 0x0);

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

static void mct_before_dqs_train_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a)
{
	if (!is_fam15h()) {
		u8 node;
		struct DCTStatStruc *p_dct_stat;

		/* Errata 178
		 *
		 * Bug#15115: Uncertainty In The Sync Chain Leads To Setup Violations
		 *            In TX FIFO
		 * Solution: BIOS should program DRAM Control Register[RdPtrInit] =
		 *            5h, (F2x[1, 0]78[3:0] = 5h).
		 * Silicon status: Fixed In Rev B0
		 *
		 * Bug#15880: Determine validity of reset settings for DDR PHY timing.
		 * Solution: At least, set WrDqs fine delay to be 0 for DDR3 training.
		 */
		for (node = 0; node < 8; node++) {
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present) {
				mct_before_dqs_train_samp(p_dct_stat); /* only Bx */
				mct_reset_dll_d(p_mct_stat, p_dct_stat, 0);
				mct_reset_dll_d(p_mct_stat, p_dct_stat, 1);
			}
		}
	}
}

/* Erratum 350 */
static void mct_reset_dll_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 receiver;
	u32 dev = p_dct_stat->dev_dct;
	u32 addr;
	u32 lo, hi;
	u8 wrap32dis = 0;
	u8 valid = 0;

	/* Skip reset DLL for B3 */
	if (p_dct_stat->logical_cpuid & AMD_DR_B3) {
		return;
	}

	/* Skip reset DLL for Family 15h */
	if (is_fam15h()) {
		return;
	}

	addr = HWCR_MSR;
	_rdmsr(addr, &lo, &hi);
	if (lo & (1 << 17)) {		/* save the old value */
		wrap32dis = 1;
	}
	lo |= (1 << 17);		/* HWCR.wrap32dis */
	/* Setting wrap32dis allows 64-bit memory references in 32bit mode */
	_wrmsr(addr, lo, hi);

	p_dct_stat->channel = dct;
	receiver = mct_init_receiver_d(p_dct_stat, dct);
	/* there are four receiver pairs, loosely associated with chipselects.*/
	for (; receiver < 8; receiver += 2) {
		if (mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, dct, receiver)) {
			addr = mct_get_rcvr_sys_addr_d(
						p_mct_stat, p_dct_stat, dct, receiver, &valid);
			if (valid) {
				/* cache fills */
				mct_read_1l_test_pattern_d(p_mct_stat, p_dct_stat, addr);

				/* Write 0000_8000h to register F2x[1,0]9C_xD080F0C */
				set_nb32_index_wait_dct(dev, dct, 0x98, 0xD080F0C, 0x00008000);
				mct_wait(80); /* wait >= 300ns */

				/* Write 0000_0000h to register F2x[1,0]9C_xD080F0C */
				set_nb32_index_wait_dct(dev, dct, 0x98, 0xD080F0C, 0x00000000);
				mct_wait(800); /* wait >= 2us */
				break;
			}
		}
	}

	if (!wrap32dis) {
		addr = HWCR_MSR;
		_rdmsr(addr, &lo, &hi);
		lo &= ~(1 << 17);	/* restore HWCR.wrap32dis */
		_wrmsr(addr, lo, hi);
	}
}

void mct_enable_dat_intlv_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 val;

	/*  Enable F2x110[DctDatIntlv] */
	/* Call back not required mctHookBeforeDatIntlv_D() */
	/* FIXME Skip for Ax */
	if (!p_dct_stat->ganged_mode) {
		val = get_nb32(dev, 0x110);
		val |= 1 << 5;			/* DctDatIntlv */
		set_nb32(dev, 0x110, val);

		/* FIXME Skip for Cx */
		dev = p_dct_stat->dev_nbmisc;
		val = get_nb32(dev, 0x8C);	/* NB Configuration Hi */
		val |= 1 << (36-32);		/* DisDatMask */
		set_nb32(dev, 0x8C, val);
	}
}

void set_dll_speed_up_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	if (!is_fam15h()) {
		u32 val;
		u32 dev = p_dct_stat->dev_dct;

		if (p_dct_stat->speed >= mhz_to_memclk_config(800)) { /* DDR1600 and above */
			/* Set bit13 PowerDown to register F2x[1, 0]98_x0D080F10 */
			set_nb32_dct(dev, dct, 0x98, 0x0D080F10);
			val = get_nb32_dct(dev, dct, 0x9C);
			val |= 1 < 13;
			set_nb32_dct(dev, dct, 0x9C, val);
			set_nb32_dct(dev, dct, 0x98, 0x4D080F10);

			/* Set bit13 PowerDown to register F2x[1, 0]98_x0D080F11 */
			set_nb32_dct(dev, dct, 0x98, 0x0D080F11);
			val = get_nb32_dct(dev, dct, 0x9C);
			val |= 1 < 13;
			set_nb32_dct(dev, dct, 0x9C, val);
			set_nb32_dct(dev, dct, 0x98, 0x4D080F11);

			/* Set bit13 PowerDown to register F2x[1, 0]98_x0D088F30 */
			set_nb32_dct(dev, dct, 0x98, 0x0D088F30);
			val = get_nb32_dct(dev, dct, 0x9C);
			val |= 1 < 13;
			set_nb32_dct(dev, dct, 0x9C, val);
			set_nb32_dct(dev, dct, 0x98, 0x4D088F30);

			/* Set bit13 PowerDown to register F2x[1, 0]98_x0D08CF30 */
			set_nb32_dct(dev, dct, 0x98, 0x0D08CF30);
			val = get_nb32_dct(dev, dct, 0x9C);
			val |= 1 < 13;
			set_nb32_dct(dev, dct, 0x9C, val);
			set_nb32_dct(dev, dct, 0x98, 0x4D08CF30);
		}
	}
}

static void sync_settings(struct DCTStatStruc *p_dct_stat)
{
	/* set F2x78[CH_SETUP_SYNC] when F2x[1, 0]9C_x04[AddrCmdSetup, CsOdtSetup,
	 * CkeSetup] setups for one DCT are all 0s and at least one of the setups,
	 * F2x[1, 0]9C_x04[AddrCmdSetup, CsOdtSetup, CkeSetup], of the other
	 * controller is 1
	 */
	u32 cha, chb;
	u32 dev = p_dct_stat->dev_dct;
	u32 val;

	cha = p_dct_stat->ch_addr_tmg[0] & 0x0202020;
	chb = p_dct_stat->ch_addr_tmg[1] & 0x0202020;

	if ((cha != chb) && ((cha == 0) || (chb == 0))) {
		val = get_nb32(dev, 0x78);
		val |= 1 << CH_SETUP_SYNC;
		set_nb32(dev, 0x78, val);
	}
}

static void after_dram_init_d(struct DCTStatStruc *p_dct_stat, u8 dct) {

	u32 val;
	u32 dev = p_dct_stat->dev_dct;

	if (p_dct_stat->logical_cpuid & (AMD_DR_B2 | AMD_DR_B3)) {
		mct_wait(10000);	/* Wait 50 us*/
		val = get_nb32(dev, 0x110);
		if (!(val & (1 << DRAM_ENABLED))) {
			/* If 50 us expires while DramEnable =0 then do the following */
			val = get_nb32_dct(dev, dct, 0x90);
			val &= ~(1 << WIDTH_128);		/* Program WIDTH_128 = 0 */
			set_nb32_dct(dev, dct, 0x90, val);

			/* Perform dummy CSR read to F2x09C_x05 */
			val = get_nb32_index_wait_dct(dev, dct, 0x98, 0x05);

			if (p_dct_stat->ganged_mode) {
				val = get_nb32_dct(dev, dct, 0x90);
				val |= 1 << WIDTH_128;		/* Program WIDTH_128 = 0 */
				set_nb32_dct(dev, dct, 0x90, val);
			}
		}
	}
}

/* ==========================================================
 *  6-bit Bank Addressing Table
 *  RR = rows-13 binary
 *  B = banks-2 binary
 *  CCC = Columns-9 binary
 * ==========================================================
 *  DCT	CCCBRR	rows	banks	Columns	64-bit CS Size
 *  Encoding
 *  0000	000000	13	2	9	128MB
 *  0001	001000	13	2	10	256MB
 *  0010	001001	14	2	10	512MB
 *  0011	010000	13	2	11	512MB
 *  0100	001100	13	3	10	512MB
 *  0101	001101	14	3	10	1GB
 *  0110	010001	14	2	11	1GB
 *  0111	001110	15	3	10	2GB
 *  1000	010101	14	3	11	2GB
 *  1001	010110	15	3	11	4GB
 *  1010	001111	16	3	10	4GB
 *  1011	010111	16	3	11	8GB
 */
u8 crc_check(struct DCTStatStruc *p_dct_stat, u8 dimm)
{
	u16 crc_calc = spd_ddr3_calc_crc(p_dct_stat->spd_data.spd_bytes[dimm],
		sizeof(p_dct_stat->spd_data.spd_bytes[dimm]));
	u16 checksum_spd = p_dct_stat->spd_data.spd_bytes[dimm][SPD_BYTE_127] << 8
		| p_dct_stat->spd_data.spd_bytes[dimm][SPD_BYTE_126];

	return crc_calc == checksum_spd;
}

int32_t abs(int32_t val)
{
	if (val < 0)
		return -val;
	return val;
}
