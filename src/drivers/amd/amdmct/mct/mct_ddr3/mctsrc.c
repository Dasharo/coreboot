/* SPDX-License-Identifier: GPL-2.0-only */

/******************************************************************************
 Description: receiver En and DQS Timing Training feature for DDR 3 MCT
******************************************************************************/

#include <arch/cpu.h>
#include <stdint.h>
#include <console/console.h>
#include <cpu/x86/cr.h>
#include <string.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static void dqs_train_rcvr_en_sw_fam10(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 pass);
static void dqs_train_rcvr_en_sw_fam15(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 pass);
static void mct_init_dqs_pos_4_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
					 struct DCTStatStruc *p_dct_stat);
static void init_dqs_pos_4_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 channel);
static void calc_ecc_dqs_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 channel);
static void mct_set_max_latency_d(struct DCTStatStruc *p_dct_stat, u8 channel, u16 dqs_rcv_en_dly);
static void mct_disable_dqs_rcv_en_d(struct DCTStatStruc *p_dct_stat);

/* Warning:  These must be located so they do not cross a logical 16-bit segment boundary! */
const u32 test_pattern_0_d[] = {
	0x55555555, 0x55555555, 0x55555555, 0x55555555,
	0x55555555, 0x55555555, 0x55555555, 0x55555555,
	0x55555555, 0x55555555, 0x55555555, 0x55555555,
	0x55555555, 0x55555555, 0x55555555, 0x55555555,
};
const u32 test_pattern_1_d[] = {
	0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
	0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
	0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
	0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
};
const u32 test_pattern_2_d[] = {
	0x12345678, 0x87654321, 0x23456789, 0x98765432,
	0x59385824, 0x30496724, 0x24490795, 0x99938733,
	0x40385642, 0x38465245, 0x29432163, 0x05067894,
	0x12349045, 0x98723467, 0x12387634, 0x34587623,
};

static void setup_rcvr_pattern(struct MCTStatStruc *p_mct_stat,
		struct DCTStatStruc *p_dct_stat, u32 *buffer, u8 pass)
{
	/*
	 * 1. Copy the alpha and Beta patterns from ROM to Cache,
	 *     aligning on 16 byte boundary
	 * 2. Set the ptr to DCTStatstruc.PtrPatternBufA for Alpha
	 * 3. Set the ptr to DCTStatstruc.PtrPatternBufB for Beta
	 */
	u32 *buf_a;
	u32 *buf_b;
	u32 *p_a;
	u32 *p_b;
	u8 i;

	buf_a = (u32 *)(((u32)buffer + 0x10) & (0xfffffff0));
	buf_b = buf_a + 32; /* ?? */
	p_a = (u32 *)setup_dqs_pattern_1_pass_b(pass);
	p_b = (u32 *)setup_dqs_pattern_1_pass_a(pass);

	for (i = 0; i < 16; i++) {
		buf_a[i] = p_a[i];
		buf_b[i] = p_b[i];
	}

	p_dct_stat->ptr_pattern_buf_a = (u32)buf_a;
	p_dct_stat->ptr_pattern_buf_b = (u32)buf_b;
}

void mct_train_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 pass)
{
	if (mct_check_number_of_dqs_rcv_en_1_pass(pass)) {
		if (is_fam15h())
			dqs_train_rcvr_en_sw_fam15(p_mct_stat, p_dct_stat, pass);
		else
			dqs_train_rcvr_en_sw_fam10(p_mct_stat, p_dct_stat, pass);
	}
}

static u16 fam15_receiver_enable_training_seed(struct DCTStatStruc *p_dct_stat, u8 dct, u8 dimm, u8 rank, u8 package_type)
{
	u32 dword;
	u16 seed = 0;

	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	u8 channel = dct;
	if (package_type == PT_GR) {
		/* Get the internal node number */
		dword = get_nb32(p_dct_stat->dev_nbmisc, 0xe8);
		dword = (dword >> 30) & 0x3;
		if (dword == 1) {
			channel += 2;
		}
	}

	if (p_dct_stat->status & (1 << SB_REGISTERED)) {
		if (package_type == PT_GR) {
			/* Socket G34: Fam15h BKDG v3.14 Table 99 */
			if (max_dimms_installable == 1) {
				if (channel == 0)
					seed = 0x43;
				else if (channel == 1)
					seed = 0x3f;
				else if (channel == 2)
					seed = 0x3a;
				else if (channel == 3)
					seed = 0x35;
			} else if (max_dimms_installable == 2) {
				if (channel == 0)
					seed = 0x54;
				else if (channel == 1)
					seed = 0x4d;
				else if (channel == 2)
					seed = 0x45;
				else if (channel == 3)
					seed = 0x40;
			} else if (max_dimms_installable == 3) {
				if (channel == 0)
					seed = 0x6b;
				else if (channel == 1)
					seed = 0x5e;
				else if (channel == 2)
					seed = 0x4b;
				else if (channel == 3)
					seed = 0x3d;
			}
		} else if (package_type == PT_C3) {
			/* Socket C32: Fam15h BKDG v3.14 Table 100 */
			if ((max_dimms_installable == 1) || (max_dimms_installable == 2)) {
				if (channel == 0)
					seed = 0x3f;
				else if (channel == 1)
					seed = 0x3e;
			} else if (max_dimms_installable == 3) {
				if (channel == 0)
					seed = 0x47;
				else if (channel == 1)
					seed = 0x38;
			}
		}
	} else if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
		if (package_type == PT_GR) {
			/* Socket G34: Fam15h BKDG v3.14 Table 99 */
			if (max_dimms_installable == 1) {
				if (channel == 0)
					seed = 0x123;
				else if (channel == 1)
					seed = 0x122;
				else if (channel == 2)
					seed = 0x112;
				else if (channel == 3)
					seed = 0x102;
			}
		} else if (package_type == PT_C3) {
			/* Socket C32: Fam15h BKDG v3.14 Table 100 */
			if (channel == 0)
				seed = 0x132;
			else if (channel == 1)
				seed = 0x122;
		}
	} else {
		if (package_type == PT_GR) {
			/* Socket G34: Fam15h BKDG v3.14 Table 99 */
			if (max_dimms_installable == 1) {
				if (channel == 0)
					seed = 0x3e;
				else if (channel == 1)
					seed = 0x38;
				else if (channel == 2)
					seed = 0x37;
				else if (channel == 3)
					seed = 0x31;
			} else if (max_dimms_installable == 2) {
				if (channel == 0)
					seed = 0x51;
				else if (channel == 1)
					seed = 0x4a;
				else if (channel == 2)
					seed = 0x46;
				else if (channel == 3)
					seed = 0x3f;
			} else if (max_dimms_installable == 3) {
				if (channel == 0)
					seed = 0x5e;
				else if (channel == 1)
					seed = 0x52;
				else if (channel == 2)
					seed = 0x48;
				else if (channel == 3)
					seed = 0x3c;
			}
		} else if (package_type == PT_C3) {
			/* Socket C32: Fam15h BKDG v3.14 Table 100 */
			if ((max_dimms_installable == 1) || (max_dimms_installable == 2)) {
				if (channel == 0)
					seed = 0x39;
				else if (channel == 1)
					seed = 0x32;
			} else if (max_dimms_installable == 3) {
				if (channel == 0)
					seed = 0x45;
				else if (channel == 1)
					seed = 0x37;
			}
		} else if (package_type == PT_M2) {
			/* Socket AM3: Fam15h BKDG v3.14 Table 101 */
			seed = 0x3a;
		} else if (package_type == PT_FM2) {
			/* Socket FM2: Fam15h Model10 BKDG 3.12 Table 43 */
			seed = 0x32;
		}
	}

	printk(BIOS_DEBUG, "%s: using seed: %04x\n", __func__, seed);

	return seed;
}

void read_dqs_write_timing_control_registers(u16 *current_total_delay, u32 dev, u8 dct, u8 dimm, u32 index_reg)
{
	u8 lane;
	u32 dword;

	for (lane = 0; lane < MAX_BYTE_LANES; lane++) {
		u32 wdt_reg;
		if ((lane == 0) || (lane == 1))
			wdt_reg = 0x30;
		if ((lane == 2) || (lane == 3))
			wdt_reg = 0x31;
		if ((lane == 4) || (lane == 5))
			wdt_reg = 0x40;
		if ((lane == 6) || (lane == 7))
			wdt_reg = 0x41;
		if (lane == 8)
			wdt_reg = 0x32;
		wdt_reg += dimm * 3;
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, wdt_reg);
		if ((lane == 7) || (lane == 5) || (lane == 3) || (lane == 1))
			current_total_delay[lane] = (dword & 0x00ff0000) >> 16;
		if ((lane == 8) || (lane == 6) || (lane == 4) || (lane == 2) || (lane == 0))
			current_total_delay[lane] = dword & 0x000000ff;
	}
}

#ifdef UNUSED_CODE
static void write_dqs_write_timing_control_registers(u16 *current_total_delay, u32 dev, u8 dct, u8 dimm, u32 index_reg)
{
	u8 lane;
	u32 dword;

	for (lane = 0; lane < MAX_BYTE_LANES; lane++) {
		u32 ret_reg;
		if ((lane == 0) || (lane == 1))
			ret_reg = 0x30;
		if ((lane == 2) || (lane == 3))
			ret_reg = 0x31;
		if ((lane == 4) || (lane == 5))
			ret_reg = 0x40;
		if ((lane == 6) || (lane == 7))
			ret_reg = 0x41;
		if (lane == 8)
			ret_reg = 0x32;
		ret_reg += dimm * 3;
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, ret_reg);
		if ((lane == 7) || (lane == 5) || (lane == 3) || (lane == 1)) {
			dword &= ~(0xff << 16);
			dword |= (current_total_delay[lane] & 0xff) << 16;
		}
		if ((lane == 8) || (lane == 6) || (lane == 4) || (lane == 2) || (lane == 0)) {
			dword &= ~0xff;
			dword |= current_total_delay[lane] & 0xff;
		}
		set_nb32_index_wait_dct(dev, dct, index_reg, ret_reg, dword);
	}
}
#endif

static void write_write_data_timing_control_registers(u16 *current_total_delay, u32 dev, u8 dct, u8 dimm, u32 index_reg)
{
	u8 lane;
	u32 dword;

	for (lane = 0; lane < MAX_BYTE_LANES; lane++) {
		u32 wdt_reg;

		/* Calculate Write Data Timing register location */
		if ((lane == 0) || (lane == 1) || (lane == 2) || (lane == 3))
			wdt_reg = 0x1;
		if ((lane == 4) || (lane == 5) || (lane == 6) || (lane == 7))
			wdt_reg = 0x2;
		if (lane == 8)
			wdt_reg = 0x3;
		wdt_reg |= (dimm << 8);

		/* Set Write Data Timing register values */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, wdt_reg);
		if ((lane == 7) || (lane == 3)) {
			dword &= ~(0x7f << 24);
			dword |= (current_total_delay[lane] & 0x7f) << 24;
		}
		if ((lane == 6) || (lane == 2)) {
			dword &= ~(0x7f << 16);
			dword |= (current_total_delay[lane] & 0x7f) << 16;
		}
		if ((lane == 5) || (lane == 1)) {
			dword &= ~(0x7f << 8);
			dword |= (current_total_delay[lane] & 0x7f) << 8;
		}
		if ((lane == 8) || (lane == 4) || (lane == 0)) {
			dword &= ~0x7f;
			dword |= current_total_delay[lane] & 0x7f;
		}
		set_nb32_index_wait_dct(dev, dct, index_reg, wdt_reg, dword);
	}
}

