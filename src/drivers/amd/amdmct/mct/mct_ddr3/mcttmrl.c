/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Description: max Read Latency Training feature for DDR 3 MCT
 */

#include <stdint.h>
#include <console/console.h>
#include <cpu/amd/msr.h>
#include <cpu/x86/cr.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static u8 compare_max_rd_lat_test_pattern_d(u32 pattern_buf, u32 addr);
static u32 get_max_rd_lat_test_addr_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 channel,
				u8 *max_rcvr_en_dly, u8 *valid);
u8 mct_get_start_max_rd_lat_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 channel,
				u8 dqs_rcv_en_dly, u32 *margin);
static void max_rd_latency_train_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void mct_set_max_rd_lat_trn_val_d(struct DCTStatStruc *p_dct_stat, u8 channel,
					u16 max_rd_lat_val);

/*Warning:  These must be located so they do not cross a logical 16-bit segment boundary!*/
static const u32 test_max_rd_lat_pattern_d[] = {
	0x6E0E3FAC, 0x0C3CFF52,
	0x4A688181, 0x49C5B613,
	0x7C780BA6, 0x5C1650E3,
	0x0C4F9D76, 0x0C6753E6,
	0x205535A5, 0xBABFB6CA,
	0x610E6E5F, 0x0C5F1C87,
	0x488493CE, 0x14C9C383,
	0xF5B9A5CD, 0x9CE8F615,

	0xAAD714B5, 0xC38F1B4C,
	0x72ED647C, 0x669F7562,
	0x5233F802, 0x4A898B30,
	0x10A40617, 0x3326B465,
	0x55386E04, 0xC807E3D3,
	0xAB49E193, 0x14B4E63A,
	0x67DF2495, 0xEA517C45,
	0x7624CE51, 0xF8140C51,

	0x4824BD23, 0xB61DD0C9,
	0x072BCFBE, 0xE8F3807D,
	0x919EA373, 0x25E30C47,
	0xFEB12958, 0x4DA80A5A,
	0xE9A0DDF8, 0x792B0076,
	0xE81C73DC, 0xF025B496,
	0x1DB7E627, 0x808594FE,
	0x82668268, 0x655C7783,
};

static u32 setup_max_rd_pattern(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u32 *buffer)
{
	/* 1. Copy the alpha and Beta patterns from ROM to Cache,
	 *    aligning on 16 byte boundary
	 * 2. Set the ptr to Cacheable copy in DCTStatstruc.PtrPatternBufA
	 *    for Alpha
	 * 3. Set the ptr to Cacheable copy in DCTStatstruc.PtrPatternBufB
	 *    for Beta
	 */
	u32 *buf;
	u8 i;

	buf = (u32 *)(((u32)buffer + 0x10) & (0xfffffff0));

	for (i = 0; i < (16 * 3); i++) {
		buf[i] = test_max_rd_lat_pattern_d[i];
	}

	return (u32)buf;
}

void train_max_read_latency_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;

		if (!p_dct_stat->node_present)
			break;

		if (p_dct_stat->dct_sys_limit)
			max_rd_latency_train_d(p_mct_stat, p_dct_stat);
	}
}

