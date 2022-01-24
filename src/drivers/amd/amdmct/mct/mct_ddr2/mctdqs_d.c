/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/x86/cr.h>
#include <cpu/amd/msr.h>
#include <cpu/amd/mtrr.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>

static void calc_ecc_dqs_pos_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u16 like,
				u8 scale, u8 chip_sel);
static void get_dqs_dat_struc_val_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 chip_sel);
static u8 middle_dqs_d(u8 min, u8 max);
static void train_read_dqs_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 cs_start);
static void train_write_dqs_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 cs_start);
static void write_dqs_test_pattern_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u32 test_addr_lo);
static void write_l18_test_pattern_d(struct DCTStatStruc *p_dct_stat,
					u32 test_addr_lo);
static void write_l9_test_pattern_d(struct DCTStatStruc *p_dct_stat,
					u32 test_addr_lo);
static u8 compare_dqs_test_pattern_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u32 addr_lo);
static void flush_dqs_test_pattern_d(struct DCTStatStruc *p_dct_stat,
					u32 addr_lo);
static void read_dqs_test_pattern_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u32 test_addr_lo);
static void mct_set_dqs_delay_csr_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u8 chip_sel);
static void mct_set_dqs_delay_all_csr_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u8 cs_start);
static void setup_dqs_pattern_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u32 *buffer);

void print_debug_dqs(const char *str, u32 val, u8 level)
{
#if DQS_TRAIN_DEBUG > 0
	if (DQS_TRAIN_DEBUG >= level) {
		printk(BIOS_DEBUG, "%s%x\n", str, val);
	}
#endif
}

void print_debug_dqs_pair(const char *str, u32 val, const char *str2, u32 val2, u8 level)
{
#if DQS_TRAIN_DEBUG > 0
	if (DQS_TRAIN_DEBUG >= level) {
		printk(BIOS_DEBUG, "%s%08x%s%08x\n", str, val, str2, val2);
	}
#endif
}

