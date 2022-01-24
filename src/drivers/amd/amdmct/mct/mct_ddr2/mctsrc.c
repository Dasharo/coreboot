/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/x86/cr.h>
#include <cpu/amd/msr.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>

/******************************************************************************
 Description: receiver En and DQS Timing Training feature for DDR 2 MCT
******************************************************************************/

static void dqs_train_rcvr_en_sw(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 pass);
static u8 mct_save_pass_rcv_en_dly_d(struct DCTStatStruc *p_dct_stat,
					u8 rcvr_en_dly, u8 channel,
					u8 receiver, u8 pass);
static u8 mct_compare_test_pattern_qw0_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u32 addr, u8 channel,
					u8 pattern, u8 pass);
static void mct_init_dqs_pos_4_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
					 struct DCTStatStruc *p_dct_stat);
static void init_dqs_pos_4_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 channel);
static void calc_ecc_dqs_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 channel);
static void mct_set_final_rcvr_en_dly_d(struct DCTStatStruc *p_dct_stat,
				u8 rcvr_en_dly, u8 where,
				u8 channel, u8 receiver,
				u32 dev, u32 index_reg,
				u8 addl_index, u8 pass);
static void mct_set_max_latency_d(struct DCTStatStruc *p_dct_stat, u8 channel, u8 dqs_rcv_en_dly);
static void fence_dyn_training_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_disable_dqs_rcv_en_d(struct DCTStatStruc *p_dct_stat);

/* Warning:  These must be located so they do not cross a logical 16-bit segment boundary! */
const u32 test_pattern_0_d[] = {
	0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
	0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
	0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
	0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
};
const u32 test_pattern_1_d[] = {
	0x55555555, 0x55555555, 0x55555555, 0x55555555,
	0x55555555, 0x55555555, 0x55555555, 0x55555555,
	0x55555555, 0x55555555, 0x55555555, 0x55555555,
	0x55555555, 0x55555555, 0x55555555, 0x55555555,
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
	u32 *p_A;
	u32 *p_B;
	u8 i;

	buf_a = (u32 *)(((u32)buffer + 0x10) & (0xfffffff0));
	buf_b = buf_a + 32; //??
	p_A = (u32 *)setup_dqs_pattern_1_pass_b(pass);
	p_B = (u32 *)setup_dqs_pattern_1_pass_a(pass);

	for (i = 0; i < 16; i++) {
		buf_a[i] = p_A[i];
		buf_b[i] = p_B[i];
	}

	p_dct_stat->ptr_pattern_buf_a = (u32)buf_a;
	p_dct_stat->ptr_pattern_buf_b = (u32)buf_b;
}


void mct_train_rcvr_en_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 pass)
{
	if (mct_check_number_of_dqs_rcv_en_1_pass(pass))
		dqs_train_rcvr_en_sw(p_mct_stat, p_dct_stat, pass);
}