void read_dqs_receiver_enable_control_registers(u16 *current_total_delay, u32 dev, u8 dct, u8 dimm, u32 index_reg)
{
	u8 lane;
	u32 mask;
	u32 dword;

	if (is_fam15h())
		mask = 0x3ff;
	else
		mask = 0x1ff;

	for (lane = 0; lane < MAX_BYTE_LANES; lane++) {
		u32 ret_reg;
		if ((lane == 0) || (lane == 1))
			ret_reg = 0x10;
		if ((lane == 2) || (lane == 3))
			ret_reg = 0x11;
		if ((lane == 4) || (lane == 5))
			ret_reg = 0x20;
		if ((lane == 6) || (lane == 7))
			ret_reg = 0x21;
		if (lane == 8)
			ret_reg = 0x12;
		ret_reg += dimm * 3;
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, ret_reg);
		if ((lane == 7) || (lane == 5) || (lane == 3) || (lane == 1)) {
			current_total_delay[lane] = (dword & (mask << 16)) >> 16;
		}
		if ((lane == 8) || (lane == 6) || (lane == 4) || (lane == 2) || (lane == 0)) {
			current_total_delay[lane] = dword & mask;
		}
	}
}

void write_dqs_receiver_enable_control_registers(u16 *current_total_delay, u32 dev, u8 dct, u8 dimm, u32 index_reg)
{
	u8 lane;
	u32 mask;
	u32 dword;

	if (is_fam15h())
		mask = 0x3ff;
	else
		mask = 0x1ff;

	for (lane = 0; lane < MAX_BYTE_LANES; lane++) {
		u32 ret_reg;
		if ((lane == 0) || (lane == 1))
			ret_reg = 0x10;
		if ((lane == 2) || (lane == 3))
			ret_reg = 0x11;
		if ((lane == 4) || (lane == 5))
			ret_reg = 0x20;
		if ((lane == 6) || (lane == 7))
			ret_reg = 0x21;
		if (lane == 8)
			ret_reg = 0x12;
		ret_reg += dimm * 3;
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, ret_reg);
		if ((lane == 7) || (lane == 5) || (lane == 3) || (lane == 1)) {
			dword &= ~(mask << 16);
			dword |= (current_total_delay[lane] & mask) << 16;
		}
		if ((lane == 8) || (lane == 6) || (lane == 4) || (lane == 2) || (lane == 0)) {
			dword &= ~mask;
			dword |= current_total_delay[lane] & mask;
		}
		set_nb32_index_wait_dct(dev, dct, index_reg, ret_reg, dword);
	}
}

static void read_dram_phase_recovery_control_registers(u16 *current_total_delay, u32 dev, u8 dct, u8 dimm, u32 index_reg)
{
	u8 lane;
	u32 dword;

	for (lane = 0; lane < MAX_BYTE_LANES; lane++) {
		u32 prc_reg;

		/* Calculate DRAM Phase Recovery Control register location */
		if ((lane == 0) || (lane == 1) || (lane == 2) || (lane == 3))
			prc_reg = 0x50;
		if ((lane == 4) || (lane == 5) || (lane == 6) || (lane == 7))
			prc_reg = 0x51;
		if (lane == 8)
			prc_reg = 0x52;

		dword = get_nb32_index_wait_dct(dev, dct, index_reg, prc_reg);
		if ((lane == 7) || (lane == 3)) {
			current_total_delay[lane] = (dword >> 24) & 0x7f;
		}
		if ((lane == 6) || (lane == 2)) {
			current_total_delay[lane] = (dword >> 16) & 0x7f;
		}
		if ((lane == 5) || (lane == 1)) {
			current_total_delay[lane] = (dword >> 8) & 0x7f;
		}
		if ((lane == 8) || (lane == 4) || (lane == 0)) {
			current_total_delay[lane] = dword & 0x7f;
		}
	}
}

static void write_dram_phase_recovery_control_registers(u16 *current_total_delay, u32 dev, u8 dct, u8 dimm, u32 index_reg)
{
	u8 lane;
	u32 dword;

	for (lane = 0; lane < MAX_BYTE_LANES; lane++) {
		u32 prc_reg;

		/* Calculate DRAM Phase Recovery Control register location */
		if ((lane == 0) || (lane == 1) || (lane == 2) || (lane == 3))
			prc_reg = 0x50;
		if ((lane == 4) || (lane == 5) || (lane == 6) || (lane == 7))
			prc_reg = 0x51;
		if (lane == 8)
			prc_reg = 0x52;

		/* Set DRAM Phase Recovery Control register values */
		dword = get_nb32_index_wait_dct(dev, dct, index_reg, prc_reg);
		if ((lane == 7) || (lane == 3)) {
			dword &= ~(0x7f << 24);
			dword |= (current_total_delay[lane] & 0x7f) << 24;
		}
		if ((lane == 6) || (lane == 2)) {
			dword &= ~(0x7f << 16);
			dword |= (current_total_delay[lane] & 0x7f) << 16;
		}
		if ((lane == 5) || (lane == 1)) {
			dword &= ~(0x7f << 8);
			dword |= (current_total_delay[lane] & 0x7f) << 8;
		}
		if ((lane == 8) || (lane == 4) || (lane == 0)) {
			dword &= ~0x7f;
			dword |= current_total_delay[lane] & 0x7f;
		}
		set_nb32_index_wait_dct(dev, dct, index_reg, prc_reg, dword);
	}
}

void read_dqs_read_data_timing_registers(u16 *delay, u32 dev, u8 dct, u8 dimm, u32 index_reg)
{
	u8 shift;
	u32 dword;
	u32 mask;

	if (is_fam15h()) {
		mask = 0x3e;
		shift = 1;
	}
	else {
		mask = 0x3f;
		shift = 0;
	}

	/* Lanes 0 - 3 */
	dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x5 | (dimm << 8));
	delay[3] = ((dword >> 24) & mask) >> shift;
	delay[2] = ((dword >> 16) & mask) >> shift;
	delay[1] = ((dword >> 8) & mask) >> shift;
	delay[0] = (dword & mask) >> shift;

	/* Lanes 4 - 7 */
	dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x6 | (dimm << 8));
	delay[7] = ((dword >> 24) & mask) >> shift;
	delay[6] = ((dword >> 16) & mask) >> shift;
	delay[5] = ((dword >> 8) & mask) >> shift;
	delay[4] = (dword & mask) >> shift;

	/* Lane 8 (ECC) */
	dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x7 | (dimm << 8));
	delay[8] = (dword & mask) >> shift;
}

void write_dqs_read_data_timing_registers(u16 *delay, u32 dev, u8 dct, u8 dimm, u32 index_reg)
{
	u8 shift;
	u32 dword;
	u32 mask;

	if (is_fam15h()) {
		mask = 0x3e;
		shift = 1;
	}
	else {
		mask = 0x3f;
		shift = 0;
	}

	/* Lanes 0 - 3 */
	dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x5 | (dimm << 8));
	dword &= ~(mask << 24);
	dword &= ~(mask << 16);
	dword &= ~(mask << 8);
	dword &= ~mask;
	dword |= ((delay[3] << shift) & mask) << 24;
	dword |= ((delay[2] << shift) & mask) << 16;
	dword |= ((delay[1] << shift) & mask) << 8;
	dword |= (delay[0] << shift) & mask;
	set_nb32_index_wait_dct(dev, dct, index_reg, 0x5 | (dimm << 8), dword);

	/* Lanes 4 - 7 */
	dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x6 | (dimm << 8));
	dword &= ~(mask << 24);
	dword &= ~(mask << 16);
	dword &= ~(mask << 8);
	dword &= ~mask;
	dword |= ((delay[7] << shift) & mask) << 24;
	dword |= ((delay[6] << shift) & mask) << 16;
	dword |= ((delay[5] << shift) & mask) << 8;
	dword |= (delay[4] << shift) & mask;
	set_nb32_index_wait_dct(dev, dct, index_reg, 0x6 | (dimm << 8), dword);

	/* Lane 8 (ECC) */
	dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x7 | (dimm << 8));
	dword &= ~mask;
	dword |= (delay[8] << shift) & mask;
	set_nb32_index_wait_dct(dev, dct, index_reg, 0x7 | (dimm << 8), dword);
}

static u32 convert_testaddr_and_channel_to_address(struct DCTStatStruc *p_dct_stat, u32 test_addr, u8 channel)
{
	set_upper_fs_base(test_addr);
	test_addr <<= 8;

	if ((p_dct_stat->status & (1 << SB_128_BIT_MODE)) && channel) {
		test_addr += 8;	/* second channel */
	}

	return test_addr;
}

/* DQS receiver Enable Training (Family 10h)
 * Algorithm detailed in:
 * The Fam10h BKDG Rev. 3.62 section 2.8.9.9.2
 */