/*Warning:  These must be located so they do not cross a logical 16-bit segment boundary!*/
static const u32 test_pattern_jd1a_d[] = {
	0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF, /* QW0-1, ALL-EVEN */
	0x00000000,0x00000000,0x00000000,0x00000000, /* QW2-3, ALL-EVEN */
	0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF, /* QW4-5, ALL-EVEN */
	0x00000000,0x00000000,0x00000000,0x00000000, /* QW6-7, ALL-EVEN */
	0xFeFeFeFe,0xFeFeFeFe,0x01010101,0x01010101, /* QW0-1, DQ0-ODD */
	0xFeFeFeFe,0xFeFeFeFe,0x01010101,0x01010101, /* QW2-3, DQ0-ODD */
	0x01010101,0x01010101,0xFeFeFeFe,0xFeFeFeFe, /* QW4-5, DQ0-ODD */
	0xFeFeFeFe,0xFeFeFeFe,0x01010101,0x01010101, /* QW6-7, DQ0-ODD */
	0x02020202,0x02020202,0x02020202,0x02020202, /* QW0-1, DQ1-ODD */
	0xFdFdFdFd,0xFdFdFdFd,0xFdFdFdFd,0xFdFdFdFd, /* QW2-3, DQ1-ODD */
	0xFdFdFdFd,0xFdFdFdFd,0x02020202,0x02020202, /* QW4-5, DQ1-ODD */
	0x02020202,0x02020202,0x02020202,0x02020202, /* QW6-7, DQ1-ODD */
	0x04040404,0x04040404,0xfBfBfBfB,0xfBfBfBfB, /* QW0-1, DQ2-ODD */
	0x04040404,0x04040404,0x04040404,0x04040404, /* QW2-3, DQ2-ODD */
	0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB, /* QW4-5, DQ2-ODD */
	0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB, /* QW6-7, DQ2-ODD */
	0x08080808,0x08080808,0xF7F7F7F7,0xF7F7F7F7, /* QW0-1, DQ3-ODD */
	0x08080808,0x08080808,0x08080808,0x08080808, /* QW2-3, DQ3-ODD */
	0xF7F7F7F7,0xF7F7F7F7,0x08080808,0x08080808, /* QW4-5, DQ3-ODD */
	0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7, /* QW6-7, DQ3-ODD */
	0x10101010,0x10101010,0x10101010,0x10101010, /* QW0-1, DQ4-ODD */
	0xeFeFeFeF,0xeFeFeFeF,0x10101010,0x10101010, /* QW2-3, DQ4-ODD */
	0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF, /* QW4-5, DQ4-ODD */
	0xeFeFeFeF,0xeFeFeFeF,0x10101010,0x10101010, /* QW6-7, DQ4-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW0-1, DQ5-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0x20202020,0x20202020, /* QW2-3, DQ5-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW4-5, DQ5-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW6-7, DQ5-ODD */
	0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf, /* QW0-1, DQ6-ODD */
	0x40404040,0x40404040,0xBfBfBfBf,0xBfBfBfBf, /* QW2-3, DQ6-ODD */
	0x40404040,0x40404040,0xBfBfBfBf,0xBfBfBfBf, /* QW4-5, DQ6-ODD */
	0x40404040,0x40404040,0xBfBfBfBf,0xBfBfBfBf, /* QW6-7, DQ6-ODD */
	0x80808080,0x80808080,0x7F7F7F7F,0x7F7F7F7F, /* QW0-1, DQ7-ODD */
	0x80808080,0x80808080,0x7F7F7F7F,0x7F7F7F7F, /* QW2-3, DQ7-ODD */
	0x80808080,0x80808080,0x7F7F7F7F,0x7F7F7F7F, /* QW4-5, DQ7-ODD */
	0x80808080,0x80808080,0x80808080,0x80808080  /* QW6-7, DQ7-ODD */
};
static const u32 test_pattern_jd1b_d[] = {
	0x00000000,0x00000000,0x00000000,0x00000000, /* QW0,CHA-B, ALL-EVEN */
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF, /* QW1,CHA-B, ALL-EVEN */
	0x00000000,0x00000000,0x00000000,0x00000000, /* QW2,CHA-B, ALL-EVEN */
	0x00000000,0x00000000,0x00000000,0x00000000, /* QW3,CHA-B, ALL-EVEN */
	0x00000000,0x00000000,0x00000000,0x00000000, /* QW4,CHA-B, ALL-EVEN */
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF, /* QW5,CHA-B, ALL-EVEN */
	0x00000000,0x00000000,0x00000000,0x00000000, /* QW6,CHA-B, ALL-EVEN */
	0x00000000,0x00000000,0x00000000,0x00000000, /* QW7,CHA-B, ALL-EVEN */
	0xFeFeFeFe,0xFeFeFeFe,0xFeFeFeFe,0xFeFeFeFe, /* QW0,CHA-B, DQ0-ODD */
	0x01010101,0x01010101,0x01010101,0x01010101, /* QW1,CHA-B, DQ0-ODD */
	0xFeFeFeFe,0xFeFeFeFe,0xFeFeFeFe,0xFeFeFeFe, /* QW2,CHA-B, DQ0-ODD */
	0x01010101,0x01010101,0x01010101,0x01010101, /* QW3,CHA-B, DQ0-ODD */
	0x01010101,0x01010101,0x01010101,0x01010101, /* QW4,CHA-B, DQ0-ODD */
	0xFeFeFeFe,0xFeFeFeFe,0xFeFeFeFe,0xFeFeFeFe, /* QW5,CHA-B, DQ0-ODD */
	0xFeFeFeFe,0xFeFeFeFe,0xFeFeFeFe,0xFeFeFeFe, /* QW6,CHA-B, DQ0-ODD */
	0x01010101,0x01010101,0x01010101,0x01010101, /* QW7,CHA-B, DQ0-ODD */
	0x02020202,0x02020202,0x02020202,0x02020202, /* QW0,CHA-B, DQ1-ODD */
	0x02020202,0x02020202,0x02020202,0x02020202, /* QW1,CHA-B, DQ1-ODD */
	0xFdFdFdFd,0xFdFdFdFd,0xFdFdFdFd,0xFdFdFdFd, /* QW2,CHA-B, DQ1-ODD */
	0xFdFdFdFd,0xFdFdFdFd,0xFdFdFdFd,0xFdFdFdFd, /* QW3,CHA-B, DQ1-ODD */
	0xFdFdFdFd,0xFdFdFdFd,0xFdFdFdFd,0xFdFdFdFd, /* QW4,CHA-B, DQ1-ODD */
	0x02020202,0x02020202,0x02020202,0x02020202, /* QW5,CHA-B, DQ1-ODD */
	0x02020202,0x02020202,0x02020202,0x02020202, /* QW6,CHA-B, DQ1-ODD */
	0x02020202,0x02020202,0x02020202,0x02020202, /* QW7,CHA-B, DQ1-ODD */
	0x04040404,0x04040404,0x04040404,0x04040404, /* QW0,CHA-B, DQ2-ODD */
	0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB, /* QW1,CHA-B, DQ2-ODD */
	0x04040404,0x04040404,0x04040404,0x04040404, /* QW2,CHA-B, DQ2-ODD */
	0x04040404,0x04040404,0x04040404,0x04040404, /* QW3,CHA-B, DQ2-ODD */
	0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB, /* QW4,CHA-B, DQ2-ODD */
	0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB, /* QW5,CHA-B, DQ2-ODD */
	0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB, /* QW6,CHA-B, DQ2-ODD */
	0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB,0xfBfBfBfB, /* QW7,CHA-B, DQ2-ODD */
	0x08080808,0x08080808,0x08080808,0x08080808, /* QW0,CHA-B, DQ3-ODD */
	0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7, /* QW1,CHA-B, DQ3-ODD */
	0x08080808,0x08080808,0x08080808,0x08080808, /* QW2,CHA-B, DQ3-ODD */
	0x08080808,0x08080808,0x08080808,0x08080808, /* QW3,CHA-B, DQ3-ODD */
	0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7, /* QW4,CHA-B, DQ3-ODD */
	0x08080808,0x08080808,0x08080808,0x08080808, /* QW5,CHA-B, DQ3-ODD */
	0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7, /* QW6,CHA-B, DQ3-ODD */
	0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7,0xF7F7F7F7, /* QW7,CHA-B, DQ3-ODD */
	0x10101010,0x10101010,0x10101010,0x10101010, /* QW0,CHA-B, DQ4-ODD */
	0x10101010,0x10101010,0x10101010,0x10101010, /* QW1,CHA-B, DQ4-ODD */
	0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF, /* QW2,CHA-B, DQ4-ODD */
	0x10101010,0x10101010,0x10101010,0x10101010, /* QW3,CHA-B, DQ4-ODD */
	0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF, /* QW4,CHA-B, DQ4-ODD */
	0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF, /* QW5,CHA-B, DQ4-ODD */
	0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF,0xeFeFeFeF, /* QW6,CHA-B, DQ4-ODD */
	0x10101010,0x10101010,0x10101010,0x10101010, /* QW7,CHA-B, DQ4-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW0,CHA-B, DQ5-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW1,CHA-B, DQ5-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW2,CHA-B, DQ5-ODD */
	0x20202020,0x20202020,0x20202020,0x20202020, /* QW3,CHA-B, DQ5-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW4,CHA-B, DQ5-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW5,CHA-B, DQ5-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW6,CHA-B, DQ5-ODD */
	0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF,0xdFdFdFdF, /* QW7,CHA-B, DQ5-ODD */
	0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf, /* QW0,CHA-B, DQ6-ODD */
	0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf, /* QW1,CHA-B, DQ6-ODD */
	0x40404040,0x40404040,0x40404040,0x40404040, /* QW2,CHA-B, DQ6-ODD */
	0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf, /* QW3,CHA-B, DQ6-ODD */
	0x40404040,0x40404040,0x40404040,0x40404040, /* QW4,CHA-B, DQ6-ODD */
	0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf, /* QW5,CHA-B, DQ6-ODD */
	0x40404040,0x40404040,0x40404040,0x40404040, /* QW6,CHA-B, DQ6-ODD */
	0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf,0xBfBfBfBf, /* QW7,CHA-B, DQ6-ODD */
	0x80808080,0x80808080,0x80808080,0x80808080, /* QW0,CHA-B, DQ7-ODD */
	0x7F7F7F7F,0x7F7F7F7F,0x7F7F7F7F,0x7F7F7F7F, /* QW1,CHA-B, DQ7-ODD */
	0x80808080,0x80808080,0x80808080,0x80808080, /* QW2,CHA-B, DQ7-ODD */
	0x7F7F7F7F,0x7F7F7F7F,0x7F7F7F7F,0x7F7F7F7F, /* QW3,CHA-B, DQ7-ODD */
	0x80808080,0x80808080,0x80808080,0x80808080, /* QW4,CHA-B, DQ7-ODD */
	0x7F7F7F7F,0x7F7F7F7F,0x7F7F7F7F,0x7F7F7F7F, /* QW5,CHA-B, DQ7-ODD */
	0x80808080,0x80808080,0x80808080,0x80808080, /* QW6,CHA-B, DQ7-ODD */
	0x80808080,0x80808080,0x80808080,0x80808080  /* QW7,CHA-B, DQ7-ODD */
};