static void dqs_train_rcvr_en_sw(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 pass)
{
	u8 channel, rcvr_en_dly, rcvr_en_dly_rmin;
	u8 test_0, test_1, curr_test, curr_test_side_0, curr_test_side_1;
	u8 ctlr_max_delay, _2_ranks, pattern_a, pattern_b;
	u8 addl_index = 0;
	u8 receiver;
	u8 _disable_dram_ecc = 0, _wrap_32_dis = 0, _sse2 = 0;
	u8 rcvr_en_dly_limit, final_value, max_delay_ch[2];
	u32 test_addr_0, test_addr_1, test_addr_0_b, test_addr_1_b;
	u32 pattern_buffer[64 + 4]; /* FIXME: need increase 8? */
	u32 errors;

	u32 val;
	u32 reg;
	u32 dev;
	u32 index_reg;
	u32 ch_start, ch_end, ch;
	u32 msr;
	CRx_TYPE cr4;
	u32 lo, hi;

	u8 valid;
	u32 tmp;
	u8 last_test;

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
		reg = 0x78 + (0x100 * ch);
		val = get_nb32(dev, reg);
		val &= ~(0x3ff << 22);
		val |= (0x0c8 << 22);		/* Max Rd Lat */
		set_nb32(dev, reg, val);
	}

	final_value = 1;
	if (pass == FIRST_PASS) {
		mct_init_dqs_pos_4_rcvr_en_d(p_mct_stat, p_dct_stat);
	} else {
		p_dct_stat->dimm_train_fail = 0;
		p_dct_stat->cs_train_fail = ~p_dct_stat->cs_present;
	}
	print_t("TrainRcvrEn: 1\n");

	cr4 = read_cr4();
	if (cr4 & (1 << 9)) {	/* save the old value */
		_sse2 = 1;
	}
	cr4 |= (1 << 9);	/* OSFXSR enable SSE2 */
	write_cr4(cr4);
	print_t("TrainRcvrEn: 2\n");

	msr = HWCR_MSR;
	_rdmsr(msr, &lo, &hi);
	//FIXME: Why use SSEDIS
	if (lo & (1 << 17)) {	/* save the old value */
		_wrap_32_dis = 1;
	}
	lo |= (1 << 17);	/* HWCR.wrap32dis */
	lo &= ~(1 << 15);	/* SSEDIS */
	_wrmsr(msr, lo, hi);	/* Setting wrap32dis allows 64-bit memory references in real mode */
	print_t("TrainRcvrEn: 3\n");

	_disable_dram_ecc = mct_disable_dimm_ecc_en_d(p_mct_stat, p_dct_stat);


	if (p_dct_stat->speed == 1) {
		p_dct_stat->t1000 = 5000;	/* get the t1000 figure (cycle time (ns)*1K */
	} else if (p_dct_stat->speed == 2) {
		p_dct_stat->t1000 = 3759;
	} else if (p_dct_stat->speed == 3) {
		p_dct_stat->t1000 = 3003;
	} else if (p_dct_stat->speed == 4) {
		p_dct_stat->t1000 = 2500;
	} else if (p_dct_stat->speed  == 5) {
		p_dct_stat->t1000 = 1876;
	} else {
		p_dct_stat->t1000 = 0;
	}

	setup_rcvr_pattern(p_mct_stat, p_dct_stat, pattern_buffer, pass);
	print_t("TrainRcvrEn: 4\n");

	errors = 0;
	dev = p_dct_stat->dev_dct;
	ctlr_max_delay = 0;

	for (channel = 0; channel < 2; channel++) {
		print_debug_dqs("\tTrainRcvEn51: node ", p_dct_stat->node_id, 1);
		print_debug_dqs("\tTrainRcvEn51: channel ", channel, 1);
		p_dct_stat->channel = channel;

		max_delay_ch[channel] = 0;
		index_reg = 0x98 + 0x100 * channel;

		receiver = mct_init_receiver_d(p_dct_stat, channel);
		/* There are four receiver pairs, loosely associated with chipselects. */
		for (; receiver < 8; receiver += 2) {
			addl_index = (receiver >> 1) * 3 + 0x10;
			last_test = DQS_FAIL;

			/* mct_ModifyIndex_D */
			rcvr_en_dly_rmin = rcvr_en_dly_limit = 0xff;

			print_debug_dqs("\t\tTrainRcvEnd52: index ", addl_index, 2);

			if (!mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel, receiver)) {
				print_t("\t\t\tRank not enabled_D\n");
				continue;
			}

			test_addr_0 = mct_get_rcvr_sys_addr_d(p_mct_stat, p_dct_stat, channel, receiver, &valid);
			if (!valid) {	/* Address not supported on current CS */
				print_t("\t\t\tAddress not supported on current CS\n");
				continue;
			}

			test_addr_0_b = test_addr_0 + (BIG_PAGE_X8_RJ8 << 3);

			if (mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel, receiver + 1)) {
				test_addr_1 = mct_get_rcvr_sys_addr_d(p_mct_stat, p_dct_stat, channel, receiver + 1, &valid);
				if (!valid) {	/* Address not supported on current CS */
					print_t("\t\t\tAddress not supported on current CS+1\n");
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

			/*
			 * Get starting rcvr_en_dly value
			 */
			rcvr_en_dly = mct_get_start_rcvr_en_dly_1_pass(pass);

			/* mct_GetInitFlag_D*/
			if (pass == FIRST_PASS) {
				p_dct_stat->dqs_rcv_en_pass = 0;
			} else {
				p_dct_stat->dqs_rcv_en_pass = 0xFF;
			}
			p_dct_stat->dqs_rcv_en_saved = 0;


			while (rcvr_en_dly < rcvr_en_dly_limit) {	/* sweep Delay value here */
				print_debug_dqs("\t\t\tTrainRcvEn541: rcvr_en_dly ", rcvr_en_dly, 3);

				/* callback not required
				if (mct_adjust_delay_d(p_dct_stat, rcvr_en_dly))
					goto skipDly;
				*/

				/* Odd steps get another pattern such that even
				 and odd steps alternate. The pointers to the
				 patterns will be swaped at the end of the loop
				 so that they correspond. */
				if (rcvr_en_dly & 1) {
					pattern_a = 1;
					pattern_b = 0;
				} else {
					/* Even step */
					pattern_a = 0;
					pattern_b = 1;
				}

				mct_write_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_0, pattern_a); /* rank 0 of DIMM, testpattern 0 */
				mct_write_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_0_b, pattern_b); /* rank 0 of DIMM, testpattern 1 */
				if (_2_ranks) {
					mct_write_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_1, pattern_a); /*rank 1 of DIMM, testpattern 0 */
					mct_write_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_1_b, pattern_b); /*rank 1 of DIMM, testpattern 1 */
				}

				mct_set_rcvr_en_dly_d(p_dct_stat, rcvr_en_dly, 0, channel, receiver, dev, index_reg, addl_index, pass);

				curr_test = DQS_FAIL;
				curr_test_side_0 = DQS_FAIL;
				curr_test_side_1 = DQS_FAIL;

				mct_read_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_0);	/*cache fills */
				test_0 = mct_compare_test_pattern_qw0_d(p_mct_stat, p_dct_stat, test_addr_0, channel, pattern_a, pass);/* ROM vs cache compare */
				proc_iocl_flush_d(test_addr_0);
				reset_dct_wr_ptr_d(dev, index_reg, addl_index);

				print_debug_dqs("\t\t\tTrainRcvEn542: test_0 result ", test_0, 3);

				// != 0x00 mean pass

				if (test_0 == DQS_PASS) {
					mct_read_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_0_b);	/*cache fills */
					/* ROM vs cache compare */
					test_1 = mct_compare_test_pattern_qw0_d(p_mct_stat, p_dct_stat, test_addr_0_b, channel, pattern_b, pass);
					proc_iocl_flush_d(test_addr_0_b);
					reset_dct_wr_ptr_d(dev, index_reg, addl_index);

					print_debug_dqs("\t\t\tTrainRcvEn543: test_1 result ", test_1, 3);

					if (test_1 == DQS_PASS) {
						curr_test_side_0 = DQS_PASS;
					}
				}
				if (_2_ranks) {
					mct_read_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_1);	/*cache fills */
					/* ROM vs cache compare */
					test_0 = mct_compare_test_pattern_qw0_d(p_mct_stat, p_dct_stat, test_addr_1, channel, pattern_a, pass);
					proc_iocl_flush_d(test_addr_1);
					reset_dct_wr_ptr_d(dev, index_reg, addl_index);

					print_debug_dqs("\t\t\tTrainRcvEn544: test_0 result ", test_0, 3);

					if (test_0 == DQS_PASS) {
						mct_read_1l_test_pattern_d(p_mct_stat, p_dct_stat, test_addr_1_b);	/*cache fills */
						/* ROM vs cache compare */
						test_1 = mct_compare_test_pattern_qw0_d(p_mct_stat, p_dct_stat, test_addr_1_b, channel, pattern_b, pass);
						proc_iocl_flush_d(test_addr_1_b);
						reset_dct_wr_ptr_d(dev, index_reg, addl_index);

						print_debug_dqs("\t\t\tTrainRcvEn545: test_1 result ", test_1, 3);
						if (test_1 == DQS_PASS) {
							curr_test_side_1 = DQS_PASS;
						}
					}
				}

				if (_2_ranks) {
					if ((curr_test_side_0 == DQS_PASS) && (curr_test_side_1 == DQS_PASS)) {
						curr_test = DQS_PASS;
					}
				} else if (curr_test_side_0 == DQS_PASS) {
					curr_test = DQS_PASS;
				}


				/* record first pass DqsRcvEn to stack */
				valid = mct_save_pass_rcv_en_dly_d(p_dct_stat, rcvr_en_dly, channel, receiver, pass);

				/* Break(1:RevF,2:DR) or not(0) FIXME: This comment deosn't make sense */
				if (valid == 2 || (last_test == DQS_FAIL && valid == 1)) {
					rcvr_en_dly_rmin = rcvr_en_dly;
					break;
				}

				last_test = curr_test;

				/* swap the rank 0 pointers */
				tmp = test_addr_0;
				test_addr_0 = test_addr_0_b;
				test_addr_0_b = tmp;

				/* swap the rank 1 pointers */
				tmp = test_addr_1;
				test_addr_1 = test_addr_1_b;
				test_addr_1_b = tmp;

				print_debug_dqs("\t\t\tTrainRcvEn56: rcvr_en_dly ", rcvr_en_dly, 3);

				rcvr_en_dly++;

			}	/* while rcvr_en_dly */

			print_debug_dqs("\t\tTrainRcvEn61: rcvr_en_dly ", rcvr_en_dly, 2);
			print_debug_dqs("\t\tTrainRcvEn61: rcvr_en_dly_rmin ", rcvr_en_dly_rmin, 3);
			print_debug_dqs("\t\tTrainRcvEn61: rcvr_en_dly_limit ", rcvr_en_dly_limit, 3);
			if (rcvr_en_dly_rmin == rcvr_en_dly_limit) {
				/* no passing window */
				p_dct_stat->err_status |= 1 << SB_NO_RCVR_EN;
				errors |= 1 << SB_NO_RCVR_EN;
				p_dct_stat->err_code = SC_FATAL_ERR;
			}

			if (rcvr_en_dly > (rcvr_en_dly_limit - 1)) {
				/* passing window too narrow, too far delayed*/
				p_dct_stat->err_status |= 1 << SB_SMALL_RCVR;
				errors |= 1 << SB_SMALL_RCVR;
				p_dct_stat->err_code = SC_FATAL_ERR;
				rcvr_en_dly = rcvr_en_dly_limit - 1;
				p_dct_stat->cs_train_fail |= 1 << receiver;
				p_dct_stat->dimm_train_fail |= 1 << (receiver + channel);
			}

			// CHB_D0_B0_RCVRDLY set in mct_average_rcvr_en_dly_pass
			mct_average_rcvr_en_dly_pass(p_dct_stat, rcvr_en_dly, rcvr_en_dly_limit, channel, receiver, pass);

			mct_set_final_rcvr_en_dly_d(p_dct_stat, rcvr_en_dly, final_value, channel, receiver, dev, index_reg, addl_index, pass);

			if (p_dct_stat->err_status & (1 << SB_SMALL_RCVR)) {
				errors |= 1 << SB_SMALL_RCVR;
			}

			rcvr_en_dly += PASS_1_MEM_CLK_DLY;
			if (rcvr_en_dly > ctlr_max_delay) {
				ctlr_max_delay = rcvr_en_dly;
			}

		}	/* while receiver */

		max_delay_ch[channel] = ctlr_max_delay;
	}	/* for channel */

	ctlr_max_delay = max_delay_ch[0];
	if (max_delay_ch[1] > ctlr_max_delay)
		ctlr_max_delay = max_delay_ch[1];

	for (channel = 0; channel < 2; channel++) {
		mct_set_max_latency_d(p_dct_stat, channel, ctlr_max_delay); /* program Ch A/B max_async_lat to correspond with max delay */
	}

	reset_dct_wr_ptr_d(dev, index_reg, addl_index);

	if (_disable_dram_ecc) {
		mct_enable_dimm_ecc_en_d(p_mct_stat, p_dct_stat, _disable_dram_ecc);
	}

	if (pass == FIRST_PASS) {
		/*Disable DQSRcvrEn training mode */
		print_t("TrainRcvrEn: mct_disable_dqs_rcv_en_d\n");
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
		u8 channel;
		printk(BIOS_DEBUG, "TrainRcvrEn: ch_max_rd_lat:\n");
		for (channel = 0; channel < 2; channel++) {
			printk(BIOS_DEBUG, "channel: %02x: %02x\n", channel, p_dct_stat->ch_max_rd_lat[channel]);
		}
	}