static void dqs_train_rcvr_en_sw_fam10(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 pass)
{
	u8 channel;
	u8 _2_ranks;
	u8 addl_index = 0;
	u8 receiver;
	u8 _disable_dram_ecc = 0, _wrap_32_dis = 0, _sse2 = 0;
	u16 ctlr_max_delay;
	u16 max_delay_ch[2];
	u32 test_addr_0, test_addr_1, test_addr_0_b, test_addr_1_b;
	u32 pattern_buffer[64 + 4]; /* FIXME: need increase 8? */
	u32 errors;

	u32 val;
	u32 reg;
	u32 dev;
	u32 index_reg;
	u32 ch_start, ch_end, ch;
	msr_t msr;
	CRx_TYPE cr4;

	u32 dword;
	u8 dimm;
	u8 rank;
	u8 lane;
	u16 current_total_delay[MAX_BYTE_LANES];
	u16 candidate_total_delay[8];
	u8 data_test_pass_sr[2][8];	/* [rank][lane] */
	u8 data_test_pass[8];		/* [lane] */
	u8 data_test_pass_prev[8];		/* [lane] */
	u8 window_det_toggle[8];
	u8 trained[8];
	u64 result_qword1;
	u64 result_qword2;

	u8 valid;

	print_debug_dqs("\nTrainRcvEn: node", p_dct_stat->node_id, 0);
	print_debug_dqs("TrainRcvEn: pass", pass, 0);

	dev = p_dct_stat->dev_dct;
	ch_start = 0;
	if (!p_dct_stat->ganged_mode) {
		ch_end = 2;
	} else {
		ch_end = 1;
	}

	for (ch = ch_start; ch < ch_end; ch++) {
		reg = 0x78;
		val = get_nb32_dct(dev, ch, reg);
		val &= ~(0x3ff << 22);
		val |= (0x0c8 << 22);		/* MaxRdLatency = 0xc8 */
		set_nb32_dct(dev, ch, reg, val);
	}

	if (pass == FIRST_PASS) {
		mct_init_dqs_pos_4_rcvr_en_d(p_mct_stat, p_dct_stat);
	} else {
		p_dct_stat->dimm_train_fail = 0;
		p_dct_stat->cs_train_fail = ~p_dct_stat->cs_present;
	}

	cr4 = read_cr4();
	if (cr4 & (1 << 9)) {	/* save the old value */
		_sse2 = 1;
	}
	cr4 |= (1 << 9);	/* OSFXSR enable SSE2 */
	write_cr4(cr4);

	msr = rdmsr(HWCR_MSR);
	/* FIXME: Why use SSEDIS */
	if (msr.lo & (1 << 17)) {	/* save the old value */
		_wrap_32_dis = 1;
	}
	msr.lo |= (1 << 17);	/* HWCR.wrap32dis */
	msr.lo &= ~(1 << 15);	/* SSEDIS */
	wrmsr(HWCR_MSR, msr);	/* Setting wrap32dis allows 64-bit memory
				   references in real mode */

	_disable_dram_ecc = mct_disable_dimm_ecc_en_d(p_mct_stat, p_dct_stat);

	setup_rcvr_pattern(p_mct_stat, p_dct_stat, pattern_buffer, pass);

	errors = 0;
	dev = p_dct_stat->dev_dct;

	for (channel = 0; channel < 2; channel++) {
		print_debug_dqs("\tTrainRcvEn51: node ", p_dct_stat->node_id, 1);
		print_debug_dqs("\tTrainRcvEn51: channel ", channel, 1);
		p_dct_stat->channel = channel;

		ctlr_max_delay = 0;
		max_delay_ch[channel] = 0;
		index_reg = 0x98;

		receiver = mct_init_receiver_d(p_dct_stat, channel);
		/* There are four receiver pairs, loosely associated with chipselects.
		 * This is essentially looping over each DIMM.
		 */
		for (; receiver < 8; receiver += 2) {
			addl_index = (receiver >> 1) * 3 + 0x10;
			dimm = (receiver >> 1);

			print_debug_dqs("\t\tTrainRcvEnd52: index ", addl_index, 2);

			if (!mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel, receiver)) {
				continue;
			}

			/* Clear data structures */
			for (lane = 0; lane < 8; lane++) {
				data_test_pass_prev[lane] = 0;
				trained[lane] = 0;
			}

			/* 2.8.9.9.2 (1, 6)
			 * Retrieve gross and fine timing fields from write DQS registers
			 */
			read_dqs_write_timing_control_registers(current_total_delay, dev, channel, dimm, index_reg);

			/* 2.8.9.9.2 (1)
			 * Program the Write Data Timing and Write ECC Timing register to
			 * the values stored in the DQS Write Timing Control register
			 * for each lane
			 */
			write_write_data_timing_control_registers(current_total_delay, dev, channel, dimm, index_reg);

			/* 2.8.9.9.2 (2)
			 * Program the Read DQS Timing Control and the Read DQS ECC Timing Control registers
			 * to 1/2 MEMCLK for all lanes
			 */
			for (lane = 0; lane < MAX_BYTE_LANES; lane++) {
				u32 rdt_reg;
				if ((lane == 0) || (lane == 1) || (lane == 2) || (lane == 3))
					rdt_reg = 0x5;
				if ((lane == 4) || (lane == 5) || (lane == 6) || (lane == 7))
					rdt_reg = 0x6;
				if (lane == 8)
					rdt_reg = 0x7;
				rdt_reg |= (dimm << 8);
				if (lane == 8)
					dword = 0x0000003f;
				else
					dword = 0x3f3f3f3f;
				set_nb32_index_wait_dct(dev, channel, index_reg, rdt_reg, dword);
			}

			/* 2.8.9.9.2 (3)
			 * Select two test addresses for each rank present
			 */
			test_addr_0 = mct_get_rcvr_sys_addr_d(p_mct_stat, p_dct_stat, channel, receiver, &valid);
			if (!valid) {	/* Address not supported on current CS */
				continue;
			}

			test_addr_0_b = test_addr_0 + (BIG_PAGE_X8_RJ8 << 3);

			if (mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel, receiver + 1)) {
				test_addr_1 = mct_get_rcvr_sys_addr_d(p_mct_stat, p_dct_stat, channel, receiver + 1, &valid);
				if (!valid) {	/* Address not supported on current CS */
					continue;
				}
				test_addr_1_b = test_addr_1 + (BIG_PAGE_X8_RJ8 << 3);
				_2_ranks = 1;
			} else {
				_2_ranks = test_addr_1 = test_addr_1_b = 0;
			}

			print_debug_dqs("\t\tTrainRcvEn53: test_addr_0 ", test_addr_0, 2);
			print_debug_dqs("\t\tTrainRcvEn53: test_addr_0_b ", test_addr_0_b, 2);
			print_debug_dqs("\t\tTrainRcvEn53: test_addr_1 ", test_addr_1, 2);
			print_debug_dqs("\t\tTrainRcvEn53: test_addr_1_b ", test_addr_1_b, 2);

			/* 2.8.9.9.2 (4, 5)
			 * Write 1 cache line of the appropriate test pattern to each test address
			 */
			mct_write_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_0, 0); /* rank 0 of DIMM, testpattern 0 */
			mct_write_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_0_b, 1); /* rank 0 of DIMM, testpattern 1 */
			if (_2_ranks) {
				mct_write_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_1, 0); /*rank 1 of DIMM, testpattern 0 */
				mct_write_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_1_b, 1); /*rank 1 of DIMM, testpattern 1 */
			}

#if DQS_TRAIN_DEBUG > 0
			for (lane = 0; lane < 8; lane++) {
				print_debug_dqs("\t\tTrainRcvEn54: lane: ", lane, 2);
				print_debug_dqs("\t\tTrainRcvEn54: current_total_delay ", current_total_delay[lane], 2);
			}
#endif

			/* 2.8.9.9.2 (6)
			 * Write gross and fine timing fields to read DQS registers
			 */
			write_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);

			/* 2.8.9.9.2 (7)
			 * Loop over all delay values up to 1 MEMCLK (0x40 delay steps) from the initial delay values
			 *
			 * FIXME
			 * It is not clear if training should be discontinued if any test failures occur in the first
			 * 1 MEMCLK window, or if it should be discontinued if no successes occur in the first 1 MEMCLK
			 * window.  Therefore, loop over up to 2 MEMCLK (0x80 delay steps) to be on the safe side.
			 */
			u16 current_delay_step;

			for (current_delay_step = 0; current_delay_step < 0x80; current_delay_step++) {
				print_debug_dqs("\t\t\tTrainRcvEn541: current_delay_step ", current_delay_step, 3);

				/* 2.8.9.9.2 (7 D)
				* Terminate if all lanes are trained
				*/
				u8 all_lanes_trained = 1;
				for (lane = 0; lane < 8; lane++)
					if (!trained[lane])
						all_lanes_trained = 0;

				if (all_lanes_trained)
					break;

				/* 2.8.9.9.2 (7 A)
				 * Loop over all ranks
				 */
				for (rank = 0; rank < (_2_ranks + 1); rank++) {
					/* 2.8.9.9.2 (7 A a-d)
					 * Read the first test address of the current rank
					 * Store the first data beat for analysis
					 * Reset read pointer in the DRAM controller FIFO
					 * Read the second test address of the current rank
					 * Store the first data beat for analysis
					 * Reset read pointer in the DRAM controller FIFO
					 */
					if (rank & 1) {
						/* 2.8.9.9.2 (7 D)
						 * Invert read instructions to alternate data read order on the bus
						 */
						proc_iocl_flush_d((rank == 0) ? test_addr_0_b : test_addr_1_b);
						result_qword2 = read64_fs(convert_testaddr_and_channel_to_address(p_dct_stat, (rank == 0) ? test_addr_0_b : test_addr_1_b, channel));
						write_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);
						proc_iocl_flush_d((rank == 0) ? test_addr_0 : test_addr_1);
						result_qword1 = read64_fs(convert_testaddr_and_channel_to_address(p_dct_stat, (rank == 0) ? test_addr_0 : test_addr_1, channel));
						write_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);
					} else {
						proc_iocl_flush_d((rank == 0) ? test_addr_0 : test_addr_1);
						result_qword1 = read64_fs(convert_testaddr_and_channel_to_address(p_dct_stat, (rank == 0) ? test_addr_0 : test_addr_1, channel));
						write_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);
						proc_iocl_flush_d((rank == 0) ? test_addr_0_b : test_addr_1_b);
						result_qword2 = read64_fs(convert_testaddr_and_channel_to_address(p_dct_stat, (rank == 0) ? test_addr_0_b : test_addr_1_b, channel));
						write_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);
					}
					/* 2.8.9.9.2 (7 A e)
					 * Compare both read patterns and flag passing ranks/lanes
					 */
					u8 result_lane_byte1;
					u8 result_lane_byte2;
					for (lane = 0; lane < 8; lane++) {
						if (trained[lane] == 1) {
#if DQS_TRAIN_DEBUG > 0
							print_debug_dqs("\t\t\t\t\t\t\t\t lane already trained: ", lane, 4);
#endif
							continue;
						}

						result_lane_byte1 = (result_qword1 >> (lane * 8)) & 0xff;
						result_lane_byte2 = (result_qword2 >> (lane * 8)) & 0xff;
						if ((result_lane_byte1 == 0x55) && (result_lane_byte2 == 0xaa))
							data_test_pass_sr[rank][lane] = 1;
						else
							data_test_pass_sr[rank][lane] = 0;
#if DQS_TRAIN_DEBUG > 0
						print_debug_dqs_pair("\t\t\t\t\t\t\t\t ", 0x55, "  |  ", result_lane_byte1, 4);
						print_debug_dqs_pair("\t\t\t\t\t\t\t\t ", 0xaa, "  |  ", result_lane_byte2, 4);
#endif
					}
				}

				/* 2.8.9.9.2 (7 B)
				 * If DIMM is dual rank, only use delays that pass testing for both ranks
				 */
				for (lane = 0; lane < 8; lane++) {
					if (_2_ranks) {
						if ((data_test_pass_sr[0][lane]) && (data_test_pass_sr[1][lane]))
							data_test_pass[lane] = 1;
						else
							data_test_pass[lane] = 0;
					} else {
						data_test_pass[lane] = data_test_pass_sr[0][lane];
					}
				}

				/* 2.8.9.9.2 (7 E)
				 * For each lane, update the DQS receiver delay setting in support of next iteration
				 */
				for (lane = 0; lane < 8; lane++) {
					if (trained[lane] == 1)
						continue;

					/* 2.8.9.9.2 (7 C a)
					 * Save the total delay of the first success after a failure for later use
					 */
					if ((data_test_pass[lane] == 1) && (data_test_pass_prev[lane] == 0)) {
						candidate_total_delay[lane] = current_total_delay[lane];
						window_det_toggle[lane] = 0;
					}

					/* 2.8.9.9.2 (7 C b)
					 * If the current delay failed testing add 1/8 UI to the current delay
					 */
					if (data_test_pass[lane] == 0)
						current_total_delay[lane] += 0x4;

					/* 2.8.9.9.2 (7 C c)
					 * If the current delay passed testing alternately add either 1/32 UI or 1/4 UI to the current delay
					 * If 1.25 UI of delay have been added with no failures the lane is considered trained
					 */
					if (data_test_pass[lane] == 1) {
						/* See if lane is trained */
						if ((current_total_delay[lane] - candidate_total_delay[lane]) >= 0x28) {
							trained[lane] = 1;

							/* Calculate and set final lane delay value
							 * The final delay is the candidate delay + 7/8 UI
							 */
							current_total_delay[lane] = candidate_total_delay[lane] + 0x1c;
						} else {
							if (window_det_toggle[lane] == 0) {
								current_total_delay[lane] += 0x1;
								window_det_toggle[lane] = 1;
							} else {
								current_total_delay[lane] += 0x8;
								window_det_toggle[lane] = 0;
							}
						}
					}
				}

				/* Update delays in hardware */
				write_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);

				/* Save previous results for comparison in the next iteration */
				for (lane = 0; lane < 8; lane++)
					data_test_pass_prev[lane] = data_test_pass[lane];
			}

#if DQS_TRAIN_DEBUG > 0
			for (lane = 0; lane < 8; lane++)
				print_debug_dqs_pair("\t\tTrainRcvEn55: Lane ", lane, " current_total_delay ", current_total_delay[lane], 2);