void train_receiver_en_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a, u8 Pass)
{
	u8 node;
	struct DCTStatStruc *p_dct_stat;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		p_dct_stat = p_dct_stat_a + node;

/*FIXME: needed?		if (!p_dct_stat->node_present)
			break;
*/
		if (p_dct_stat->dct_sys_limit) {
			mct_train_rcvr_en_d(p_mct_stat, p_dct_stat, Pass);
		}
	}
}


static void set_ecc_dqs_rd_wr_pos_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 chip_sel)
{
	u8 channel;
	u8 direction;

	for (channel = 0; channel < 2; channel++) {
		for (direction = 0; direction < 2; direction++) {
			p_dct_stat->channel = channel;	/* channel A or B */
			p_dct_stat->direction = direction; /* Read or write */
			calc_ecc_dqs_pos_d(p_mct_stat, p_dct_stat, p_dct_stat->ch_ecc_dqs_like[channel], p_dct_stat->ch_ecc_dqs_scale[channel], chip_sel);
			print_debug_dqs_pair("\t\tSetEccDQSRdWrPos: channel ", channel, direction == DQS_READDIR ? " R dqs_delay":" W dqs_delay",	p_dct_stat->dqs_delay, 2);
			p_dct_stat->byte_lane = 8;
			store_dqs_dat_struct_val_d(p_mct_stat, p_dct_stat, chip_sel);
			mct_set_dqs_delay_csr_d(p_mct_stat, p_dct_stat, chip_sel);
		}
	}
}



static void calc_ecc_dqs_pos_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u16 like, u8 scale, u8 chip_sel)
{
	u8 dqs_delay_0, dqs_delay_1;
	u16 dqs_delay;

	p_dct_stat->byte_lane = like & 0xff;
	get_dqs_dat_struc_val_d(p_mct_stat, p_dct_stat, chip_sel);
	dqs_delay_0 = p_dct_stat->dqs_delay;

	p_dct_stat->byte_lane = (like >> 8) & 0xff;
	get_dqs_dat_struc_val_d(p_mct_stat, p_dct_stat, chip_sel);
	dqs_delay_1 = p_dct_stat->dqs_delay;

	if (dqs_delay_0 > dqs_delay_1) {
		dqs_delay = dqs_delay_0 - dqs_delay_1;
	} else {
		dqs_delay = dqs_delay_1 - dqs_delay_0;
	}

	dqs_delay = dqs_delay * (~scale);

	dqs_delay += 0x80;	// round it

	dqs_delay >>= 8;		// /256

	if (dqs_delay_0 > dqs_delay_1) {
		dqs_delay = dqs_delay_1 - dqs_delay;
	} else {
		dqs_delay += dqs_delay_1;
	}

	p_dct_stat->dqs_delay = (u8)dqs_delay;
}


static void train_dqs_rd_wr_pos_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 cs_start)
{
	u32 errors;
	u8 channel, dqs_wr_delay;
	u8 _disable_dram_ecc = 0;
	u32 pattern_buffer[292];
	u8 _wrap_32_dis = 0, _sse2 = 0;
	u8 dqs_wr_delay_end;

	u32 addr;
	CRx_TYPE cr4;
	u32 lo, hi;

	print_debug_dqs("\nTrainDQSRdWrPos: node_id ", p_dct_stat->node_id, 0);
	cr4 = read_cr4();
	if (cr4 & (1 << 9)) {
		_sse2 = 1;
	}
	cr4 |= (1 << 9);	/* OSFXSR enable SSE2 */
	write_cr4(cr4);

	addr = HWCR_MSR;
	_rdmsr(addr, &lo, &hi);
	if (lo & (1 << 17)) {
		_wrap_32_dis = 1;
	}
	lo |= (1 << 17);	/* HWCR.wrap32dis */
	_wrmsr(addr, lo, hi);	/* allow 64-bit memory references in real mode */

	/* Disable ECC correction of reads on the dram bus. */
	_disable_dram_ecc = mct_disable_dimm_ecc_en_d(p_mct_stat, p_dct_stat);

	setup_dqs_pattern_d(p_mct_stat, p_dct_stat, pattern_buffer);

	/* mct_BeforeTrainDQSRdWrPos_D */
	dqs_wr_delay_end = 0x20;

	errors = 0;
	for (channel = 0; channel < 2; channel++) {
		print_debug_dqs("\tTrainDQSRdWrPos: 1 channel ",channel, 1);
		p_dct_stat->channel = channel;

		if (p_dct_stat->dimm_valid_dct[channel] == 0)	/* mct_BeforeTrainDQSRdWrPos_D */
			continue;

		for (dqs_wr_delay = 0; dqs_wr_delay < dqs_wr_delay_end; dqs_wr_delay++) {
			p_dct_stat->dqs_delay = dqs_wr_delay;
			p_dct_stat->direction = DQS_WRITEDIR;
			mct_set_dqs_delay_all_csr_d(p_mct_stat, p_dct_stat, cs_start);

			print_debug_dqs("\t\tTrainDQSRdWrPos: 21 dqs_wr_delay ", dqs_wr_delay, 2);
			train_read_dqs_d(p_mct_stat, p_dct_stat, cs_start);

			print_debug_dqs("\t\tTrainDQSRdWrPos: 22 train_errors ",p_dct_stat->train_errors, 2);
			if (p_dct_stat->train_errors == 0) {
					break;
			}
			errors |= p_dct_stat->train_errors;
		}
		if (dqs_wr_delay < dqs_wr_delay_end) {
			errors = 0;

			print_debug_dqs("\tTrainDQSRdWrPos: 231 dqs_wr_delay ", dqs_wr_delay, 1);
			train_write_dqs_d(p_mct_stat, p_dct_stat, cs_start);
		}
		print_debug_dqs("\tTrainDQSRdWrPos: 232 errors ", errors, 1);
		p_dct_stat->err_status |= errors;
	}

#if DQS_TRAIN_DEBUG > 0
	{
		u8 val;
		u8 i;
		u8 channel, Receiver, Dir;
		u8 *p;

		for (Dir = 0; Dir < 2; Dir++) {
			if (Dir == 0) {
				printk(BIOS_DEBUG, "TrainDQSRdWrPos: ch_d_dir_b_dqs WR:\n");
			} else {
				printk(BIOS_DEBUG, "TrainDQSRdWrPos: ch_d_dir_b_dqs RD:\n");
			}
			for (channel = 0; channel < 2; channel++) {
				printk(BIOS_DEBUG, "channel: %02x\n", channel);
				for (Receiver = cs_start; Receiver < (cs_start + 2); Receiver += 2) {
					printk(BIOS_DEBUG, "\t\tReceiver: %02x: ", Receiver);
					p = p_dct_stat->persistentData.ch_d_dir_b_dqs[channel][Receiver >> 1][Dir];
					for (i = 0; i < 8; i++) {
						val  = p[i];
						printk(BIOS_DEBUG, "%02x ", val);
					}
					printk(BIOS_DEBUG, "\n");
				}
			}
		}

	}
#endif

	if (_disable_dram_ecc) {
		mct_enable_dimm_ecc_en_d(p_mct_stat, p_dct_stat, _disable_dram_ecc);
	}
	if (!_wrap_32_dis) {
		addr = HWCR_MSR;
		_rdmsr(addr, &lo, &hi);
		lo &= ~(1 << 17);	/* restore HWCR.wrap32dis */
		_wrmsr(addr, lo, hi);
	}
	if (!_sse2) {
		cr4 = read_cr4();
		cr4 &= ~(1 << 9);	/* restore cr4.OSFXSR */
		write_cr4(cr4);
	}

	print_tx("TrainDQSRdWrPos: status ", p_dct_stat->status);
	print_tx("TrainDQSRdWrPos: train_errors ", p_dct_stat->train_errors);
	print_tx("TrainDQSRdWrPos: err_status ", p_dct_stat->err_status);
	print_tx("TrainDQSRdWrPos: err_code ", p_dct_stat->err_code);
	print_t("TrainDQSRdWrPos: Done\n");
}