#endif

#if DQS_TRAIN_DEBUG > 0
	{
		u8 val;
		u8 channel, receiver;
		u8 i;
		u8 *p;

		printk(BIOS_DEBUG, "TrainRcvrEn: ch_d_b_rcvr_dly:\n");
		for (channel = 0; channel < 2; channel++) {
			printk(BIOS_DEBUG, "channel: %02x\n", channel);
			for (receiver = 0; receiver < 8; receiver+=2) {
				printk(BIOS_DEBUG, "\t\tReceiver: %02x: ", receiver);
				p = p_dct_stat->persistentData.ch_d_b_rcvr_dly[channel][receiver >> 1];
				for (i = 0; i < 8; i++) {
					val  = p[i];
					printk(BIOS_DEBUG, "%02x ", val);
				}
			printk(BIOS_DEBUG, "\n");
			}
		}
	}
#endif

	print_tx("TrainRcvrEn: status ", p_dct_stat->status);
	print_tx("TrainRcvrEn: err_status ", p_dct_stat->err_status);
	print_tx("TrainRcvrEn: err_code ", p_dct_stat->err_code);
	print_t("TrainRcvrEn: Done\n");
}


u8 mct_init_receiver_d(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	if (p_dct_stat->dimm_valid_dct[dct] == 0) {
		return 8;
	} else {
		return 0;
	}
}