#endif

			/* Find highest delay value and save for later use */
			for (lane = 0; lane < 8; lane++)
				if (current_total_delay[lane] > ctlr_max_delay)
					ctlr_max_delay = current_total_delay[lane];

			/* See if any lanes failed training, and set error flags appropriately
			 * For all trained lanes, save delay values for later use
			 */
			for (lane = 0; lane < 8; lane++) {
				if (trained[lane]) {
					p_dct_stat->ch_d_b_rcvr_dly[channel][receiver >> 1][lane] = current_total_delay[lane];
				} else {
					printk(BIOS_WARNING, "TrainRcvrEn: WARNING: Lane %d of receiver %d on channel %d failed training!\n", lane, receiver, channel);

					/* Set error flags */
					p_dct_stat->err_status |= 1 << SB_NO_RCVR_EN;
					errors |= 1 << SB_NO_RCVR_EN;
					p_dct_stat->err_code = SC_FATAL_ERR;
					p_dct_stat->cs_train_fail |= 1 << receiver;
					p_dct_stat->dimm_train_fail |= 1 << (receiver + channel);
				}
			}

			/* 2.8.9.9.2 (8)
			 * Flush the receiver FIFO
			 * Write one full cache line of non-0x55/0xaa data to one of the test addresses, then read it back to flush the FIFO
			 */
			/* FIXME
			 * This does not seem to be needed, and has a tendency to lock up the
			 * boot process while attempting to write the test pattern.
			 */
		}
		max_delay_ch[channel] = ctlr_max_delay;
	}

	ctlr_max_delay = max_delay_ch[0];
	if (max_delay_ch[1] > ctlr_max_delay)
		ctlr_max_delay = max_delay_ch[1];

	for (channel = 0; channel < 2; channel++) {
		mct_set_max_latency_d(p_dct_stat, channel, ctlr_max_delay); /* program Ch A/B max_async_lat to correspond with max delay */
	}

	for (channel = 0; channel < 2; channel++) {
		reset_dct_wr_ptr_d(dev, channel, index_reg, addl_index);
	}

	if (_disable_dram_ecc) {
		mct_enable_dimm_ecc_en_d(p_mct_stat, p_dct_stat, _disable_dram_ecc);
	}

	if (pass == FIRST_PASS) {
		/*Disable DQSRcvrEn training mode */
		mct_disable_dqs_rcv_en_d(p_dct_stat);
	}

	if (!_wrap_32_dis) {
		msr = rdmsr(HWCR_MSR);
		msr.lo &= ~(1 << 17);	/* restore HWCR.wrap32dis */
		wrmsr(HWCR_MSR, msr);
	}
	if (!_sse2) {
		cr4 = read_cr4();
		cr4 &= ~(1 << 9);	/* restore cr4.OSFXSR */
		write_cr4(cr4);
	}

#if DQS_TRAIN_DEBUG > 0
	{
		u8 ChannelDTD;
		printk(BIOS_DEBUG, "TrainRcvrEn: ch_max_rd_lat:\n");
		for (ChannelDTD = 0; ChannelDTD < 2; ChannelDTD++) {
			printk(BIOS_DEBUG, "channel:%x: %x\n",
			       ChannelDTD, p_dct_stat->ch_max_rd_lat[ChannelDTD][0]);
		}
	}
#endif

#if DQS_TRAIN_DEBUG > 0
	{
		u16 valDTD;
		u8 ChannelDTD, ReceiverDTD;
		u8 i;
		u16 *p;

		printk(BIOS_DEBUG, "TrainRcvrEn: ch_d_b_rcvr_dly:\n");
		for (ChannelDTD = 0; ChannelDTD < 2; ChannelDTD++) {
			printk(BIOS_DEBUG, "channel:%x\n", ChannelDTD);
			for (ReceiverDTD = 0; ReceiverDTD < 8; ReceiverDTD+=2) {
				printk(BIOS_DEBUG, "\t\tReceiver:%x:", ReceiverDTD);
				p = p_dct_stat->ch_d_b_rcvr_dly[ChannelDTD][ReceiverDTD >> 1];
				for (i = 0; i < 8; i++) {
					valDTD = p[i];
					printk(BIOS_DEBUG, " %03x", valDTD);
				}
				printk(BIOS_DEBUG, "\n");
			}
		}
	}
#endif

	printk(BIOS_DEBUG, "TrainRcvrEn: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "TrainRcvrEn: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "TrainRcvrEn: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "TrainRcvrEn: Done\n\n");
}

/* DQS receiver Enable Training Pattern Generation (Family 15h)
 * Algorithm detailed in:
 * The Fam15h BKDG Rev. 3.14 section 2.10.5.8.2 (4)
 */
static void generate_dram_receiver_enable_training_pattern_fam15(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct, u8 receiver)
{
	u32 dword;
	u32 dev = p_dct_stat->dev_dct;

	/* 2.10.5.7.1.1
	 * It appears that the DCT only supports 8-beat burst length mode,
	 * so do nothing here...
	 */

	/* Wait for CmdSendInProg == 0 */
	do {
		dword = get_nb32_dct(dev, dct, 0x250);
	} while (dword & (0x1 << 12));

	/* Set CmdTestEnable = 1 */
	dword = get_nb32_dct(dev, dct, 0x250);
	dword |= (0x1 << 2);
	set_nb32_dct(dev, dct, 0x250, dword);

	/* 2.10.5.8.6.1.1 Send Activate Command */
	dword = get_nb32_dct(dev, dct, 0x28c);
	dword &= ~(0xff << 22);				/* CmdChipSelect = receiver */
	dword |= ((0x1 << receiver) << 22);
	dword &= ~(0x7 << 19);				/* CmdBank = 0 */
	dword &= ~(0x3ffff);				/* CmdAddress = 0 */
	dword |= (0x1 << 31);				/* SendActCmd = 1 */
	set_nb32_dct(dev, dct, 0x28c, dword);

	/* Wait for SendActCmd == 0 */
	do {
		dword = get_nb32_dct(dev, dct, 0x28c);
	} while (dword & (0x1 << 31));

	/* Wait 75 MEMCLKs. */
	precise_memclk_delay_fam15(p_mct_stat, p_dct_stat, dct, 75);

	/* 2.10.5.8.6.1.2 */
	set_nb32_dct(dev, dct, 0x274, 0x0);		/* DQMask = 0 */
	set_nb32_dct(dev, dct, 0x278, 0x0);

	dword = get_nb32_dct(dev, dct, 0x27c);
	dword &= ~(0xff);				/* EccMask = 0 */
	if (p_dct_stat->dimm_ecc_present == 0)
		dword |= 0xff;				/* EccMask = 0xff */
	set_nb32_dct(dev, dct, 0x27c, dword);

	/* 2.10.5.8.6.1.2 */
	dword = get_nb32_dct(dev, dct, 0x270);
	dword &= ~(0x7ffff);				/* DataPrbsSeed = 55555 */
	dword |= (0x44443);				/* Use AGESA seed */
	set_nb32_dct(dev, dct, 0x270, dword);

	/* 2.10.5.8.2 (4) */
	dword = get_nb32_dct(dev, dct, 0x260);
	dword &= ~(0x1fffff);				/* CmdCount = 192 */
	dword |= 192;
	set_nb32_dct(dev, dct, 0x260, dword);

	/* Configure Target A */
	dword = get_nb32_dct(dev, dct, 0x254);
	dword &= ~(0x7 << 24);				/* TgtChipSelect = receiver */
	dword |= (receiver & 0x7) << 24;
	dword &= ~(0x7 << 21);				/* TgtBank = 0 */
	dword &= ~(0x3ff);				/* TgtAddress = 0 */
	set_nb32_dct(dev, dct, 0x254, dword);

	dword = get_nb32_dct(dev, dct, 0x250);
	dword |= (0x1 << 3);				/* ResetAllErr = 1 */
	dword &= ~(0x1 << 4);				/* StopOnErr = 0 */
	dword &= ~(0x3 << 8);				/* CmdTgt = 0 (Target A) */
	dword &= ~(0x7 << 5);				/* CmdType = 0 (Read) */
	dword |= (0x1 << 11);				/* SendCmd = 1 */
	set_nb32_dct(dev, dct, 0x250, dword);

	/* 2.10.5.8.6.1.2 Wait for TestStatus == 1 and CmdSendInProg == 0 */
	do {
		dword = get_nb32_dct(dev, dct, 0x250);
	} while ((dword & (0x1 << 12)) || (!(dword & (0x1 << 10))));

	dword = get_nb32_dct(dev, dct, 0x250);
	dword &= ~(0x1 << 11);				/* SendCmd = 0 */
	set_nb32_dct(dev, dct, 0x250, dword);

	/* 2.10.5.8.6.1.1 Send Precharge Command */
	/* Wait 25 MEMCLKs. */
	precise_memclk_delay_fam15(p_mct_stat, p_dct_stat, dct, 25);

	dword = get_nb32_dct(dev, dct, 0x28c);
	dword &= ~(0xff << 22);				/* CmdChipSelect = receiver */
	dword |= ((0x1 << receiver) << 22);
	dword &= ~(0x7 << 19);				/* CmdBank = 0 */
	dword &= ~(0x3ffff);				/* CmdAddress = 0x400 */
	dword |= 0x400;
	dword |= (0x1 << 30);				/* SendPchgCmd = 1 */
	set_nb32_dct(dev, dct, 0x28c, dword);

	/* Wait for SendPchgCmd == 0 */
	do {
		dword = get_nb32_dct(dev, dct, 0x28c);
	} while (dword & (0x1 << 30));

	/* Wait 25 MEMCLKs. */
	precise_memclk_delay_fam15(p_mct_stat, p_dct_stat, dct, 25);

	/* Set CmdTestEnable = 0 */
	dword = get_nb32_dct(dev, dct, 0x250);
	dword &= ~(0x1 << 2);
	set_nb32_dct(dev, dct, 0x250, dword);
}

/* DQS receiver Enable Training (Family 15h)
 * Algorithm detailed in:
 * The Fam15h BKDG Rev. 3.14 section 2.10.5.8.2
 * This algorithm runs once at the lowest supported MEMCLK,
 * then once again at the highest supported MEMCLK.
 */