static void setup_dqs_pattern_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u32 *buffer)
{
	/* 1. Set the Pattern type (0 or 1) in DCTStatstruc.Pattern
	 * 2. Copy the pattern from ROM to Cache, aligning on 16 byte boundary
	 * 3. Set the ptr to Cacheable copy in DCTStatstruc.PtrPatternBufA
	 */

	u32 *buf;
	u16 i;

	buf = (u32 *)(((u32)buffer + 0x10) & (0xfffffff0));
	if (p_dct_stat->status & (1 << SB_128_BIT_MODE)) {
		p_dct_stat->pattern = 1;	/* 18 cache lines, alternating qwords */
		for (i = 0; i < 16*18; i++)
			buf[i] = test_pattern_jd1b_d[i];
	} else {
		p_dct_stat->pattern = 0;	/* 9 cache lines, sequential qwords */
		for (i = 0; i < 16*9; i++)
			buf[i] = test_pattern_jd1a_d[i];
	}
	p_dct_stat->ptr_pattern_buf_a = (u32)buf;
}


static void train_dqs_pos_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 cs_start)
{
	u32 errors;
	u8 chip_sel, dqs_delay;
	u8 rnk_dly_seq_pass_min, rnk_dly_seq_pass_max, rnk_dly_filter_min, rnk_dly_filter_max;
	u8 last_test;
	u32 test_addr;
	u8 byte_lane;
	u8 mutual_cs_pass_w[64];
	u8 banks_present;
	u8 dqs_delay_end;
	u8 tmp, valid;


	/* mutual_cs_pass_w: each byte represents a bitmap of pass/fail per
	 * byte_lane.  The indext within mutual_cs_pass_w is the delay value
	 * given the results.
	 */


	print_debug_dqs("\t\t\tTrainDQSPos begin ", 0, 3);

	errors = 0;
	banks_present = 0;

	if (p_dct_stat->direction == DQS_READDIR) {
		dqs_delay_end = 64;
		mct_adjust_delay_range_d(p_mct_stat, p_dct_stat, &dqs_delay_end);
	} else {
		dqs_delay_end = 32;
	}

	/* Bitmapped status per delay setting, 0xff = All positions
	 * passing (1= PASS). Set the entire array.
	 */
	for (dqs_delay = 0; dqs_delay < 64; dqs_delay++) {
		mutual_cs_pass_w[dqs_delay] = 0xFF;
	}

	for (chip_sel = cs_start; chip_sel < (cs_start + 2); chip_sel++) { /* logical register chipselects 0..7 */
		print_debug_dqs("\t\t\t\tTrainDQSPos: 11 chip_sel ", chip_sel, 4);

		if (!mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, p_dct_stat->channel, chip_sel)) {
			print_debug_dqs("\t\t\t\tmct_RcvrRankEnabled_D CS not enabled ", chip_sel, 4);
			continue;
		}

		banks_present = 1;	/* flag for at least one bank is present */
		test_addr = mct_get_mct_sys_addr_d(p_mct_stat, p_dct_stat, p_dct_stat->channel, chip_sel, &valid);
		if (!valid) {
			print_debug_dqs("\t\t\t\tAddress not supported on current CS ", test_addr, 4);
			continue;
		}

		print_debug_dqs("\t\t\t\tTrainDQSPos: 12 test_addr ", test_addr, 4);
		set_upper_fs_base(test_addr);	/* fs:eax = far ptr to target */

		if (p_dct_stat->direction == DQS_READDIR) {
			print_debug_dqs("\t\t\t\tTrainDQSPos: 13 for read ", 0, 4);
			write_dqs_test_pattern_d(p_mct_stat, p_dct_stat, test_addr << 8);
		}

		for (dqs_delay = 0; dqs_delay < dqs_delay_end; dqs_delay++) {
			print_debug_dqs("\t\t\t\t\tTrainDQSPos: 141 dqs_delay ", dqs_delay, 5);
			if (mutual_cs_pass_w[dqs_delay] == 0)
				continue; //skip current delay value if other chipselects have failed all 8 bytelanes
			p_dct_stat->dqs_delay = dqs_delay;
			mct_set_dqs_delay_all_csr_d(p_mct_stat, p_dct_stat, cs_start);
			print_debug_dqs("\t\t\t\t\tTrainDQSPos: 142 mutual_cs_pass_w ", mutual_cs_pass_w[dqs_delay], 5);

			if (p_dct_stat->direction == DQS_WRITEDIR) {
				print_debug_dqs("\t\t\t\t\tTrainDQSPos: 143 for write", 0, 5);
				write_dqs_test_pattern_d(p_mct_stat, p_dct_stat, test_addr << 8);
			}

			print_debug_dqs("\t\t\t\t\tTrainDQSPos: 144 Pattern ", p_dct_stat->pattern, 5);
			read_dqs_test_pattern_d(p_mct_stat, p_dct_stat, test_addr << 8);
			/* print_debug_dqs("\t\t\t\t\tTrainDQSPos: 145 mutual_cs_pass_w ", mutual_cs_pass_w[dqs_delay], 5); */
			tmp = compare_dqs_test_pattern_d(p_mct_stat, p_dct_stat, test_addr << 8); /* 0 = fail, 1 = pass */

			if (mct_check_fence_hole_adjust_d(p_mct_stat, p_dct_stat, dqs_delay, chip_sel, &tmp)) {
				goto skipLocMiddle;
			}

			mutual_cs_pass_w[dqs_delay] &= tmp;
			print_debug_dqs("\t\t\t\t\tTrainDQSPos: 146\tMutualCSPassW ", mutual_cs_pass_w[dqs_delay], 5);

			set_target_wtio_d(test_addr);
			flush_dqs_test_pattern_d(p_dct_stat, test_addr << 8);
			reset_target_wtio_d();
		}

	}

	if (banks_present) {
		for (byte_lane = 0; byte_lane < 8; byte_lane++) {
			print_debug_dqs("\t\t\t\tTrainDQSPos: 31 byte_lane ",byte_lane, 4);
			p_dct_stat->byte_lane = byte_lane;
			last_test = DQS_FAIL;		/* Analyze the results */
			rnk_dly_seq_pass_min = 0;
			rnk_dly_seq_pass_max = 0;
			rnk_dly_filter_max = 0;
			rnk_dly_filter_min = 0;
			for (dqs_delay = 0; dqs_delay < dqs_delay_end; dqs_delay++) {
				if (mutual_cs_pass_w[dqs_delay] & (1 << byte_lane)) {
					print_debug_dqs("\t\t\t\t\tTrainDQSPos: 321 dqs_delay ", dqs_delay, 5);
					print_debug_dqs("\t\t\t\t\tTrainDQSPos: 322 mutual_cs_pass_w ", mutual_cs_pass_w[dqs_delay], 5);

					rnk_dly_seq_pass_max = dqs_delay;
					if (last_test == DQS_FAIL) {
						rnk_dly_seq_pass_min = dqs_delay; //start sequential run
					}
					if ((rnk_dly_seq_pass_max - rnk_dly_seq_pass_min) > (rnk_dly_filter_max-rnk_dly_filter_min)) {
						rnk_dly_filter_min = rnk_dly_seq_pass_min;
						rnk_dly_filter_max = rnk_dly_seq_pass_max;
					}
					last_test = DQS_PASS;
				} else {
					last_test = DQS_FAIL;
				}
			}
			print_debug_dqs("\t\t\t\tTrainDQSPos: 33 rnk_dly_seq_pass_max ", rnk_dly_seq_pass_max, 4);
			if (rnk_dly_seq_pass_max == 0) {
				errors |= 1 << SB_NO_DQS_POS; /* no passing window */
			} else {
				print_debug_dqs_pair("\t\t\t\tTrainDQSPos: 34 RnkDlyFilter: ", rnk_dly_filter_min, " ",  rnk_dly_filter_max, 4);
				if (((rnk_dly_filter_max - rnk_dly_filter_min) < MIN_DQS_WNDW)) {
					errors |= 1 << SB_SMALL_DQS;
				} else {
					u8 middle_dqs;
					/* mctEngDQSwindow_Save_D Not required for arrays */
					middle_dqs = middle_dqs_d(rnk_dly_filter_min, rnk_dly_filter_max);
					p_dct_stat->dqs_delay = middle_dqs;
					mct_set_dqs_delay_csr_d(p_mct_stat, p_dct_stat, cs_start);  /* load the register with the value */
					store_dqs_dat_struct_val_d(p_mct_stat, p_dct_stat, cs_start); /* store the value into the data structure */
					print_debug_dqs("\t\t\t\tTrainDQSPos: 42 middle_dqs : ",middle_dqs, 4);
				}
			}
		}
	}