static void mct_set_final_rcvr_en_dly_d(struct DCTStatStruc *p_dct_stat, u8 rcvr_en_dly, u8 where, u8 channel, u8 receiver, u32 dev, u32 index_reg, u8 addl_index, u8 pass/*, u8 *p*/)
{
	/*
	 * Program final DqsRcvEnDly to additional index for DQS receiver
	 *  enabled delay
	 */
	mct_set_rcvr_en_dly_d(p_dct_stat, rcvr_en_dly, where, channel, receiver, dev, index_reg, addl_index, pass);
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
		reg = 0x78 + 0x100 * ch;
		val = get_nb32(dev, reg);
		val &= ~(1 << DQS_RCV_EN_TRAIN);
		set_nb32(dev, reg, val);
	}
}


/* mct_ModifyIndex_D
 * Function only used once so it was inlined.
 */


/* mct_GetInitFlag_D
 * Function only used once so it was inlined.
 */


void mct_set_rcvr_en_dly_d(struct DCTStatStruc *p_dct_stat, u8 rcvr_en_dly,
			u8 final_value, u8 channel, u8 receiver, u32 dev,
			u32 index_reg, u8 addl_index, u8 pass)
{
	u32 index;
	u8 i;
	u8 *p;
	u32 val;

	if (rcvr_en_dly == 0xFE) {
		/*set the boudary flag */
		p_dct_stat->status |= 1 << SB_DQS_RCV_LIMIT;
	}

	/* DimmOffset not needed for ch_d_b_rcvr_dly array */


	for (i = 0; i < 8; i++) {
		if (final_value) {
			/*calculate dimm offset */
			p = p_dct_stat->persistentData.ch_d_b_rcvr_dly[channel][receiver >> 1];
			rcvr_en_dly = p[i];
		}

		/* if flag = 0, set DqsRcvEn value to reg. */
		/* get the register index from table */
		index = Table_DQSRcvEn_Offset[i >> 1];
		index += addl_index;	/* DIMMx DqsRcvEn byte0 */
		val = get_nb32_index_wait(dev, index_reg, index);
		if (i & 1) {
			/* odd byte lane */
			val &= ~(0xFF << 16);
			val |= (rcvr_en_dly << 16);
		} else {
			/* even byte lane */
			val &= ~0xFF;
			val |= rcvr_en_dly;
		}
		set_nb32_index_wait(dev, index_reg, index, val);
	}

}