static void dqs_train_rcvr_en_sw_fam15(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 pass)
{
	u8 channel;
	u8 _2_ranks;
	u8 addl_index = 0;
	u8 receiver;
	u8 _disable_dram_ecc = 0, _wrap_32_dis = 0, _sse2 = 0;
	u32 errors;

	u32 val;
	u32 dev;
	u32 index_reg;
	u32 ch_start, ch_end, ch;
	u32 msr;
	CRx_TYPE cr4;
	u32 lo, hi;

	u32 dword;
	u8 dimm;
	u8 rank;
	u8 lane;
	u8 nibble;
	u8 mem_clk;
	u16 min_mem_clk;
	u16 initial_seed;
	u8 train_both_nibbles;
	u16 current_total_delay[MAX_BYTE_LANES];
	u16 nibble0_current_total_delay[MAX_BYTE_LANES];
	u16 dqs_ret_pass1_total_delay[MAX_BYTE_LANES];
	u16 rank0_current_total_delay[MAX_BYTE_LANES];
	u16 phase_recovery_delays[MAX_BYTE_LANES];
	u16 seed[MAX_BYTE_LANES];
	u16 seed_gross[MAX_BYTE_LANES];
	u16 seed_fine[MAX_BYTE_LANES];
	u16 seed_pre_gross[MAX_BYTE_LANES];

	u8 package_type = mct_get_nv_bits(NV_PACK_TYPE);
	u16 fam15h_freq_tab[] = {0, 0, 0, 0, 333, 0, 400, 0, 0, 0, 533, 0, 0, 0, 667, 0, 0, 0, 800, 0, 0, 0, 933};

	u8 lane_count;
	lane_count = get_available_lane_count(p_mct_stat, p_dct_stat);

	print_debug_dqs("\nTrainRcvEn: node", p_dct_stat->node_id, 0);
	print_debug_dqs("TrainRcvEn: pass", pass, 0);

	min_mem_clk = mct_get_nv_bits(NV_MIN_MEMCLK);

	train_both_nibbles = 0;
	if (p_dct_stat->dimm_x4_present)
		if (is_fam15h())
			train_both_nibbles = 1;

	dev = p_dct_stat->dev_dct;
	index_reg = 0x98;
	ch_start = 0;
	ch_end = 2;

	for (ch = ch_start; ch < ch_end; ch++) {
		u8 max_rd_latency = 0x55;
		u8 p_state;

		/* 2.10.5.6 */
		fam_15_enable_training_mode(p_mct_stat, p_dct_stat, ch, 1);

		/* 2.10.5.2 */
		for (p_state = 0; p_state < 3; p_state++) {
			val = get_nb32_dct_nb_pstate(dev, ch, p_state, 0x210);
			val &= ~(0x3ff << 22);			/* MaxRdLatency = max_rd_latency */
			val |= (max_rd_latency & 0x3ff) << 22;
			set_nb32_dct_nb_pstate(dev, ch, p_state, 0x210, val);
		}
	}

	if (pass != FIRST_PASS) {
		p_dct_stat->dimm_train_fail = 0;
		p_dct_stat->cs_train_fail = ~p_dct_stat->cs_present;
	}

	cr4 = read_cr4();
	if (cr4 & (1 << 9)) {	/* save the old value */
		_sse2 = 1;
	}
	cr4 |= (1 << 9);	/* OSFXSR enable SSE2 */
	write_cr4(cr4);

	msr = HWCR_MSR;
	_rdmsr(msr, &lo, &hi);
	/* FIXME: Why use SSEDIS */
	if (lo & (1 << 17)) {	/* save the old value */
		_wrap_32_dis = 1;
	}
	lo |= (1 << 17);	/* HWCR.wrap32dis */
	lo &= ~(1 << 15);	/* SSEDIS */
	_wrmsr(msr, lo, hi);	/* Setting wrap32dis allows 64-bit memory references in real mode */

	_disable_dram_ecc = mct_disable_dimm_ecc_en_d(p_mct_stat, p_dct_stat);

	errors = 0;
	dev = p_dct_stat->dev_dct;

	for (channel = 0; channel < 2; channel++) {
		print_debug_dqs("\tTrainRcvEn51: node ", p_dct_stat->node_id, 1);
		print_debug_dqs("\tTrainRcvEn51: channel ", channel, 1);
		p_dct_stat->channel = channel;

		mem_clk = get_nb32_dct(dev, channel, 0x94) & 0x1f;

		receiver = mct_init_receiver_d(p_dct_stat, channel);
		/* There are four receiver pairs, loosely associated with chipselects.
		 * This is essentially looping over each DIMM.
		 */
		for (; receiver < 8; receiver += 2) {
			addl_index = (receiver >> 1) * 3 + 0x10;
			dimm = (receiver >> 1);

			print_debug_dqs("\t\tTrainRcvEnd52: index ", addl_index, 2);

			if (!mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel, receiver)) {
				continue;
			}

			/* Retrieve the total delay values from pass 1 of DQS receiver enable training */
			if (pass != FIRST_PASS) {
				read_dqs_receiver_enable_control_registers(dqs_ret_pass1_total_delay, dev, channel, dimm, index_reg);
			}

			/* 2.10.5.8.2
			 * Loop over all ranks
			 */
			if (mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel, receiver + 1))
				_2_ranks = 1;
			else
				_2_ranks = 0;
			for (rank = 0; rank < (_2_ranks + 1); rank++) {
				for (nibble = 0; nibble < (train_both_nibbles + 1); nibble++) {
					/* 2.10.5.8.2 (1)
					 * Specify the target DIMM and nibble to be trained
					 */
					dword = get_nb32_index_wait_dct(dev, channel, index_reg, 0x00000008);
					dword &= ~(0x3 << 4);		/* TrDimmSel = dimm */
					dword |= ((dimm & 0x3) << 4);
					dword &= ~(0x1 << 2);		/* TR_NIBBLE_SEL = nibble */
					dword |= ((nibble & 0x1) << 2);
					set_nb32_index_wait_dct(dev, channel, index_reg, 0x00000008, dword);

					/* 2.10.5.8.2 (2)
					 * Retrieve gross and fine timing fields from write DQS registers
					 */
					read_dqs_write_timing_control_registers(current_total_delay, dev, channel, dimm, index_reg);

					/* 2.10.5.8.2.1
					 * Generate the DQS receiver Enable Training Seed Values
					 */
					if (pass == FIRST_PASS) {
						initial_seed = fam15_receiver_enable_training_seed(p_dct_stat, channel, dimm, rank, package_type);

						/* Adjust seed for the minimum platform supported frequency */
						initial_seed = (u16) (((((u64) initial_seed) *
							fam15h_freq_tab[mem_clk] * 100) / (min_mem_clk * 100)));

						for (lane = 0; lane < lane_count; lane++) {
							u16 wl_pass1_delay;
							wl_pass1_delay = current_total_delay[lane];

							seed[lane] = initial_seed + wl_pass1_delay;
						}
					} else {
						u8 addr_prelaunch = 0;		/* TODO: Fetch the correct value from RC2[0] */
						u16 register_delay;
						s16 seed_prescaling;

						memcpy(current_total_delay, dqs_ret_pass1_total_delay, sizeof(current_total_delay));
						if ((p_dct_stat->status & (1 << SB_REGISTERED))) {
							if (addr_prelaunch)
								register_delay = 0x30;
							else
								register_delay = 0x20;
						} else if ((p_dct_stat->status & (1 << SB_LOAD_REDUCED))) {
							/* TODO
							 * Load reduced DIMM support unimplemented
							 */
							register_delay = 0x0;
						} else {
							register_delay = 0x0;
						}

						for (lane = 0; lane < lane_count; lane++) {
							seed_prescaling = current_total_delay[lane] - register_delay - 0x20;
							seed[lane] = (u16) (register_delay + ((((u64) seed_prescaling) * fam15h_freq_tab[mem_clk] * 100) / (min_mem_clk * 100)));
						}
					}

					for (lane = 0; lane < lane_count; lane++) {
						seed_gross[lane] = (seed[lane] >> 5) & 0x1f;
						seed_fine[lane] = seed[lane] & 0x1f;

						/*if (seed_gross[lane] == 0)
							seed_pre_gross[lane] = 0;
						else */if (seed_gross[lane] & 0x1)
							seed_pre_gross[lane] = 1;
						else
							seed_pre_gross[lane] = 2;

						/* Calculate phase recovery delays */
						phase_recovery_delays[lane] = ((seed_pre_gross[lane] & 0x1f) << 5) | (seed_fine[lane] & 0x1f);

						/* Set the gross delay.
						* NOTE: While the BKDG states to only program DqsRcvEnGrossDelay, this appears
						* to have been a misprint as DqsRcvEnFineDelay should be set to zero as well.
						*/
						current_total_delay[lane] = ((seed_gross[lane] & 0x1f) << 5);
					}

					/* 2.10.5.8.2 (2) / 2.10.5.8.2.1 (5 6)
					 * Program PhRecFineDly and PhRecGrossDly
					 */
					write_dram_phase_recovery_control_registers(phase_recovery_delays, dev, channel, dimm, index_reg);

					/* 2.10.5.8.2 (2) / 2.10.5.8.2.1 (7)
					 * Program the DQS receiver Enable delay values for each lane
					 */
					write_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);

					/* 2.10.5.8.2 (3)
					 * Program DQS_RCV_TR_EN = 1
					 */
					dword = get_nb32_index_wait_dct(dev, channel, index_reg, 0x00000008);
					dword |= (0x1 << 13);
					set_nb32_index_wait_dct(dev, channel, index_reg, 0x00000008, dword);

					/* 2.10.5.8.2 (4)
					 * Issue 192 read requests to the target rank
					 */
					generate_dram_receiver_enable_training_pattern_fam15(p_mct_stat, p_dct_stat, channel, receiver + (rank & 0x1));

					/* 2.10.5.8.2 (5)
					 * Program DQS_RCV_TR_EN = 0
					 */
					dword = get_nb32_index_wait_dct(dev, channel, index_reg, 0x00000008);
					dword &= ~(0x1 << 13);
					set_nb32_index_wait_dct(dev, channel, index_reg, 0x00000008, dword);

					/* 2.10.5.8.2 (6)
					 * Read PhRecGrossDly, PhRecFineDly
					 */
					read_dram_phase_recovery_control_registers(phase_recovery_delays, dev, channel, dimm, index_reg);

					/* 2.10.5.8.2 (7)
					 * Calculate and program the DQS receiver Enable delay values
					 */
					for (lane = 0; lane < lane_count; lane++) {
						current_total_delay[lane] = (phase_recovery_delays[lane] & 0x1f);
						current_total_delay[lane] |= ((seed_gross[lane] + ((phase_recovery_delays[lane] >> 5) & 0x1f) - seed_pre_gross[lane] + 1) << 5);
						if (nibble == 1) {
							/* 2.10.5.8.2 (1)
							 * Average the trained values of both nibbles on x4 DIMMs
							 */
							current_total_delay[lane] = (nibble0_current_total_delay[lane] + current_total_delay[lane]) / 2;
						}
					}

#if DQS_TRAIN_DEBUG > 1
					for (lane = 0; lane < 8; lane++)
						printk(BIOS_DEBUG, "\t\tTrainRcvEn55: channel: %d dimm: %d nibble: %d lane %d current_total_delay: %04x ch_d_b_rcvr_dly: %04x\n",
							channel, dimm, nibble, lane, current_total_delay[lane], p_dct_stat->ch_d_b_rcvr_dly[channel][dimm][lane]);
#endif
					write_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);

					if (nibble == 0) {
						/* Back up the Nibble 0 delays for later use */
						memcpy(nibble0_current_total_delay, current_total_delay, sizeof(current_total_delay));
					}

					/* Exit nibble training if current DIMM is not x4 */
					if ((p_dct_stat->dimm_x4_present & (1 << (dimm + channel))) == 0)
						break;
				}

				if (_2_ranks) {
					if (rank == 0) {
						/* Back up the Rank 0 delays for later use */
						memcpy(rank0_current_total_delay, current_total_delay, sizeof(current_total_delay));
					}
					if (rank == 1) {
						/* 2.10.5.8.2 (8)
						 * Compute the average delay across both ranks and program the result into
						 * the DQS receiver Enable delay registers
						 */
						for (lane = 0; lane < lane_count; lane++) {
							current_total_delay[lane] = (rank0_current_total_delay[lane] + current_total_delay[lane]) / 2;
							if (lane == 8)
								p_dct_stat->ch_d_bc_rcvr_dly[channel][dimm] = current_total_delay[lane];
							else
								p_dct_stat->ch_d_b_rcvr_dly[channel][dimm][lane] = current_total_delay[lane];
						}
						write_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);
					}
				} else {
					/* Save the current delay for later use by other routines */
					for (lane = 0; lane < lane_count; lane++) {
						if (lane == 8)
							p_dct_stat->ch_d_bc_rcvr_dly[channel][dimm] = current_total_delay[lane];
						else
							p_dct_stat->ch_d_b_rcvr_dly[channel][dimm][lane] = current_total_delay[lane];
					}
				}
			}

#if DQS_TRAIN_DEBUG > 0
			for (lane = 0; lane < 8; lane++)
				print_debug_dqs_pair("\t\tTrainRcvEn56: Lane ", lane, " current_total_delay ", current_total_delay[lane], 2);