skipLocMiddle:
	p_dct_stat->train_errors = errors;

	print_debug_dqs("\t\t\tTrainDQSPos: errors ", errors, 3);

}


void store_dqs_dat_struct_val_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 chip_sel)
{
	/* Store the dqs_delay value, found during a training sweep, into the DCT
	 * status structure for this node
	 */


	/* When 400, 533, 667, it will support dimm0/1/2/3,
	 * and set conf for dimm0, hw will copy to dimm1/2/3
	 * set for dimm1, hw will copy to dimm3
	 * Rev A/B only support DIMM0/1 when 800MHz and above + 0x100 to next dimm
	 * Rev C support DIMM0/1/2/3 when 800MHz and above  + 0x100 to next dimm
	 */

	/* FindDQSDatDimmVal_D is not required since we use an array */
	u8 dn = 0;

	if (p_dct_stat->status & (1 << SB_OVER_400MHZ))
		dn = chip_sel >> 1; /* if odd or even logical DIMM */

	p_dct_stat->persistentData.ch_d_dir_b_dqs[p_dct_stat->channel][dn][p_dct_stat->direction][p_dct_stat->byte_lane] =
					p_dct_stat->dqs_delay;
}


static void get_dqs_dat_struc_val_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 chip_sel)
{
	u8 dn = 0;


	/* When 400, 533, 667, it will support dimm0/1/2/3,
	 * and set conf for dimm0, hw will copy to dimm1/2/3
	 * set for dimm1, hw will copy to dimm3
	 * Rev A/B only support DIMM0/1 when 800MHz and above + 0x100 to next dimm
	 * Rev C support DIMM0/1/2/3 when 800MHz and above  + 0x100 to next dimm
	 */

	/* FindDQSDatDimmVal_D is not required since we use an array */
	if (p_dct_stat->status & (1 << SB_OVER_400MHZ))
		dn = chip_sel >> 1; /*if odd or even logical DIMM */

	p_dct_stat->dqs_delay =
		p_dct_stat->persistentData.ch_d_dir_b_dqs[p_dct_stat->channel][dn][p_dct_stat->direction][p_dct_stat->byte_lane];
}