static void max_rd_latency_train_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u8 channel;
	u32 test_addr_0;
	u8 _disable_dram_ecc = 0, _wrap_32_dis = 0, _sse2 = 0;
	u16 max_rd_lat_dly;
	u8 rcvr_en_dly = 0;
	u32 pattern_buffer[60];	/* FIXME: why not 48 + 4 */
	u32 margin;
	u32 addr;
	CRx_TYPE cr4;
	u32 lo, hi;

	u8 valid;
	u32 pattern_buf;

	cr4 = read_cr4();
	if (cr4 & (1 << 9)) {		/* save the old value */
		_sse2 = 1;
	}
	cr4 |= (1 << 9);		/* OSFXSR enable SSE2 */
	write_cr4(cr4);

	addr = HWCR_MSR;
	_rdmsr(addr, &lo, &hi);
	if (lo & (1 << 17)) {		/* save the old value */
		_wrap_32_dis = 1;
	}
	lo |= (1 << 17);		/* HWCR.wrap32dis */
	lo &= ~(1 << 15);		/* SSEDIS */
	/* Setting wrap32dis allows 64-bit memory references in
	   real mode */
	_wrmsr(addr, lo, hi);

	_disable_dram_ecc = mct_disable_dimm_ecc_en_d(p_mct_stat, p_dct_stat);

	pattern_buf = setup_max_rd_pattern(p_mct_stat, p_dct_stat, pattern_buffer);

	for (channel = 0; channel < 2; channel++) {
		print_debug_dqs("\tMaxRdLatencyTrain51: channel ",channel, 1);
		p_dct_stat->channel = channel;

		if ((p_dct_stat->status & (1 << SB_128_BIT_MODE)) && channel)
			break;		/*if ganged mode, skip DCT 1 */

		test_addr_0 = get_max_rd_lat_test_addr_d(p_mct_stat, p_dct_stat, channel, &rcvr_en_dly,	 &valid);
		if (!valid)	/* Address not supported on current CS */
			continue;
		/* rank 1 of DIMM, testpattern 0 */
		write_max_rd_lat_1_cl_test_pattern_d(pattern_buf, test_addr_0);

		max_rd_lat_dly = mct_get_start_max_rd_lat_d(p_mct_stat, p_dct_stat, channel, rcvr_en_dly, &margin);
		print_debug_dqs("\tMaxRdLatencyTrain52:  max_rd_lat_dly start ", max_rd_lat_dly, 2);
		print_debug_dqs("\tMaxRdLatencyTrain52:  max_rd_lat_dly margin ", margin, 2);
		while (max_rd_lat_dly < MAX_RD_LAT) {	/* sweep Delay value here */
			mct_set_max_rd_lat_trn_val_d(p_dct_stat, channel, max_rd_lat_dly);
			read_max_rd_lat_1_cl_test_pattern_d(test_addr_0);
			if (compare_max_rd_lat_test_pattern_d(pattern_buf, test_addr_0) == DQS_PASS)
				break;
			set_target_wtio_d(test_addr_0);
			flush_max_rd_lat_test_pattern_d(test_addr_0);
			reset_target_wtio_d();
			max_rd_lat_dly++;
		}
		print_debug_dqs("\tMaxRdLatencyTrain53:  max_rd_lat_dly end ", max_rd_lat_dly, 2);
		mct_set_max_rd_lat_trn_val_d(p_dct_stat, channel, max_rd_lat_dly + margin);
	}

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

#if DQS_TRAIN_DEBUG > 0
	{
		u8 channel_dtd;
		printk(BIOS_DEBUG, "maxRdLatencyTrain: ch_max_rd_lat:\n");
		for (channel_dtd = 0; channel_dtd < 2; channel_dtd++) {
			printk(BIOS_DEBUG, "channel: %02x: %02x\n", channel_dtd, p_dct_stat->ch_max_rd_lat[channel_dtd][0]);
		}
	}
#endif
}

static void mct_set_max_rd_lat_trn_val_d(struct DCTStatStruc *p_dct_stat,
					u8 channel, u16 max_rd_lat_val)
{
	u8 i;
	u32 reg;
	u32 dev;
	u32 val;

	if (p_dct_stat->ganged_mode) {
		channel = 0; /* for safe */
		for (i = 0; i < 2; i++)
			p_dct_stat->ch_max_rd_lat[i][0] = max_rd_lat_val;
	} else {
		p_dct_stat->ch_max_rd_lat[channel][0] = max_rd_lat_val;
	}

	dev = p_dct_stat->dev_dct;
	reg = 0x78;
	val = Get_NB32_DCT(dev, channel, reg);
	val &= ~(0x3ff << 22);
	val |= max_rd_lat_val << 22;
	/* program MaxRdLatency to correspond with current delay */
	Set_NB32_DCT(dev, channel, reg, val);
}

static u8 compare_max_rd_lat_test_pattern_d(u32 pattern_buf, u32 addr)
{
	/* Compare only the first beat of data.  Since target addrs are cache
	 * line aligned, the channel parameter is used to determine which cache
	 * QW to compare.
	 */

	u32 *test_buf = (u32 *)pattern_buf;
	u32 addr_lo;
	u32 val, val_test;
	int i;
	u8 ret = DQS_PASS;

	set_upper_fs_base(addr);
	addr_lo = addr << 8;

	_EXECFENCE;
	for (i = 0; i < 16*3; i++) {
		val = read32_fs(addr_lo);
		val_test = test_buf[i];

		print_debug_dqs_pair("\t\t\t\t\t\ttest_buf = ", (u32)test_buf, " value = ", val_test, 5);
		print_debug_dqs_pair("\t\t\t\t\t\ttaddr_lo = ", addr_lo, " value = ", val, 5);
		if (val != val_test) {
			ret = DQS_FAIL;
			break;
		}
		addr_lo += 4;
	}

	return ret;
}