static void mct_set_max_latency_d(struct DCTStatStruc *p_dct_stat, u8 channel, u8 dqs_rcv_en_dly)
{
	u32 dev;
	u32 reg;
	u16 sub_total;
	u32 index_reg;
	u32 reg_off;
	u32 val;
	u32 valx;

	if (p_dct_stat->ganged_mode)
		channel = 0;

	dev = p_dct_stat->dev_dct;
	reg_off = 0x100 * channel;
	index_reg = 0x98 + reg_off;

	/* Multiply the CAS Latency by two to get a number of 1/2 MEMCLKs units.*/
	val = get_nb32(dev, 0x88 + reg_off);
	sub_total = ((val & 0x0f) + 1) << 1;	/* sub_total is 1/2 Memclk unit */

	/* If registered DIMMs are being used then
	 *  add 1 MEMCLK to the sub-total.
	 */
	val = get_nb32(dev, 0x90 + reg_off);
	if (!(val & (1 << UN_BUFF_DIMM)))
		sub_total += 2;

	/* If the address prelaunch is setup for 1/2 MEMCLKs then
	 *  add 1, else add 2 to the sub-total.
	 *  if (AddrCmdSetup || CsOdtSetup || CkeSetup) then K := K + 2;
	 */
	val = get_nb32_index_wait(dev, index_reg, 0x04);
	if (!(val & 0x00202020))
		sub_total += 1;
	else
		sub_total += 2;

	/* If the F2x[1, 0]78[RdPtrInit] field is 4, 5, 6 or 7 MEMCLKs,
	 * then add 4, 3, 2, or 1 MEMCLKs, respectively to the sub-total. */
	val = get_nb32(dev, 0x78 + reg_off);
	sub_total += 8 - (val & 0x0f);

	/* Convert bits 7-5 (also referred to as the course delay) of
	 * the current (or worst case) DQS receiver enable delay to
	 * 1/2 MEMCLKs units, rounding up, and add this to the sub-total.
	 */
	sub_total += dqs_rcv_en_dly >> 5;	/*BOZO-no rounding up */

	/* Add 5.5 to the sub-total. 5.5 represents part of the
	 * processor specific constant delay value in the DRAM
	 * clock domain.
	 */
	sub_total <<= 1;		/*scale 1/2 MemClk to 1/4 MemClk */
	sub_total += 11;		/*add 5.5 1/2MemClk */

	/* Convert the sub-total (in 1/2 MEMCLKs) to northbridge
	 * clocks (NCLKs) as follows (assuming DDR400 and assuming
	 * that no P-state or link speed changes have occurred).
	 */

	/* New formula:
	 * sub_total *= 3*(Fn2xD4[NBFid]+4)/(3+Fn2x94[MemClkFreq])/2 */
	val = get_nb32(dev, 0x94 + reg_off);

	/* sub_total div 4 to scale 1/4 MemClk back to MemClk */
	val &= 7;
	if (val == 4) {
		val++;		/* adjust for DDR2-1066 */
	}
	valx = (val + 3) << 2;

	val = get_nb32(p_dct_stat->dev_nbmisc, 0xD4);
	sub_total *= ((val & 0x1f) + 4) * 3;

	sub_total /= valx;
	if (sub_total % valx) {	/* round up */
		sub_total++;
	}

	/* Add 5 NCLKs to the sub-total. 5 represents part of the
	 * processor specific constant value in the northbridge
	 * clock domain.
	 */
	sub_total += 5;

	p_dct_stat->ch_max_rd_lat[channel] = sub_total;
	if (p_dct_stat->ganged_mode) {
		p_dct_stat->ch_max_rd_lat[1] = sub_total;
	}

	/* Program the F2x[1, 0]78[MaxRdLatency] register with
	 * the total delay value (in NCLKs).
	 */

	reg = 0x78 + reg_off;
	val = get_nb32(dev, reg);
	val &= ~(0x3ff << 22);
	val |= (sub_total & 0x3ff) << 22;

	/* program MaxRdLatency to correspond with current delay */
	set_nb32(dev, reg, val);
}