/* FindDQSDatDimmVal_D is not required since we use an array */


static u8 middle_dqs_d(u8 min, u8 max)
{
	u8 size;
	size = max-min;
	if (size % 2)
		size++;		// round up if the size isn't even.
	return (min + (size >> 1));
}


static void train_read_dqs_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 cs_start)
{
	print_debug_dqs("\t\tTrainReadPos ", 0, 2);
	p_dct_stat->direction = DQS_READDIR;
	train_dqs_pos_d(p_mct_stat, p_dct_stat, cs_start);
}


static void train_write_dqs_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 cs_start)
{
	p_dct_stat->direction = DQS_WRITEDIR;
	print_debug_dqs("\t\tTrainWritePos", 0, 2);
	train_dqs_pos_d(p_mct_stat, p_dct_stat, cs_start);
}


void proc_iocl_flush_d(u32 addr_hi)
{
	set_target_wtio_d(addr_hi);
	proc_CLFLUSH(addr_hi);
	reset_target_wtio_d();
}


static u8 chip_sel_present_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 channel, u8 chip_sel)
{
	u32 val;
	u32 reg;
	u32 dev = p_dct_stat->dev_dct;
	u32 reg_off;
	u8 ret = 0;

	if (!p_dct_stat->ganged_mode) {
		reg_off = 0x100 * channel;
	} else {
		reg_off = 0;
	}

	if (chip_sel < MAX_CS_SUPPORTED) {
		reg = 0x40 + (chip_sel << 2) + reg_off;
		val = get_nb32(dev, reg);
		if (val & (1 << 0))
			ret = 1;
	}

	return ret;
}


/* proc_CLFLUSH_D located in mct_gcc.h */


static void write_dqs_test_pattern_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u32 test_addr_lo)
{
	/* Write a pattern of 72 bit times (per DQ), to test dram functionality.
	 * The pattern is a stress pattern which exercises both ISI and
	 * crosstalk.  The number of cache lines to fill is dependent on DCT
	 * width mode and burstlength.
	 * Mode BL  Lines Pattern no.
	 * ----+---+-------------------
	 * 64	4	  9	0
	 * 64	8	  9	0
	 * 64M	4	  9	0
	 * 64M	8	  9	0
	 * 128	4	  18	1
	 * 128	8	  N/A	-
	 */

	if (p_dct_stat->pattern == 0)
		write_l9_test_pattern_d(p_dct_stat, test_addr_lo);
	else
		write_l18_test_pattern_d(p_dct_stat, test_addr_lo);
}


static void write_l18_test_pattern_d(struct DCTStatStruc *p_dct_stat,
					u32 test_addr_lo)
{
	u8 *buf;

	buf = (u8 *)p_dct_stat->PtrPatternBufA;
	write_ln_test_pattern(test_addr_lo, buf, 18);

}


static void write_l9_test_pattern_d(struct DCTStatStruc *p_dct_stat,
					u32 test_addr_lo)
{
	u8 *buf;

	buf = (u8 *)p_dct_stat->PtrPatternBufA;
	write_ln_test_pattern(test_addr_lo, buf, 9);
}



static u8 compare_dqs_test_pattern_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u32 addr_lo)
{
	/* Compare a pattern of 72 bit times (per DQ), to test dram functionality.
	 * The pattern is a stress pattern which exercises both ISI and
	 * crosstalk.  The number of cache lines to fill is dependent on DCT
	 * width mode and burstlength.
	 * Mode BL  Lines Pattern no.
	 * ----+---+-------------------
	 * 64	4	  9	0
	 * 64	8	  9	0
	 * 64M	4	  9	0
	 * 64M	8	  9	0
	 * 128	4	  18	1
	 * 128	8	  N/A	-
	 */

	u32 *test_buf;
	u8 bitmap;
	u8 bytelane;
	u8 i;
	u32 value;
	u8 j;
	u32 value_test;
	u8 pattern, channel;

	pattern = p_dct_stat->pattern;
	channel = p_dct_stat->channel;
	test_buf = (u32 *)p_dct_stat->PtrPatternBufA;

	if (pattern && channel) {
		addr_lo += 8; //second channel
		test_buf += 2;
	}

	bytelane = 0;		/* bytelane counter */
	bitmap = 0xFF;		/* bytelane test bitmap, 1 = pass */
	for (i = 0; i < (9 * 64 / 4); i++) { /* sizeof testpattern. /4 due to next loop */
		value = read32_fs(addr_lo);
		value_test = *test_buf;

		print_debug_dqs_pair("\t\t\t\t\t\ttest_buf = ", (u32)test_buf, " value = ", value_test, 7);
		print_debug_dqs_pair("\t\t\t\t\t\ttaddr_lo = ", addr_lo, " value = ", value, 7);

		for (j = 0; j < (4 * 8); j += 8) { /* go through a 32bit data, on 1 byte step. */
			if (((value >> j) & 0xff) != ((value_test >> j) & 0xff)) {
				bitmap &= ~(1 << bytelane);
			}

			bytelane++;
			bytelane &= 0x7;
		}

		print_debug_dqs("\t\t\t\t\t\tbitmap = ", bitmap, 7);

		if (!bitmap)
			break;

		if (bytelane == 0) {
			if (pattern == 1) { //dual channel
				addr_lo += 8; //skip over other channel's data
				test_buf += 2;
			}
		}
		addr_lo += 4;
		test_buf += 1;
	}

	return bitmap;
}