static u32 get_max_rd_lat_test_addr_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u8 channel, u8 *max_rcvr_en_dly,
					u8 *valid)
{
	u8 max = 0;

	u8 channel_max = 0;
	u8 d;
	u8 d_max = 0;

	u8 byte;
	u32 test_addr_0 = 0;
	u8 ch, ch_start, ch_end;
	u8 bn;

	bn = 8;

	if (p_dct_stat->status & (1 << SB_128_BIT_MODE)) {
		ch_start = 0;
		ch_end = 2;
	} else {
		ch_start = channel;
		ch_end = channel + 1;
	}

	*valid = 0;

	for (ch = ch_start; ch < ch_end; ch++) {
		for (d = 0; d < 4; d++) {
			for (byte = 0; byte < bn; byte++) {
				u8 tmp;
				tmp = p_dct_stat->ch_d_b_rcvr_dly[ch][d][byte];
				if (tmp > max) {
					max = tmp;
					channel_max = channel;
					d_max = d;
				}
			}
		}
	}

	if (mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel_max, d_max << 1))  {
		test_addr_0 = mct_get_mct_sys_addr_d(p_mct_stat, p_dct_stat, channel_max, d_max << 1, valid);
	}

	if (*valid)
		*max_rcvr_en_dly = max;

	return test_addr_0;
}

u8 mct_get_start_max_rd_lat_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u8 channel, u8 dqs_rcv_en_dly, u32 *margin)
{
	u32 sub_total;
	u32 val;
	u32 valx;
	u32 valxx;
	u32 index_reg;
	u32 dev;

	if (p_dct_stat->ganged_mode)
		channel =  0;

	index_reg = 0x98;

	dev = p_dct_stat->dev_dct;

	/* Multiply the CAS Latency by two to get a number of 1/2 MEMCLKs units.*/
	val = Get_NB32_DCT(dev, channel, 0x88);
	sub_total = ((val & 0x0f) + 1) << 1;	/* sub_total is 1/2 Memclk unit */

	/* If registered DIMMs are being used then add 1 MEMCLK to the sub-total*/
	val = Get_NB32_DCT(dev, channel, 0x90);
	if (!(val & (1 << UN_BUFF_DIMM)))
		sub_total += 2;

	/*If the address prelaunch is setup for 1/2 MEMCLKs then add 1,
	 *  else add 2 to the sub-total. if (AddrCmdSetup || CsOdtSetup
	 *  || CkeSetup) then K := K + 2; */
	val = Get_NB32_index_wait_DCT(dev, channel, index_reg, 0x04);
	if (!(val & 0x00202020))
		sub_total += 1;
	else
		sub_total += 2;

	/* If the F2x[1, 0]78[RdPtrInit] field is 4, 5, 6 or 7 MEMCLKs,
	 *  then add 4, 3, 2, or 1 MEMCLKs, respectively to the sub-total. */
	val = Get_NB32_DCT(dev, channel, 0x78);
	sub_total += 8 - (val & 0x0f);

	/* Convert bits 7-5 (also referred to as the course delay) of the current
	 * (or worst case) DQS receiver enable delay to 1/2 MEMCLKs units,
	 * rounding up, and add this to the sub-total. */
	sub_total += dqs_rcv_en_dly >> 5;	/*BOZO-no rounding up */

	sub_total <<= 1;			/*scale 1/2 MemClk to 1/4 MemClk */

	/* Convert the sub-total (in 1/2 MEMCLKs) to northbridge clocks (NCLKs)
	 * as follows (assuming DDR400 and assuming that no P-state or link speed
	 * changes have occurred). */

	/*New formula:
	sub_total *= 3*(Fn2xD4[NBFid]+4)/(3+Fn2x94[MemClkFreq])/2 */
	val = Get_NB32_DCT(dev, channel, 0x94);
	/* sub_total div 4 to scale 1/4 MemClk back to MemClk */
	val &= 7;
	if (val >= 3) {
		val <<= 1;
	} else
		val += 3;
	valx = (val) << 2;	/* sub_total div 4 to scale 1/4 MemClk back to MemClk */

	val = get_nb32(p_dct_stat->dev_nbmisc, 0xD4);
	val = ((val & 0x1f) + 4) * 3;

	/* Calculate 1 MemClk + 1 NCLK delay in NCLKs for margin */
	valxx = val << 2;
	valxx /= valx;
	if (valxx % valx)
		valxx++;	/* round up */
	valxx++;		/* add 1NCLK */
	*margin = valxx;	/* one MemClk delay in NCLKs and one additional NCLK */

	val *= sub_total;

	val /= valx;
	if (val % valx)
		val++;		/* round up */

	return val;
}