static u8 mct_save_pass_rcv_en_dly_d(struct DCTStatStruc *p_dct_stat,
			u8 rcvr_en_dly, u8 channel,
			u8 receiver, u8 pass)
{
	u8 i;
	u8 mask_Saved, mask_Pass;
	u8 *p;

	/* calculate dimm offset
	 * not needed for ch_d_b_rcvr_dly array
	 */

	/* cmp if there has new DqsRcvEnDly to be recorded */
	mask_Pass = p_dct_stat->dqs_rcv_en_pass;

	if (pass == SECOND_PASS) {
		mask_Pass = ~mask_Pass;
	}

	mask_Saved = p_dct_stat->dqs_rcv_en_saved;
	if (mask_Pass != mask_Saved) {

		/* find desired stack offset according to channel/dimm/byte */
		if (pass == SECOND_PASS) {
			// FIXME: SECOND_PASS is never used for Barcelona p = p_dct_stat->persistentData.CH_D_B_RCVRDLY_1[channel][receiver>>1];
			p = 0; // Keep the compiler happy.
		} else {
			mask_Saved &= mask_Pass;
			p = p_dct_stat->persistentData.ch_d_b_rcvr_dly[channel][receiver >> 1];
		}
		for (i = 0; i < 8; i++) {
			/* cmp per byte lane */
			if (mask_Pass & (1 << i)) {
				if (!(mask_Saved & (1 << i))) {
					/* save RcvEnDly to stack, according to
					the related Dimm/byte lane */
					p[i] = (u8)rcvr_en_dly;
					mask_Saved |= 1 << i;
				}
			}
		}
		p_dct_stat->dqs_rcv_en_saved = mask_Saved;
	}
	return mct_save_rcv_en_dly_d_1_pass(p_dct_stat, pass);
}