static void flush_dqs_test_pattern_d(struct DCTStatStruc *p_dct_stat,
					u32 addr_lo)
{
	/* Flush functions in mct_gcc.h */
	if (p_dct_stat->pattern == 0) {
		flush_dqs_test_pattern_l9(addr_lo);
	} else {
		flush_dqs_test_pattern_l18(addr_lo);
	}
}

void set_target_wtio_d(u32 test_addr)
{
	u32 lo, hi;
	hi = test_addr >> 24;
	lo = test_addr << 8;
	_wrmsr(MTRR_IORR0_BASE, lo, hi);		/* IORR0 Base */
	hi = 0xFF;
	lo = 0xFC000800;			/* 64MB Mask */
	_wrmsr(MTRR_IORR0_MASK, lo, hi);		/* IORR0 Mask */
}


void reset_target_wtio_d(void)
{
	u32 lo, hi;

	hi = 0;
	lo = 0;
	_wrmsr(MTRR_IORR0_MASK, lo, hi); // IORR0 Mask
}


static void read_dqs_test_pattern_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u32 test_addr_lo)
{
	/* Read a pattern of 72 bit times (per DQ), to test dram functionality.
	 * The pattern is a stress pattern which exercises both ISI and
	 * crosstalk.  The number of cache lines to fill is dependent on DCT
	 * width mode and burstlength.
	 * Mode BL  Lines Pattern no.
	 * ----+---+-------------------
	 * 64	4	  9	0
	 * 64	8	  9	0
	 * 64M	4	  9	0
	 * 64M	8	  9	0
	 * 128	4	  18	1
	 * 128	8	  N/A	-
	 */
	if (p_dct_stat->pattern == 0)
		read_l9_test_pattern(test_addr_lo);
	else
		read_l18_test_pattern(test_addr_lo);
	_MFENCE;
}


u32 set_upper_fs_base(u32 addr_hi)
{
	/* Set the upper 32-bits of the Base address, 4GB aligned) for the
	 * FS selector.
	 */

	u32 lo, hi;
	u32 addr;
	lo = 0;
	hi = addr_hi >> 24;
	addr = FS_Base;
	_wrmsr(addr, lo, hi);
	return addr_hi << 8;
}


void reset_dct_wr_ptr_d(u32 dev, u32 index_reg, u32 index)
{
	u32 val;

	val = get_nb32_index_wait(dev, index_reg, index);
	set_nb32_index_wait(dev, index_reg, index, val);
}


/* mctEngDQSwindow_Save_D not required with arrays */


void mct_train_dqs_pos_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	u8 chip_sel;
	struct DCTStatStruc *p_dct_stat;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		p_dct_stat = p_dct_stat_a + node;
		if (p_dct_stat->dct_sys_limit) {
			/* when DCT speed >= 400MHz, we only support 2 DIMMs
			 * and we have two sets registers for DIMM0 and DIMM1 so
			 * here we must traning DQSRd/WrPos for DIMM0 and DIMM1
			 */
			if (p_dct_stat->speed >= 4) {
				p_dct_stat->status |= (1 << SB_OVER_400MHZ);
			}
			for (chip_sel = 0; chip_sel < MAX_CS_SUPPORTED; chip_sel += 2) {
				train_dqs_rd_wr_pos_d(p_mct_stat, p_dct_stat, chip_sel);
				set_ecc_dqs_rd_wr_pos_d(p_mct_stat, p_dct_stat, chip_sel);
			}
		}
	}
}


/* mct_BeforeTrainDQSRdWrPos_D
 * Function is inline.
 */

u8 mct_disable_dimm_ecc_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u8 _disable_dram_ecc = 0;
	u32 val;
	u32 reg;
	u32 dev;

	/*Disable ECC correction of reads on the dram bus. */

	dev = p_dct_stat->dev_dct;
	reg = 0x90;
	val = get_nb32(dev, reg);
	if (val & (1 << DIMM_EC_EN)) {
		_disable_dram_ecc |= 0x01;
		val &= ~(1 << DIMM_EC_EN);
		set_nb32(dev, reg, val);
	}
	if (!p_dct_stat->ganged_mode) {
		reg = 0x190;
		val = get_nb32(dev, reg);
		if (val & (1 << DIMM_EC_EN)) {
			_disable_dram_ecc |= 0x02;
			val &= ~(1 << DIMM_EC_EN);
			set_nb32(dev, reg, val);
		}
	}
	return _disable_dram_ecc;
}



void mct_enable_dimm_ecc_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 _disable_dram_ecc)
{

	u32 val;
	u32 reg;
	u32 dev;

	/* Enable ECC correction if it was previously disabled */

	dev = p_dct_stat->dev_dct;

	if ((_disable_dram_ecc & 0x01) == 0x01) {
		reg = 0x90;
		val = get_nb32(dev, reg);
		val |= (1 << DIMM_EC_EN);
		set_nb32(dev, reg, val);
	}
	if ((_disable_dram_ecc & 0x02) == 0x02) {
		reg = 0x190;
		val = get_nb32(dev, reg);
		val |= (1 << DIMM_EC_EN);
		set_nb32(dev, reg, val);
	}
}


static void mct_set_dqs_delay_csr_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 chip_sel)
{
	u8 byte_lane;
	u32 val;
	u32 index_reg = 0x98 + 0x100 * p_dct_stat->channel;
	u8 shift;
	u32 dqs_delay = (u32)p_dct_stat->dqs_delay;
	u32 dev = p_dct_stat->dev_dct;
	u32 index;

	byte_lane = p_dct_stat->byte_lane;

	/* channel is offset */
	if (byte_lane < 4) {
		index = 1;
	} else if (byte_lane <8) {
		index = 2;
	} else {
		index = 3;
	}

	if (p_dct_stat->direction == DQS_READDIR) {
		index += 4;
	}

	/* get the proper register index */
	shift = byte_lane % 4;
	shift <<= 3; /* get bit position of bytelane, 8 bit */

	if (p_dct_stat->status & (1 << SB_OVER_400MHZ)) {
		index += (chip_sel >> 1) * 0x100;	/* if logical DIMM1/DIMM3 */
	}

	val = get_nb32_index_wait(dev, index_reg, index);
	val &= ~(0x7f << shift);
	val |= (dqs_delay << shift);
	set_nb32_index_wait(dev, index_reg, index, val);
}