#endif
		}
	}

	/* Calculate and program MaxRdLatency for both channels */
	calc_set_max_rd_latency_d_fam15(p_mct_stat, p_dct_stat, 0, 0);
	calc_set_max_rd_latency_d_fam15(p_mct_stat, p_dct_stat, 1, 0);

	if (_disable_dram_ecc) {
		mct_enable_dimm_ecc_en_d(p_mct_stat, p_dct_stat, _disable_dram_ecc);
	}

	if (pass == FIRST_PASS) {
		/*Disable DQSRcvrEn training mode */
		mct_disable_dqs_rcv_en_d(p_dct_stat);
	}

	if (!_wrap_32_dis) {
		msr = HWCR_MSR;
		_rdmsr(msr, &lo, &hi);
		lo &= ~(1 << 17);	/* restore HWCR.wrap32dis */
		_wrmsr(msr, lo, hi);
	}
	if (!_sse2) {
		cr4 = read_cr4();
		cr4 &= ~(1 << 9);	/* restore cr4.OSFXSR */
		write_cr4(cr4);
	}

#if DQS_TRAIN_DEBUG > 0
	{
		u8 ChannelDTD;
		printk(BIOS_DEBUG, "TrainRcvrEn: ch_max_rd_lat:\n");
		for (ChannelDTD = 0; ChannelDTD < 2; ChannelDTD++) {
			printk(BIOS_DEBUG, "channel:%x: %x\n",
			       ChannelDTD, p_dct_stat->ch_max_rd_lat[ChannelDTD][0]);
		}
	}
#endif

#if DQS_TRAIN_DEBUG > 0
	{
		u16 valDTD;
		u8 ChannelDTD, ReceiverDTD;
		u8 i;
		u16 *p;

		printk(BIOS_DEBUG, "TrainRcvrEn: ch_d_b_rcvr_dly:\n");
		for (ChannelDTD = 0; ChannelDTD < 2; ChannelDTD++) {
			printk(BIOS_DEBUG, "channel:%x\n", ChannelDTD);
			for (ReceiverDTD = 0; ReceiverDTD < 8; ReceiverDTD+=2) {
				printk(BIOS_DEBUG, "\t\tReceiver:%x:", ReceiverDTD);
				p = p_dct_stat->ch_d_b_rcvr_dly[ChannelDTD][ReceiverDTD >> 1];
				for (i = 0; i < 8; i++) {
					valDTD = p[i];
					printk(BIOS_DEBUG, " %03x", valDTD);
				}
				printk(BIOS_DEBUG, "\n");
			}
		}
	}
#endif

	printk(BIOS_DEBUG, "TrainRcvrEn: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "TrainRcvrEn: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "TrainRcvrEn: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "TrainRcvrEn: Done\n\n");
}

static void write_max_read_latency_to_registers(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct, u16 *latency)
{
	u32 dword;
	u8 nb_pstate;

	for (nb_pstate = 0; nb_pstate < 2; nb_pstate++) {
		dword = get_nb32_dct_nb_pstate(p_dct_stat->dev_dct, dct, nb_pstate, 0x210);
		dword &= ~(0x3ff << 22);
		dword |= ((latency[nb_pstate] & 0x3ff) << 22);
		set_nb32_dct_nb_pstate(p_dct_stat->dev_dct, dct, nb_pstate, 0x210, dword);
	}
}

/* DQS MaxRdLatency Training (Family 15h)
 * Algorithm detailed in:
 * The Fam15h BKDG Rev. 3.14 section 2.10.5.8.5.1
 * This algorithm runs at the highest supported MEMCLK.
 */
void dqs_train_max_rd_latency_sw_fam15(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u8 channel;
	u8 receiver;
	u8 _disable_dram_ecc = 0, _wrap_32_dis = 0, _sse2 = 0;
	u32 errors;

	u32 dev;
	u32 index_reg;
	u32 ch_start, ch_end;
	u32 msr;
	CRx_TYPE cr4;
	u32 lo, hi;

	u32 dword;
	u8 dimm;
	u8 lane;
	u8 mem_clk;
	u32 nb_clk;
	u8 nb_pstate;
	u16 current_total_delay[MAX_BYTE_LANES];
	u16 current_rdqs_total_delay[MAX_BYTE_LANES];
	u8 current_worst_case_total_delay_dimm;
	u16 current_worst_case_total_delay_value;

	u8 lane_count;
	lane_count = get_available_lane_count(p_mct_stat, p_dct_stat);

	u16 fam15h_freq_tab[] = {0, 0, 0, 0, 333, 0, 400, 0, 0, 0, 533, 0, 0, 0, 667, 0, 0, 0, 800, 0, 0, 0, 933};

	print_debug_dqs("\nTrainMaxRdLatency: node", p_dct_stat->node_id, 0);

	dev = p_dct_stat->dev_dct;
	index_reg = 0x98;
	ch_start = 0;
	ch_end = 2;

	cr4 = read_cr4();
	if (cr4 & (1 << 9)) {	/* save the old value */
		_sse2 = 1;
	}
	cr4 |= (1 << 9);	/* OSFXSR enable SSE2 */
	write_cr4(cr4);

	msr = HWCR_MSR;
	_rdmsr(msr, &lo, &hi);
	/* FIXME: Why use SSEDIS */
	if (lo & (1 << 17)) {	/* save the old value */
		_wrap_32_dis = 1;
	}
	lo |= (1 << 17);	/* HWCR.wrap32dis */
	lo &= ~(1 << 15);	/* SSEDIS */
	_wrmsr(msr, lo, hi);	/* Setting wrap32dis allows 64-bit memory references in real mode */

	_disable_dram_ecc = mct_disable_dimm_ecc_en_d(p_mct_stat, p_dct_stat);

	errors = 0;
	dev = p_dct_stat->dev_dct;

	for (channel = 0; channel < 2; channel++) {
		print_debug_dqs("\tTrainMaxRdLatency51: node ", p_dct_stat->node_id, 1);
		print_debug_dqs("\tTrainMaxRdLatency51: channel ", channel, 1);
		p_dct_stat->channel = channel;

		if (p_dct_stat->dimm_valid_dct[channel] == 0)
			continue;

		mem_clk = get_nb32_dct(dev, channel, 0x94) & 0x1f;

		receiver = mct_init_receiver_d(p_dct_stat, channel);

		/* Find DIMM with worst case receiver enable delays */
		current_worst_case_total_delay_dimm = 0;
		current_worst_case_total_delay_value = 0;

		/* There are four receiver pairs, loosely associated with chipselects.
		 * This is essentially looping over each DIMM.
		 */
		for (; receiver < 8; receiver += 2) {
			dimm = (receiver >> 1);

			print_debug_dqs("\t\tTrainMaxRdLatency52: receiver ", receiver, 2);

			if (!mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel, receiver)) {
				continue;
			}

			/* Retrieve the total delay values from pass 1 of DQS receiver enable training */
			read_dqs_receiver_enable_control_registers(current_total_delay, dev, channel, dimm, index_reg);
			read_dqs_read_data_timing_registers(current_rdqs_total_delay, dev, channel, dimm, index_reg);

			for (lane = 0; lane < lane_count; lane++) {
				current_total_delay[lane] += current_rdqs_total_delay[lane];
				if (current_total_delay[lane] > current_worst_case_total_delay_value) {
					current_worst_case_total_delay_dimm = dimm;
					current_worst_case_total_delay_value = current_total_delay[lane];
				}
			}

#if DQS_TRAIN_DEBUG > 0
			for (lane = 0; lane < lane_count; lane++)
				print_debug_dqs_pair("\t\tTrainMaxRdLatency56: Lane ", lane, " current_total_delay ", current_total_delay[lane], 2);
#endif
		}

		/* 2.10.5.8.5.1.1 */
		calc_set_max_rd_latency_d_fam15(p_mct_stat, p_dct_stat, channel, 1);

		/* 2.10.5.8.5.1.[2,3]
		 * Write the DRAM training pattern to the test address
		 */
		write_dram_dqs_training_pattern_fam15(p_mct_stat, p_dct_stat, channel, current_worst_case_total_delay_dimm << 1, 0xff, 0);

		/* 2.10.5.8.5.1.4
		 * Incrementally test each MaxRdLatency candidate
		 */
		for (; p_dct_stat->ch_max_rd_lat[channel][0] < 0x3ff; p_dct_stat->ch_max_rd_lat[channel][0]++) {
			write_max_read_latency_to_registers(p_mct_stat, p_dct_stat, channel, p_dct_stat->ch_max_rd_lat[channel]);
			read_dram_dqs_training_pattern_fam15(p_mct_stat, p_dct_stat, channel, current_worst_case_total_delay_dimm << 1, 0xff, 0);
			dword = get_nb32_dct(dev, channel, 0x268) & 0x3ffff;
			if (!dword)
				break;
			set_nb32_index_wait_dct(dev, channel, index_reg, 0x00000050, 0x13131313);
		}
		dword = get_nb32_dct(dev, channel, 0x268) & 0x3ffff;
		if (dword)
			printk(BIOS_ERR, "WARNING: MaxRdLatency training FAILED!  Attempting to continue but your system may be unstable...\n");

		/* 2.10.5.8.5.1.5 */
		nb_pstate = 0;
		mem_clk = get_nb32_dct(dev, channel, 0x94) & 0x1f;
		if (fam15h_freq_tab[mem_clk] == 0) {
			return;
		}
		dword = get_nb32(p_dct_stat->dev_nbctl, (0x160 + (nb_pstate * 4)));		/* Retrieve NbDid, NbFid */
		nb_clk = (200 * (((dword >> 1) & 0x1f) + 0x4)) / (((dword >> 7) & 0x1) ? 2 : 1);

		p_dct_stat->ch_max_rd_lat[channel][0]++;
		p_dct_stat->ch_max_rd_lat[channel][0] += ((((u64)15 * 100000000000ULL) / ((u64)fam15h_freq_tab[mem_clk] * 1000000ULL))
							 * ((u64)nb_clk * 1000)) / 1000000000ULL;

		write_max_read_latency_to_registers(p_mct_stat, p_dct_stat, channel, p_dct_stat->ch_max_rd_lat[channel]);
	}

	if (_disable_dram_ecc) {
		mct_enable_dimm_ecc_en_d(p_mct_stat, p_dct_stat, _disable_dram_ecc);
	}

	if (!_wrap_32_dis) {
		msr = HWCR_MSR;
		_rdmsr(msr, &lo, &hi);
		lo &= ~(1 << 17);	/* restore HWCR.wrap32dis */
		_wrmsr(msr, lo, hi);
	}
	if (!_sse2) {
		cr4 = read_cr4();
		cr4 &= ~(1 << 9);	/* restore cr4.OSFXSR */
		write_cr4(cr4);
	}

#if DQS_TRAIN_DEBUG > 0
	{
		u8 ChannelDTD;
		printk(BIOS_DEBUG, "TrainMaxRdLatency: ch_max_rd_lat:\n");
		for (ChannelDTD = 0; ChannelDTD < 2; ChannelDTD++) {
			printk(BIOS_DEBUG, "channel:%x: %x\n",
			       ChannelDTD, p_dct_stat->ch_max_rd_lat[ChannelDTD][0]);
		}
	}
#endif

	printk(BIOS_DEBUG, "TrainMaxRdLatency: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "TrainMaxRdLatency: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "TrainMaxRdLatency: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "TrainMaxRdLatency: Done\n\n");
}

u8 mct_init_receiver_d(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	if (p_dct_stat->dimm_valid_dct[dct] == 0) {
		return 8;
	} else {
		return 0;
	}
}