static u8 mct_compare_test_pattern_qw0_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat,
					u32 addr, u8 channel,
					u8 pattern, u8 pass)
{
	/* Compare only the first beat of data.  Since target addrs are cache
	 * line aligned, the channel parameter is used to determine which
	 * cache QW to compare.
	 */

	u8 *test_buf;
	u8 i;
	u8 result;
	u8 value;


	if (pass == FIRST_PASS) {
		if (pattern == 1) {
			test_buf = (u8 *)test_pattern_1_d;
		} else {
			test_buf = (u8 *)test_pattern_0_d;
		}
	} else {		// Second pass
		test_buf = (u8 *)test_pattern_2_d;
	}

	set_upper_fs_base(addr);
	addr <<= 8;

	if ((p_dct_stat->status & (1 << SB_128_BIT_MODE)) && channel) {
		addr += 8;	/* second channel */
		test_buf += 8;
	}

	print_debug_dqs_pair("\t\t\t\t\t\t  test_buf = ", (u32)test_buf, "  |  addr_lo = ", addr,  4);
	for (i = 0; i < 8; i++) {
		value = read32_fs(addr);
		print_debug_dqs_pair("\t\t\t\t\t\t\t\t ", test_buf[i], "  |  ", value, 4);

		if (value == test_buf[i]) {
			p_dct_stat->dqs_rcv_en_pass |= (1 << i);
		} else {
			p_dct_stat->dqs_rcv_en_pass &= ~(1 << i);
		}
	}

	result = DQS_FAIL;

	if (pass == FIRST_PASS) {
		/* if first pass, at least one byte lane pass
		 * ,then DQS_PASS = 1 and will set to related reg.
		 */
		if (p_dct_stat->dqs_rcv_en_pass != 0) {
			result = DQS_PASS;
		} else {
			result = DQS_FAIL;
		}

	} else {
		/* if second pass, at least one byte lane fail
		 * ,then DQS_FAIL = 1 and will set to related reg.
		 */
		if (p_dct_stat->dqs_rcv_en_pass != 0xFF) {
			result = DQS_FAIL;
		} else {
			result = DQS_PASS;
		}
	}

	/* if second pass, we can't find the fail until FFh,
	 * then let it fail to save the final delay
	 */
	if ((pass == SECOND_PASS) && (p_dct_stat->status & (1 << SB_DQS_RCV_LIMIT))) {
		result = DQS_FAIL;
		p_dct_stat->dqs_rcv_en_pass = 0;
	}

	/* second pass needs to be inverted
	 * FIXME? this could be inverted in the above code to start with...
	 */
	if (pass == SECOND_PASS) {
		if (result == DQS_PASS) {
			result = DQS_FAIL;
		} else if (result == DQS_FAIL) { /* FIXME: doesn't need to be else if */
			result = DQS_PASS;
		}
	}


	return result;
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
	u8 dn = 2; // TODO: Rev C could be 4
	u32 dev = p_dct_stat->dev_dct;
	u32 index_reg = 0x98 + 0x100 * channel;


	// FIXME: add Cx support
	dword = 0x00000000;
	for (i = 1; i <= 3; i++) {
		for (j = 0; j < dn; j++)
			/* DIMM0 Write Data Timing Low */
			/* DIMM0 Write ECC Timing */
			set_nb32_index_wait(dev, index_reg, i + 0x100 * j, dword);
	}

	/* errata #180 */
	dword = 0x2f2f2f2f;
	for (i = 5; i <= 6; i++) {
		for (j = 0; j < dn; j++)
			/* DIMM0 Read DQS Timing Control Low */
			set_nb32_index_wait(dev, index_reg, i + 0x100 * j, dword);
	}

	dword = 0x0000002f;
	for (j = 0; j < dn; j++)
		/* DIMM0 Read DQS ECC Timing Control */
		set_nb32_index_wait(dev, index_reg, 7 + 0x100 * j, dword);
}