static void mct_set_dqs_delay_all_csr_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u8 cs_start)
{
	u8 byte_lane;
	u8 chip_sel = cs_start;


	for (chip_sel = cs_start; chip_sel < (cs_start + 2); chip_sel++) {
		if (mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, p_dct_stat->channel, chip_sel)) {
			for (byte_lane = 0; byte_lane < 8; byte_lane++) {
				p_dct_stat->byte_lane = byte_lane;
				mct_set_dqs_delay_csr_d(p_mct_stat, p_dct_stat, chip_sel);
			}
		}
	}
}


u8 mct_rcvr_rank_enabled_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 channel, u8 chip_sel)
{
	u8 ret;

	ret = chip_sel_present_d(p_mct_stat, p_dct_stat, channel, chip_sel);
	return ret;
}


u32 mct_get_rcvr_sys_addr_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 channel, u8 receiver, u8 *valid)
{
	return mct_get_mct_sys_addr_d(p_mct_stat, p_dct_stat, channel, receiver, valid);
}


u32 mct_get_mct_sys_addr_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 channel, u8 receiver, u8 *valid)
{
	u32 val;
	u32 reg_off = 0;
	u32 reg;
	u32 dword;
	u32 dev = p_dct_stat->dev_dct;

	*valid = 0;


	if (!p_dct_stat->ganged_mode) {
		reg_off = 0x100 * channel;
	}

	/* get the local base addr of the chipselect */
	reg = 0x40 + (receiver << 2) + reg_off;
	val = get_nb32(dev, reg);

	val &= ~0x0F;

	/* unganged mode DCT0+DCT1, sys addr of DCT1 = node
	 * base+DctSelBaseAddr+local ca base*/
	if ((channel) && (p_dct_stat->ganged_mode == 0) && (p_dct_stat->dimm_valid_dct[0] > 0)) {
		reg = 0x110;
		dword = get_nb32(dev, reg);
		dword &= 0xfffff800;
		dword <<= 8;	/* scale [47:27] of F2x110[31:11] to [39:8]*/
		val += dword;

		/* if DCTSelBaseAddr < Hole, and eax > HoleBase, then add Hole size to test address */
		if ((val >= p_dct_stat->dct_hole_base) && (p_dct_stat->dct_hole_base > dword)) {
			dword = (~(p_dct_stat->dct_hole_base >> (24 - 8)) + 1) & 0xFF;
			dword <<= (24 - 8);
			val += dword;
		}
	} else {
		/* sys addr = node base+local cs base */
		val += p_dct_stat->dct_sys_base;

		/* New stuff */
		if (p_dct_stat->dct_hole_base && (val >= p_dct_stat->dct_hole_base)) {
			val -= p_dct_stat->dct_sys_base;
			dword = get_nb32(p_dct_stat->dev_map, 0xF0); /* get Hole Offset */
			val += (dword & 0x0000ff00) << (24-8-8);
		}
	}

	/* New stuff */
	val += ((1 << 21) >> 8);	/* Add 2MB offset to avoid compat area */
	if (val >= MCT_TRNG_KEEPOUT_START) {
		while (val < MCT_TRNG_KEEPOUT_END)
			val += (1 << (15-8));	/* add 32K */
	}

	/* Add a node seed */
	val += (((1 * p_dct_stat->node_id) << 20) >> 8);	/* Add 1MB per node to avoid aliases */

	/* HW remap disabled? */
	if (!(p_dct_stat->status & (1 << SB_HW_HOLE))) {
		if (!(p_dct_stat->status & (1 << SB_SW_NODE_HOLE))) {
			/* SW memhole disabled */
			u32 lo, hi;
			_rdmsr(TOP_MEM, &lo, &hi);
			lo >>= 8;
			if ((val >= lo) && (val < _4GB_RJ8)) {
				val = 0;
				*valid = 0;
				goto exitGetAddr;
			} else {
				*valid = 1;
				goto exitGetAddrWNoError;
			}
		} else {
			*valid = 1;
			goto exitGetAddrWNoError;
		}
	} else {
		*valid = 1;
		goto exitGetAddrWNoError;
	}

exitGetAddrWNoError:

	/* Skip if Address is in UMA region */
	dword = p_mct_stat->sub_4G_cache_top;
	dword >>= 8;
	if (dword != 0) {
		if ((val >= dword) && (val < _4GB_RJ8)) {
			val = 0;
			*valid = 0;
		} else {
			*valid = 1;
		}
	}
	print_debug_dqs("mct_get_mct_sys_addr_d: receiver ", receiver, 2);
	print_debug_dqs("mct_get_mct_sys_addr_d: channel ", channel, 2);
	print_debug_dqs("mct_get_mct_sys_addr_d: base_addr ", val, 2);
	print_debug_dqs("mct_get_mct_sys_addr_d: valid ", *valid, 2);
	print_debug_dqs("mct_get_mct_sys_addr_d: status ", p_dct_stat->status, 2);
	print_debug_dqs("mct_get_mct_sys_addr_d: HoleBase ", p_dct_stat->dct_hole_base, 2);
	print_debug_dqs("mct_get_mct_sys_addr_d: Cachetop ", p_mct_stat->sub_4G_cache_top, 2);

exitGetAddr:
	return val;
}


void mct_write_1l_test_pattern_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u32 test_addr, u8 pattern)
{

	u8 *buf;

	/* Issue the stream of writes. When F2x11C[MctWrLimit] is reached
	 * (or when F2x11C[FLUSH_WR] is set again), all the writes are written
	 * to DRAM.
	 */

	set_upper_fs_base(test_addr);

	if (pattern)
		buf = (u8 *)p_dct_stat->ptr_pattern_buf_b;
	else
		buf = (u8 *)p_dct_stat->ptr_pattern_buf_a;

	write_ln_test_pattern(test_addr << 8, buf, 1);
}


void mct_read_1l_test_pattern_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u32 addr)
{
	/* BIOS issues the remaining (Ntrain - 2) reads after checking that
	 * F2x11C[PREF_DRAM_TRAIN_MODE] is cleared. These reads must be to
	 * consecutive cache lines (i.e., 64 bytes apart) and must not cross
	 * a naturally aligned 4KB boundary. These reads hit the prefetches and
	 * read the data from the prefetch buffer.
	 */

	/* get data from DIMM */
	set_upper_fs_base(addr);

	/* 1st move causes read fill (to exclusive or shared)*/
	read32_fs(addr << 8);
}