static void mct_disable_dqs_rcv_en_d(struct DCTStatStruc *p_dct_stat)
{
	u8 ch_end, ch;
	u32 reg;
	u32 dev;
	u32 val;

	dev = p_dct_stat->dev_dct;
	if (p_dct_stat->ganged_mode) {
		ch_end = 1;
	} else {
		ch_end = 2;
	}

	for (ch = 0; ch < ch_end; ch++) {
		reg = 0x78;
		val = get_nb32_dct(dev, ch, reg);
		val &= ~(1 << DQS_RCV_EN_TRAIN);
		set_nb32_dct(dev, ch, reg, val);
	}
}

/* mct_ModifyIndex_D
 * Function only used once so it was inlined.
 */

/* mct_GetInitFlag_D
 * Function only used once so it was inlined.
 */

/* Set F2x[1, 0]9C_x[2B:10] DRAM DQS receiver Enable Timing Control Registers
 * See BKDG Rev. 3.62 page 268 for more information
 */
void mct_set_rcvr_en_dly_d(struct DCTStatStruc *p_dct_stat, u16 RcvrEnDly,
			u8 FinalValue, u8 channel, u8 receiver, u32 dev,
			u32 index_reg, u8 addl_index, u8 pass)
{
	u32 index;
	u8 i;
	u16 *p;
	u32 val;

	if (RcvrEnDly == 0x1fe) {
		/*set the boundary flag */
		p_dct_stat->status |= 1 << SB_DQS_RCV_LIMIT;
	}

	/* DimmOffset not needed for ch_d_b_rcvr_dly array */
	for (i = 0; i < 8; i++) {
		if (FinalValue) {
			/*calculate dimm offset */
			p = p_dct_stat->ch_d_b_rcvr_dly[channel][receiver >> 1];
			RcvrEnDly = p[i];
		}

		/* if flag = 0, set DqsRcvEn value to reg. */
		/* get the register index from table */
		index = table_dqs_rcv_en_offset[i >> 1];
		index += addl_index;	/* DIMMx DqsRcvEn byte0 */
		val = get_nb32_index_wait_dct(dev, channel, index_reg, index);
		if (i & 1) {
			/* odd byte lane */
			val &= ~(0x1ff << 16);
			val |= ((RcvrEnDly & 0x1ff) << 16);
		} else {
			/* even byte lane */
			val &= ~0x1ff;
			val |= (RcvrEnDly & 0x1ff);
		}
		set_nb32_index_wait_dct(dev, channel, index_reg, index, val);
	}

}

/* Calculate MaxRdLatency
 * Algorithm detailed in the Fam10h BKDG Rev. 3.62 section 2.8.9.9.5
 */
static void mct_set_max_latency_d(struct DCTStatStruc *p_dct_stat, u8 channel, u16 dqs_rcv_en_dly)
{
	u32 dev;
	u32 reg;
	u32 sub_total;
	u32 index_reg;
	u32 val;

	u8 cpu_val_n;
	u8 cpu_val_p;

	u16 freq_tab[] = {400, 533, 667, 800};

	/* Set up processor-dependent values */
	if (p_dct_stat->logical_cpuid & AMD_DR_Dx) {
		/* Revision D and above */
		cpu_val_n = 4;
		cpu_val_p = 29;
	} else if (p_dct_stat->logical_cpuid & AMD_DR_Cx) {
		/* Revision C */
		u8 package_type = mct_get_nv_bits(NV_PACK_TYPE);
		if ((package_type == PT_L1)		/* Socket F (1207) */
			|| (package_type == PT_M2)	/* Socket AM3 */
			|| (package_type == PT_S1)) {	/* Socket S1g <x> */
			cpu_val_n = 10;
			cpu_val_p = 11;
		} else {
			cpu_val_n = 4;
			cpu_val_p = 29;
		}
	} else {
		/* Revision B and below */
		cpu_val_n = 10;
		cpu_val_p = 11;
	}

	if (p_dct_stat->ganged_mode)
		channel = 0;

	dev = p_dct_stat->dev_dct;
	index_reg = 0x98;

	/* Multiply the CAS Latency by two to get a number of 1/2 MEMCLKs units.*/
	val = get_nb32_dct(dev, channel, 0x88);
	sub_total = ((val & 0x0f) + 4) << 1;	/* sub_total is 1/2 Memclk unit */

	/* If registered DIMMs are being used then
	 *  add 1 MEMCLK to the sub-total.
	 */
	val = get_nb32_dct(dev, channel, 0x90);
	if (!(val & (1 << UN_BUFF_DIMM)))
		sub_total += 2;

	/* If the address prelaunch is setup for 1/2 MEMCLKs then
	 *  add 1, else add 2 to the sub-total.
	 *  if (AddrCmdSetup || CsOdtSetup || CkeSetup) then K := K + 2;
	 */
	val = get_nb32_index_wait_dct(dev, channel, index_reg, 0x04);
	if (!(val & 0x00202020))
		sub_total += 1;
	else
		sub_total += 2;

	/* If the F2x[1, 0]78[RdPtrInit] field is 4, 5, 6 or 7 MEMCLKs,
	 * then add 4, 3, 2, or 1 MEMCLKs, respectively to the sub-total. */
	val = get_nb32_dct(dev, channel, 0x78);
	sub_total += 8 - (val & 0x0f);

	/* Convert bits 7-5 (also referred to as the coarse delay) of
	 * the current (or worst case) DQS receiver enable delay to
	 * 1/2 MEMCLKs units, rounding up, and add this to the sub-total.
	 */
	sub_total += dqs_rcv_en_dly >> 5;	/* Retrieve gross delay portion of value */

	/* Add "P" to the sub-total. "P" represents part of the
	 * processor specific constant delay value in the DRAM
	 * clock domain.
	 */
	sub_total <<= 1;		/*scale 1/2 MemClk to 1/4 MemClk */
	sub_total += cpu_val_p;	/*add "P" 1/2MemClk */
	sub_total >>= 1;		/*scale 1/4 MemClk back to 1/2 MemClk */

	/* Convert the sub-total (in 1/2 MEMCLKs) to northbridge
	 * clocks (NCLKs)
	 */
	sub_total *= 200 * ((get_nb32(p_dct_stat->dev_nbmisc, 0xd4) & 0x1f) + 4);
	sub_total /= freq_tab[((get_nb32_dct(p_dct_stat->dev_dct, channel, 0x94) & 0x7) - 3)];
	sub_total = (sub_total + (2 - 1)) / 2;	/* Round up */

	/* Add "N" NCLKs to the sub-total. "N" represents part of the
	 * processor specific constant value in the northbridge
	 * clock domain.
	 */
	sub_total += (cpu_val_n) / 2;

	p_dct_stat->ch_max_rd_lat[channel][0] = sub_total;
	if (p_dct_stat->ganged_mode) {
		p_dct_stat->ch_max_rd_lat[1][0] = sub_total;
	}

	/* Program the F2x[1, 0]78[MaxRdLatency] register with
	 * the total delay value (in NCLKs).
	 */
	reg = 0x78;
	val = get_nb32_dct(dev, channel, reg);
	val &= ~(0x3ff << 22);
	val |= (sub_total & 0x3ff) << 22;

	/* program MaxRdLatency to correspond with current delay */
	set_nb32_dct(dev, channel, reg, val);
}

static void mct_init_dqs_pos_4_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Initialize the DQS Positions in preparation for
	 * receiver Enable Training.
	 * Write Position is 1/2 Memclock Delay
	 * Read Position is 1/2 Memclock Delay
	 */
	u8 i;
	for (i = 0; i < 2; i++) {
		init_dqs_pos_4_rcvr_en_d(p_mct_stat, p_dct_stat, i);
	}
}

static void init_dqs_pos_4_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 channel)
{
	/* Initialize the DQS Positions in preparation for
	 * receiver Enable Training.
	 * Write Position is no Delay
	 * Read Position is 1/2 Memclock Delay
	 */

	u8 i, j;
	u32 dword;
	u8 dn = 4; /* TODO: Rev C could be 4 */
	u32 dev = p_dct_stat->dev_dct;
	u32 index_reg = 0x98;

	/* FIXME: add Cx support */
	dword = 0x00000000;
	for (i = 1; i <= 3; i++) {
		for (j = 0; j < dn; j++)
			/* DIMM0 Write Data Timing Low */
			/* DIMM0 Write ECC Timing */
			set_nb32_index_wait_dct(dev, channel, index_reg, i + 0x100 * j, dword);
	}

	/* errata #180 */
	dword = 0x2f2f2f2f;
	for (i = 5; i <= 6; i++) {
		for (j = 0; j < dn; j++)
			/* DIMM0 Read DQS Timing Control Low */
			set_nb32_index_wait_dct(dev, channel, index_reg, i + 0x100 * j, dword);
	}

	dword = 0x0000002f;
	for (j = 0; j < dn; j++)
		/* DIMM0 Read DQS ECC Timing Control */
		set_nb32_index_wait_dct(dev, channel, index_reg, 7 + 0x100 * j, dword);
}

void set_ecc_dqs_rcvr_en_d(struct DCTStatStruc *p_dct_stat, u8 channel)
{
	u32 dev;
	u32 index_reg;
	u32 index;
	u8 chip_sel;
	u16 *p;
	u32 val;

	dev = p_dct_stat->dev_dct;
	index_reg = 0x98;
	index = 0x12;
	p = p_dct_stat->ch_d_bc_rcvr_dly[channel];
	print_debug_dqs("\t\tSetEccDQSRcvrPos: channel ", channel,  2);
	for (chip_sel = 0; chip_sel < MAX_CS_SUPPORTED; chip_sel += 2) {
		val = p[chip_sel >> 1];
		set_nb32_index_wait_dct(dev, channel, index_reg, index, val);
		print_debug_dqs_pair("\t\tSetEccDQSRcvrPos: chip_sel ",
					chip_sel, " rcvr_delay ",  val, 2);
		index += 3;
	}
}

static void calc_ecc_dqs_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 channel)
{
	u8 chip_sel;
	u16 ecc_dqs_like;
	u8 ecc_dqs_scale;
	u32 val, val0, val1;
	s16 delay_differential;

	ecc_dqs_like = p_dct_stat->ch_ecc_dqs_like[channel];
	ecc_dqs_scale = p_dct_stat->ch_ecc_dqs_scale[channel];

	for (chip_sel = 0; chip_sel < MAX_CS_SUPPORTED; chip_sel += 2) {
		if (mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel, chip_sel)) {
			u16 *p;
			p = p_dct_stat->ch_d_b_rcvr_dly[channel][chip_sel >> 1];

			if (p_dct_stat->status & (1 << SB_REGISTERED)) {
				val0 = p[0x2];
				val1 = p[0x3];

				delay_differential = (s16)val1 - (s16)val0;
				delay_differential += (s16)val1;

				val = delay_differential;
			} else {
				/* DQS Delay Value of Data Bytelane
				 * most like ECC byte lane */
				val0 = p[ecc_dqs_like & 0x07];
				/* DQS Delay Value of Data Bytelane
				 * 2nd most like ECC byte lane */
				val1 = p[(ecc_dqs_like >> 8) & 0x07];

				if (val0 > val1) {
					val = val0 - val1;
				} else {
					val = val1 - val0;
				}

				val *= ~ecc_dqs_scale;
				val >>= 8; /* /256 */

				if (val0 > val1) {
					val -= val1;
				} else {
					val += val0;
				}
			}

			p_dct_stat->ch_d_bc_rcvr_dly[channel][chip_sel >> 1] = val;
		}
	}
	set_ecc_dqs_rcvr_en_d(p_dct_stat, channel);
}

/* 2.8.9.9.4
 * ECC Byte Lane Training
 * DQS receiver Enable Delay
 */
void mct_set_ecc_dqs_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	u8 i;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		if (!p_dct_stat->node_present)
			break;
		if (p_dct_stat->dct_sys_limit) {
			for (i = 0; i < 2; i++)
				calc_ecc_dqs_rcvr_en_d(p_mct_stat, p_dct_stat, i);
		}
	}
}