void set_ecc_dqs_rcvr_en_d(struct DCTStatStruc *p_dct_stat, u8 channel)
{
	u32 dev;
	u32 index_reg;
	u32 index;
	u8 chip_sel;
	u8 *p;
	u32 val;

	dev = p_dct_stat->dev_dct;
	index_reg = 0x98 + channel * 0x100;
	index = 0x12;
	p = p_dct_stat->persistentData.ch_d_bc_rcvr_dly[channel];
	print_debug_dqs("\t\tSetEccDQSRcvrPos: channel ", channel,  2);
	for (chip_sel = 0; chip_sel < MAX_CS_SUPPORTED; chip_sel += 2) {
		val = p[chip_sel >> 1];
		set_nb32_index_wait(dev, index_reg, index, val);
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

	ecc_dqs_like = p_dct_stat->ch_ecc_dqs_like[channel];
	ecc_dqs_scale = p_dct_stat->ch_ecc_dqs_scale[channel];

	for (chip_sel = 0; chip_sel < MAX_CS_SUPPORTED; chip_sel += 2) {
		if (mct_rcvr_rank_enabled_d(p_mct_stat, p_dct_stat, channel, chip_sel)) {
			u8 *p;
			p = p_dct_stat->persistentData.ch_d_b_rcvr_dly[channel][chip_sel >> 1];

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
			val >>= 8; // /256

			if (val0 > val1) {
				val -= val1;
			} else {
				val += val0;
			}

			p_dct_stat->persistentData.ch_d_bc_rcvr_dly[channel][chip_sel >> 1] = val;
		}
	}
	set_ecc_dqs_rcvr_en_d(p_dct_stat, channel);
}

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
			struct DCTStatStruc *p_dct_stat_a)
{

	u8 node = 0;
	struct DCTStatStruc *p_dct_stat;

	// FIXME: skip for Ax
	while (node < MAX_NODES_SUPPORTED) {
		p_dct_stat = p_dct_stat_a + node;

		if (p_dct_stat->dct_sys_limit) {
			fence_dyn_training_d(p_mct_stat, p_dct_stat, 0);
			fence_dyn_training_d(p_mct_stat, p_dct_stat, 1);
		}
		node++;
	}
}


static void fence_dyn_training_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u16 av_rec_value;
	u32 val;
	u32 dev;
	u32 index_reg = 0x98 + 0x100 * dct;
	u32 index;

	/* BIOS first programs a seed value to the phase recovery engine
	 *  (recommended 19) registers.
	 * Dram Phase Recovery Control Register (F2x[1,0]9C_x[51:50] and
	 * F2x[1,0]9C_x52.) .
	 */

	dev = p_dct_stat->dev_dct;
	for (index = 0x50; index <= 0x52; index ++) {
		val = (FENCE_TRN_FIN_DLY_SEED & 0x1F);
		if (index != 0x52) {
			val |= val << 8 | val << 16 | val << 24;
		}
		set_nb32_index_wait(dev, index_reg, index, val);
	}


	/* Set F2x[1,0]9C_x08[PHY_FENCE_TR_EN]=1. */
	val = get_nb32_index_wait(dev, index_reg, 0x08);
	val |= 1 << PHY_FENCE_TR_EN;
	set_nb32_index_wait(dev, index_reg, 0x08, val);

	/* Wait 200 MEMCLKs. */
	mct_wait(50000);		/* wait 200us */

	/* Clear F2x[1,0]9C_x08[PHY_FENCE_TR_EN]=0. */
	val = get_nb32_index_wait(dev, index_reg, 0x08);
	val &= ~(1 << PHY_FENCE_TR_EN);
	set_nb32_index_wait(dev, index_reg, 0x08, val);

	/* BIOS reads the phase recovery engine registers
	 * F2x[1,0]9C_x[51:50] and F2x[1,0]9C_x52. */
	av_rec_value = 0;
	for (index = 0x50; index <= 0x52; index ++) {
		val = get_nb32_index_wait(dev, index_reg, index);
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
	av_rec_value -= 8;
	val = get_nb32_index_wait(dev, index_reg, 0x0C);
	val &= ~(0x1F << 16);
	val |= (av_rec_value & 0x1F) << 16;
	set_nb32_index_wait(dev, index_reg, 0x0C, val);

	/* Rewrite F2x[1,0]9C_x04-DRAM Address/Command Timing Control Register
	 * delays (both channels). */
	val = get_nb32_index_wait(dev, index_reg, 0x04);
	set_nb32_index_wait(dev, index_reg, 0x04, val);
}


void mct_wait(u32 cycles)
{
	u32 saved;
	u32 hi, lo, msr;

	/* Wait # of 50ns cycles
	   This seems like a hack to me...  */

	cycles <<= 3;		/* x8 (number of 1.25ns ticks) */

	msr = 0x10;			/* TSC */
	_rdmsr(msr, &lo, &hi);
	saved = lo;
	do {
		_rdmsr(msr, &lo, &hi);
	} while (lo - saved < cycles);
}