void phy_assisted_mem_fence_training(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a, s16 single_node_number)
{
	u8 node = 0;
	struct DCTStatStruc *p_dct_stat;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	u8 start_node = 0;
	u8 end_node = MAX_NODES_SUPPORTED;

	if (single_node_number >= 0) {
		start_node = single_node_number;
		end_node = single_node_number + 1;
	}

	/* FIXME: skip for Ax */
	for (node = start_node; node < end_node; node++) {
		p_dct_stat = p_dct_stat_a + node;
		if (!p_dct_stat->node_present)
			continue;

		if (p_dct_stat->dct_sys_limit) {
			if (is_fam15h()) {
				/* Fam15h BKDG v3.14 section 2.10.5.3.3
				 * This picks up where init_ddr_phy left off
				 */
				u8 dct;
				u8 index;
				u32 dword;
				u32 datc_backup;
				u32 training_dword;
				u32 fence2_config_dword;
				u32 fence_tx_pad_config_dword;
				u32 index_reg = 0x98;
				u32 dev = p_dct_stat->dev_dct;

				for (dct = 0; dct < 2; dct++) {
					if (!p_dct_stat->dimm_valid_dct[dct])
						continue;

					printk(BIOS_SPEW, "%s: training node %d DCT %d\n", __func__, node, dct);

					/* Back up D18F2x9C_x0000_0004_dct[1:0] */
					datc_backup = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000004);

					/* FenceTrSel = 0x2 */
					dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000008);
					dword &= ~(0x3 << 6);
					dword |= (0x2 << 6);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000008, dword);

					/* Set phase recovery seed values */
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000050, 0x13131313);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000051, 0x13131313);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000052, 0x00000013);

					training_dword = fence_dyn_training_d(p_mct_stat, p_dct_stat, dct);

					/* Save calculated fence value to the TX DLL */
					dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c);
					dword &= ~(0x1f << 26);
					dword |= ((training_dword & 0x1f) << 26);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c, dword);

					/* D18F2x9C_x0D0F_0[F,8:0]0F_dct[1:0][AlwaysEnDllClks]=0x1 */
					for (index = 0; index < 0x9; index++) {
						dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f000f | (index << 8));
						dword &= ~(0x7 << 12);
						dword |= (0x1 << 12);
						set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f000f | (index << 8), dword);
					}

					/* FenceTrSel = 0x1 */
					dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000008);
					dword &= ~(0x3 << 6);
					dword |= (0x1 << 6);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000008, dword);

					/* Set phase recovery seed values */
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000050, 0x13131313);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000051, 0x13131313);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000052, 0x00000013);

					training_dword = fence_dyn_training_d(p_mct_stat, p_dct_stat, dct);

					/* Save calculated fence value to the RX DLL */
					dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c);
					dword &= ~(0x1f << 21);
					dword |= ((training_dword & 0x1f) << 21);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c, dword);

					/* D18F2x9C_x0D0F_0[F,8:0]0F_dct[1:0][AlwaysEnDllClks]=0x0 */
					for (index = 0; index < 0x9; index++) {
						dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f000f | (index << 8));
						dword &= ~(0x7 << 12);
						set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f000f | (index << 8), dword);
					}

					/* FenceTrSel = 0x3 */
					dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x00000008);
					dword &= ~(0x3 << 6);
					dword |= (0x3 << 6);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000008, dword);

					/* Set phase recovery seed values */
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000050, 0x13131313);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000051, 0x13131313);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000052, 0x00000013);

					fence_tx_pad_config_dword = fence_dyn_training_d(p_mct_stat, p_dct_stat, dct);

					/* Save calculated fence value to the TX Pad */
					dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c);
					dword &= ~(0x1f << 16);
					dword |= ((fence_tx_pad_config_dword & 0x1f) << 16);
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c, dword);

					/* Program D18F2x9C_x0D0F_[C,8,2][2:0]31_dct[1:0] */
					training_dword = fence_tx_pad_config_dword;
					if (fence_tx_pad_config_dword < 16)
						training_dword |= (0x1 << 4);
					else
						training_dword = 0;
					for (index = 0; index < 0x3; index++) {
						dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f2031 | (index << 8));
						dword &= ~(0x1f);
						dword |= (training_dword & 0x1f);
						set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f2031 | (index << 8), dword);
					}
					for (index = 0; index < 0x3; index++) {
						dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8031 | (index << 8));
						dword &= ~(0x1f);
						dword |= (training_dword & 0x1f);
						set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f8031 | (index << 8), dword);
					}
					for (index = 0; index < 0x3; index++) {
						dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc031 | (index << 8));
						dword &= ~(0x1f);
						dword |= (training_dword & 0x1f);
						set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0fc031 | (index << 8), dword);
					}

					/* Assemble Fence2 configuration word (Fam15h BKDG v3.14 page 331) */
					dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0000000c);
					fence2_config_dword = 0;

					/* TxPad */
					training_dword = (dword >> 16) & 0x1f;
					if (training_dword < 16)
						training_dword |= 0x10;
					else
						training_dword = 0;
					fence2_config_dword |= training_dword;

					/* RxDll */
					training_dword = (dword >> 21) & 0x1f;
					if (training_dword < 16)
						training_dword |= 0x10;
					else
						training_dword = 0;
					fence2_config_dword |= (training_dword << 10);

					/* TxDll */
					training_dword = (dword >> 26) & 0x1f;
					if (training_dword < 16)
						training_dword |= 0x10;
					else
						training_dword = 0;
					fence2_config_dword |= (training_dword << 5);

					/* Program D18F2x9C_x0D0F_0[F,8:0]31_dct[1:0] */
					for (index = 0; index < 0x9; index++) {
						dword = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f0031 | (index << 8));
						dword &= ~(0x7fff);
						dword |= fence2_config_dword;
						set_nb32_index_wait_dct(dev, dct, index_reg, 0x0d0f0031 | (index << 8), dword);
					}

					/* Restore D18F2x9C_x0000_0004_dct[1:0] */
					set_nb32_index_wait_dct(dev, dct, index_reg, 0x00000004, datc_backup);

					printk(BIOS_SPEW, "%s: done training node %d DCT %d\n", __func__, node, dct);
				}
			} else {
				fence_dyn_training_d(p_mct_stat, p_dct_stat, 0);
				fence_dyn_training_d(p_mct_stat, p_dct_stat, 1);
			}
		}
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

u32 fence_dyn_training_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u16 av_rec_value;
	u32 val;
	u32 dev;
	u32 index_reg = 0x98;
	u32 index;

	dev = p_dct_stat->dev_dct;

	if (is_fam15h()) {
		/* Set F2x[1,0]9C_x08[PHY_FENCE_TR_EN] */
		val = get_nb32_index_wait_dct(dev, dct, index_reg, 0x08);
		val |= 1 << PHY_FENCE_TR_EN;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x08, val);

		/* Wait 2000 MEMCLKs */
		precise_memclk_delay_fam15(p_mct_stat, p_dct_stat, dct, 2000);

		/* Clear F2x[1,0]9C_x08[PHY_FENCE_TR_EN] */
		val = get_nb32_index_wait_dct(dev, dct, index_reg, 0x08);
		val &= ~(1 << PHY_FENCE_TR_EN);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x08, val);

		/* BIOS reads the phase recovery engine registers
		 * F2x[1,0]9C_x[51:50] and F2x[1,0]9C_x52.
		 * Average the fine delay components only.
		 */
		av_rec_value = 0;
		for (index = 0x50; index <= 0x52; index++) {
			val = get_nb32_index_wait_dct(dev, dct, index_reg, index);
			av_rec_value += val & 0x1f;
			if (index != 0x52) {
				av_rec_value += (val >> 8) & 0x1f;
				av_rec_value += (val >> 16) & 0x1f;
				av_rec_value += (val >> 24) & 0x1f;
			}
		}

		val = av_rec_value / 9;
		if (av_rec_value % 9)
			val++;
		av_rec_value = val;

		if (av_rec_value < 6)
			av_rec_value = 0;
		else
			av_rec_value -= 6;

		return av_rec_value;
	} else {
		/* BIOS first programs a seed value to the phase recovery engine
		 *  (recommended 19) registers.
		 * Dram Phase Recovery Control Register (F2x[1,0]9C_x[51:50] and
		 * F2x[1,0]9C_x52.) .
		 */
		for (index = 0x50; index <= 0x52; index ++) {
			val = (FENCE_TRN_FIN_DLY_SEED & 0x1F);
			if (index != 0x52) {
				val |= val << 8 | val << 16 | val << 24;
			}
			set_nb32_index_wait_dct(dev, dct, index_reg, index, val);
		}

		/* Set F2x[1,0]9C_x08[PHY_FENCE_TR_EN]=1. */
		val = get_nb32_index_wait_dct(dev, dct, index_reg, 0x08);
		val |= 1 << PHY_FENCE_TR_EN;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x08, val);

		/* Wait 200 MEMCLKs. */
		mct_wait(50000);		/* wait 200us */

		/* Clear F2x[1,0]9C_x08[PHY_FENCE_TR_EN]=0. */
		val = get_nb32_index_wait_dct(dev, dct, index_reg, 0x08);
		val &= ~(1 << PHY_FENCE_TR_EN);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x08, val);

		/* BIOS reads the phase recovery engine registers
		 * F2x[1,0]9C_x[51:50] and F2x[1,0]9C_x52. */
		av_rec_value = 0;
		for (index = 0x50; index <= 0x52; index ++) {
			val = get_nb32_index_wait_dct(dev, dct, index_reg, index);
			av_rec_value += val & 0x7F;
			if (index != 0x52) {
				av_rec_value += (val >> 8) & 0x7F;
				av_rec_value += (val >> 16) & 0x7F;
				av_rec_value += (val >> 24) & 0x7F;
			}
		}

		val = av_rec_value / 9;
		if (av_rec_value % 9)
			val++;
		av_rec_value = val;

		/* Write the (averaged value -8) to F2x[1,0]9C_x0C[PhyFence]. */
		/* inlined mct_AdjustFenceValue() */
		/* TODO: The RBC0 is not supported. */
		/* if (p_dct_stat->logical_cpuid & AMD_RB_C0)
			av_rec_value -= 3;
		else
		*/
		if (p_dct_stat->logical_cpuid & AMD_DR_Dx)
			av_rec_value -= 8;
		else if (p_dct_stat->logical_cpuid & AMD_DR_Cx)
			av_rec_value -= 8;
		else if (p_dct_stat->logical_cpuid & AMD_DR_Bx)
			av_rec_value -= 8;

		val = get_nb32_index_wait_dct(dev, dct, index_reg, 0x0C);
		val &= ~(0x1F << 16);
		val |= (av_rec_value & 0x1F) << 16;
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x0C, val);

		/* Rewrite F2x[1,0]9C_x04-DRAM Address/Command Timing Control Register
		 * delays (both channels).
		 */
		val = get_nb32_index_wait_dct(dev, dct, index_reg, 0x04);
		set_nb32_index_wait_dct(dev, dct, index_reg, 0x04, val);

		return av_rec_value;
	}
}

void mct_wait(u32 cycles)
{
	u32 saved;
	u32 hi, lo, msr;

	/* Wait # of 50ns cycles
	   This seems like a hack to me...  */

	cycles <<= 3;		/* x8 (number of 1.25ns ticks) */

	msr = TSC_MSR;			/* TSC */
	_rdmsr(msr, &lo, &hi);
	saved = lo;
	do {
		_rdmsr(msr, &lo, &hi);
	} while (lo - saved < cycles);
}
