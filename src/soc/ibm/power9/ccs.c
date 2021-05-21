/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <timer.h>
#include <endian.h>

#include "istep_13_scom.h"

static inline uint64_t reverse_bits(uint64_t x)
{
	x = swab64(x);		// reverse bytes
	x = (x & 0xF0F0F0F0F0F0F0F0) >> 4 |
	    (x & 0x0F0F0F0F0F0F0F0F) << 4;	// reverse nibbles in bytes
	x = (x & 0x1111111111111111) << 3 |
	    (x & 0x2222222222222222) << 1 |
	    (x & 0x4444444444444444) >> 1 |
	    (x & 0x8888888888888888) >> 3;	// reverse bits in nibbles

	return x;
}

/* 32 total, but last one is reserved for ccs_execute() */
#define MAX_CCS_INSTR	31

static unsigned instr;
static uint64_t total_cycles;

/* TODO: 4R, CID? */
void ccs_add_instruction(chiplet_id_t id, mrs_cmd_t mrs, uint8_t csn,
                         uint8_t cke, uint16_t idles)
{
	/*
	 * CSS_INST_ARR0_n layout (bits from MRS):
	 * [0-13]  A0-A13
	 * [14]    A17
	 * [15]    BG1
	 * [17-18] BA0-1
	 * [19]    BG0
	 * [21]    A16
	 * [22]    A15
	 * [23]    A14
	 */
	uint64_t mrs64 = (reverse_bits(mrs) & PPC_BITMASK(0, 13)) | /* A0-A13 */
	                 PPC_SHIFT(mrs & (1<<14), 23 + 14) |        /* A14 */
	                 PPC_SHIFT(mrs & (1<<15), 22 + 15) |        /* A15 */
	                 PPC_SHIFT(mrs & (1<<16), 21 + 16) |        /* A16 */
	                 PPC_SHIFT(mrs & (1<<17), 14 + 17) |        /* A17 */
	                 PPC_SHIFT(mrs & (1<<20), 17 + 20) |        /* BA0 */
	                 PPC_SHIFT(mrs & (1<<21), 18 + 21) |        /* BA1 */
	                 PPC_SHIFT(mrs & (1<<22), 19 + 22) |        /* BG0 */
	                 PPC_SHIFT(mrs & (1<<23), 15 + 23);         /* BA1 */

	/* MC01.MCBIST.CCS.CCS_INST_ARR0_n
	      [all]   0
	      // "ACT is high. It's a no-care in the spec but it seems to raise
	      // questions when people look at the trace, so lets set it high."
	      [20]    CCS_INST_ARR0_00_CCS_DDR_ACTN =     1
	      // "CKE is high Note: P8 set all 4 of these high - not sure if that's
	      // correct. BRS"
	      [24-27] CCS_INST_ARR0_00_CCS_DDR_CKE =      cke
	      [32-33] CCS_INST_ARR0_00_CCS_DDR_CSN_0_1 =  csn[0:1]
	      [36-37] CCS_INST_ARR0_00_CCS_DDR_CSN_2_3 =  csn[2:3]
	*/
	write_scom_for_chiplet(id, CCS_INST_ARR0_00 + instr,
	                       mrs64 | PPC_BIT(CCS_INST_ARR0_00_CCS_DDR_ACTN) |
	                       PPC_SHIFT(cke & 0xF, CCS_INST_ARR0_00_CCS_DDR_CKE) |
	                       PPC_SHIFT((csn >> 2) & 3, CCS_INST_ARR0_00_CCS_DDR_CSN_0_1) |
	                       PPC_SHIFT(csn & 3, CCS_INST_ARR0_00_CCS_DDR_CSN_2_3));

	/* MC01.MCBIST.CCS.CCS_INST_ARR1_n
	      [all]   0
	      [0-15]  CCS_INST_ARR1_00_IDLES =    idles
	      [59-63] CCS_INST_ARR1_00_GOTO_CMD = instr + 1
	*/
	write_scom_for_chiplet(id, CCS_INST_ARR1_00 + instr,
	                       PPC_SHIFT(idles, CCS_INST_ARR1_00_IDLES) |
	                       PPC_SHIFT(instr + 1, CCS_INST_ARR1_00_GOTO_CMD));

	/*
	 * For the last instruction in the stream we could decrease it by one (final
	 * DES added in ccs_execute()), but subtracting it would take longer than
	 * that one cycle, so leave it.
	 */
	total_cycles += idles;
	instr++;

	if (instr >= MAX_CCS_INSTR) {
		/* Maybe call ccs_execute() here? Would need mca_i... */
		die("CCS instructions overflowed\n");
	}
}

/* This isn't useful for anything but calibration steps, do we want it? */
static void dump_cal_errors(chiplet_id_t id, int mca_i)
{
	/* Stop CCS so it won't mess up with the values */
	write_scom_for_chiplet(id, CCS_CNTLQ, PPC_BIT(CCS_CNTLQ_CCS_STOP));

#if CONFIG(DEBUG_RAM_SETUP)
	int dp;

	for (dp = 0; dp < 5; dp++) {
		printk(BIOS_ERR, "DP %d\n", dp);
		printk(BIOS_ERR, "\t%#16.16llx - RD_VREF_CAL_ERROR\n",
		       dp_mca_read(id, dp, mca_i, DDRPHY_DP16_RD_VREF_CAL_ERROR_P0_0));
		printk(BIOS_ERR, "\t%#16.16llx - DQ_BIT_DISABLE_RP0\n",
		       dp_mca_read(id, dp, mca_i, DDRPHY_DP16_DQ_BIT_DISABLE_RP0_P0_0));
		printk(BIOS_ERR, "\t%#16.16llx - DQS_BIT_DISABLE_RP0\n",
		       dp_mca_read(id, dp, mca_i, DDRPHY_DP16_DQS_BIT_DISABLE_RP0_P0_0));
		printk(BIOS_ERR, "\t%#16.16llx - WR_ERROR0\n",
		       dp_mca_read(id, dp, mca_i, DDRPHY_DP16_WR_ERROR0_P0_0));
		printk(BIOS_ERR, "\t%#16.16llx - RD_STATUS0\n",
		       dp_mca_read(id, dp, mca_i, DDRPHY_DP16_RD_STATUS0_P0_0));
		printk(BIOS_ERR, "\t%#16.16llx - RD_LVL_STATUS2\n",
		       dp_mca_read(id, dp, mca_i, DDRPHY_DP16_RD_LVL_STATUS2_P0_0));
		printk(BIOS_ERR, "\t%#16.16llx - RD_LVL_STATUS0\n",
		       dp_mca_read(id, dp, mca_i, DDRPHY_DP16_RD_LVL_STATUS0_P0_0));
		printk(BIOS_ERR, "\t%#16.16llx - WR_VREF_ERROR0\n",
		       dp_mca_read(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR0_P0_0));
		printk(BIOS_ERR, "\t%#16.16llx - WR_VREF_ERROR1\n",
		       dp_mca_read(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR1_P0_0));
	}

	printk(BIOS_ERR, "%#16.16llx - APB_ERROR_STATUS0\n",
	       mca_read(id, mca_i, DDRPHY_APB_ERROR_STATUS0_P0));

	printk(BIOS_ERR, "%#16.16llx - RC_ERROR_STATUS0\n",
	       mca_read(id, mca_i, DDRPHY_RC_ERROR_STATUS0_P0));

	printk(BIOS_ERR, "%#16.16llx - SEQ_ERROR_STATUS0\n",
	       mca_read(id, mca_i, DDRPHY_SEQ_ERROR_STATUS0_P0));

	printk(BIOS_ERR, "%#16.16llx - WC_ERROR_STATUS0\n",
	       mca_read(id, mca_i, DDRPHY_WC_ERROR_STATUS0_P0));

	printk(BIOS_ERR, "%#16.16llx - PC_ERROR_STATUS0\n",
	       mca_read(id, mca_i, DDRPHY_PC_ERROR_STATUS0_P0));

	printk(BIOS_ERR, "%#16.16llx - PC_INIT_CAL_ERROR\n",
	       mca_read(id, mca_i, DDRPHY_PC_INIT_CAL_ERROR_P0));

	printk(BIOS_ERR, "%#16.16llx - PC_INIT_CAL_STATUS\n",
	       mca_read(id, mca_i, DDRPHY_PC_INIT_CAL_STATUS_P0));

	printk(BIOS_ERR, "%#16.16llx - IOM_PHY0_DDRPHY_FIR_REG\n",
	       mca_read(id, mca_i, IOM_PHY0_DDRPHY_FIR_REG));
#endif
	die("CCS execution timeout\n");
}

void ccs_execute(chiplet_id_t id, int mca_i)
{
	uint64_t poll_timeout;
	long time;

	/*
	 * Polling parameters: initial delay is total_cycles/8, no delay between
	 * polls (coreboot API checks in a busy loop, but there is nothing else to
	 * do than wait), poll count is whatever it takes to get to total_cycles
	 * times 4 just in case (won't hurt unless calibration fails anyway).
	 */
	if (total_cycles < 8)
		total_cycles = 8;
	poll_timeout = nck_to_us((total_cycles * 7 * 4) / 8);

	write_scom_for_chiplet(id, CCS_CNTLQ, PPC_BIT(CCS_CNTLQ_CCS_STOP));
	time = wait_us(1, !(read_scom_for_chiplet(id, CCS_STATQ) &
	                    PPC_BIT(CCS_STATQ_CCS_IP)));

	/* Is it always as described below (CKE, CSN) or is it a copy of last instr? */
	/* Final DES - CCS does not wait for IDLES for the last command before
	 * clearing IP (in progress) bit, so we must use one separate DES
	 * instruction at the end.
	MC01.MCBIST.CCS.CCS_INST_ARR0_n
	      [all]   0
	      [20]    CCS_INST_ARR0_00_CCS_DDR_ACTN =     1
	      [24-27] CCS_INST_ARR0_00_CCS_DDR_CKE =      0xf
	      [32-33] CCS_INST_ARR0_00_CCS_DDR_CSN_0_1 =  3
	      [36-37] CCS_INST_ARR0_00_CCS_DDR_CSN_2_3 =  3
	MC01.MCBIST.CCS.CCS_INST_ARR1_n
	      [all]   0
	      [58]    CCS_INST_ARR1_00_CCS_END = 1
	*/
	write_scom_for_chiplet(id, CCS_INST_ARR0_00 + instr,
	                       PPC_BIT(CCS_INST_ARR0_00_CCS_DDR_ACTN) |
	                       PPC_SHIFT(0xF, CCS_INST_ARR0_00_CCS_DDR_CKE) |
	                       PPC_SHIFT(3, CCS_INST_ARR0_00_CCS_DDR_CSN_0_1) |
	                       PPC_SHIFT(3, CCS_INST_ARR0_00_CCS_DDR_CSN_2_3));
	write_scom_for_chiplet(id, CCS_INST_ARR1_00 + instr,
	                       PPC_BIT(CCS_INST_ARR1_00_CCS_END));

	/* Select ports
	MC01.MCBIST.MBA_SCOMFIR.MCB_CNTLQ
	      // Broadcast mode is not supported, set only one bit at a time
	      [2-5]   MCB_CNTLQ_MCBCNTL_PORT_SEL = bitmap with MCA index
	*/
	scom_and_or_for_chiplet(id, MCB_CNTLQ, ~PPC_BITMASK(2, 5), PPC_BIT(2 + mca_i));

	/* Lets go */
	write_scom_for_chiplet(id, CCS_CNTLQ, PPC_BIT(CCS_CNTLQ_CCS_START));

	/* With microsecond resolution we are probably wasting a lot of time here. */
	delay_nck(total_cycles/8);

	/* timeout(50*10ns):
	  if MC01.MCBIST.MBA_SCOMFIR.CCS_STATQ[0] (CCS_STATQ_CCS_IP) != 1: break
	  delay(10ns)
	if MC01.MCBIST.MBA_SCOMFIR.CCS_STATQ != 0x40..00: report failure  // only [1] set, others 0
	*/
	time = wait_us(poll_timeout, (udelay(1), !(read_scom_for_chiplet(id, CCS_STATQ) &
	                               PPC_BIT(CCS_STATQ_CCS_IP))));

	/* This isn't useful for anything but calibration steps, do we want it? */
	if (!time)
		dump_cal_errors(id, mca_i);

	printk(BIOS_DEBUG, "CCS took %lld us (%lld us timeout), %d instruction(s)\n",
	       time + nck_to_us(total_cycles/8),
	       poll_timeout + nck_to_us(total_cycles/8), instr);

	if (read_scom_for_chiplet(id, CCS_STATQ) != PPC_BIT(CCS_STATQ_CCS_DONE))
		die("(%#16.16llx) CCS execution error\n", read_scom_for_chiplet(id, CCS_STATQ));

	instr = 0;
	total_cycles = 0;

	/* LRDIMM only */
	// cleanup_from_execute();
}

/*
 * Constant to invert A3-A9, A11, A13, BA0-1, BG0-1. This also changes BG1 to 1,
 * which automatically selects B-side. Note that A17 is not included here.
 */
static const mrs_cmd_t invert = 0xF02BF8;

/*
 * Procedure for sending MRS through CCS
 *
 * We need to remember about two things here:
 * - RDIMM has A-side and B-side, some address bits are inverted for B-side;
 *   side is selected by DBG1 (when mirroring is enabled DBG0 is used for odd
 *   ranks to select side, instead of DBG1)
 * - odd ranks may or may not have mirrored lines, depending on SPD[136].
 *
 * Because of those two reasons we cannot simply repeat MRS data for all sides
 * and ranks, we have to do some juggling instead. Inverting is easy, we just
 * have to XOR with appropriate mask (special case for A17, it is not inverted
 * if it isn't used). Mirroring will require manual bit manipulations.
 *
 * There are no signals that are mirrored but not inverted, which means that
 * the order of those operations doesn't matter.
 */
/* TODO: add support for A17. For now it is blocked in initial SPD parsing. */
void ccs_add_mrs(chiplet_id_t id, mrs_cmd_t mrs, enum rank_selection ranks,
                 int mirror, uint16_t idles)
{
	if (ranks & DIMM0_RANK0) {
		/* DIMM 0, rank 0, side A */
		/*
		 * "Not sure if we can get tricky here and only delay after the b-side MR.
		 * The question is whether the delay is needed/assumed by the register or is
		 * purely a DRAM mandated delay. We know we can't go wrong having both
		 * delays but if we can ever confirm that we only need one we can fix this.
		 * BRS"
		 */
		ccs_add_instruction(id, mrs, 0x7, 0xF, idles);

		/* DIMM 0, rank 0, side B - invert A3-A9, A11, A13, A17 (TODO), BA0-1, BG0-1 */
		ccs_add_instruction(id, mrs ^ invert, 0x7, 0xF, idles);
	}

	if (ranks & DIMM0_RANK1) {
		/* DIMM 0, rank 1, side A, mirror if needed */
		if (mirror)
			mrs = ddr4_mrs_mirror_pins(mrs);

		ccs_add_instruction(id, mrs, 0xB, 0xF, idles);

		/* DIMM 0, rank 1, side B - MRS is already mirrored, just invert it */
		ccs_add_instruction(id, mrs ^ invert, 0xB, 0xF, idles);
	}

	if (ranks & DIMM1_RANK0) {
		/* DIMM 1, rank 0, side A */
		ccs_add_instruction(id, mrs, 0xD, 0xF, idles);

		/* DIMM 1, rank 0, side B - invert A3-A9, A11, A13, A17 (TODO), BA0-1, BG0-1 */
		ccs_add_instruction(id, mrs ^ invert, 0xD, 0xF, idles);
	}

	if (ranks & DIMM1_RANK1) {
		/* DIMM 1, rank 1, side A, mirror if needed */
		if (mirror)
			mrs = ddr4_mrs_mirror_pins(mrs);

		ccs_add_instruction(id, mrs, 0xE, 0xF, idles);

		/* DIMM 1, rank 1, side B - MRS is already mirrored, just invert it */
		ccs_add_instruction(id, mrs ^ invert, 0xE, 0xF, idles);
	}
}

void ccs_phy_hw_step(chiplet_id_t id, int mca_i, int rp, enum cal_config conf,
                     uint64_t step_cycles)
{
	/* MC01.MCBIST.CCS.CCS_INST_ARR0_n
		[all]   0
		// "CKE is high Note: P8 set all 4 of these high - not sure if that's correct. BRS"
		[24-27] CCS_INST_ARR0_00_CCS_DDR_CKE =      0xf
		[32-33] CCS_INST_ARR0_00_CCS_DDR_CSN_0_1 =  3     // Not used by the engine for calibration?
		[36-37] CCS_INST_ARR0_00_CCS_DDR_CSN_2_3 =  3     // Not used by the engine for calibration?
		[56-59] CCS_INST_ARR0_00_CCS_DDR_CAL_TYPE = 0xc
	*/
	write_scom_for_chiplet(id, CCS_INST_ARR0_00 + instr,
	                       PPC_SHIFT(0xF, CCS_INST_ARR0_00_CCS_DDR_CKE) |
	                       PPC_SHIFT(3, CCS_INST_ARR0_00_CCS_DDR_CSN_0_1) |
	                       PPC_SHIFT(3, CCS_INST_ARR0_00_CCS_DDR_CSN_2_3) |
	                       PPC_SHIFT(0xC, CCS_INST_ARR0_00_CCS_DDR_CAL_TYPE));

	/* MC01.MCBIST.CCS.CCS_INST_ARR1_n
		[all]   0
		[53-56] CCS_INST_ARR1_00_DDR_CAL_RANK =           rp
		[57]    CCS_INST_ARR1_00_DDR_CALIBRATION_ENABLE = 1
		[59-63] CCS_INST_ARR1_00_GOTO_CMD =               instr + 1
	*/
	write_scom_for_chiplet(id, CCS_INST_ARR1_00 + instr,
	                       PPC_SHIFT(rp, CCS_INST_ARR1_00_DDR_CAL_RANK) |
	                       PPC_BIT(CCS_INST_ARR1_00_DDR_CALIBRATION_ENABLE) |
	                       PPC_SHIFT(instr + 1, CCS_INST_ARR1_00_GOTO_CMD));

	total_cycles += step_cycles;
	instr++;

	/* Setup calibration config
	IOM0.DDRPHY_PC_INIT_CAL_CONFIG0_P0
		[48-57] i_cal_config            // cal_config is already encoded, don't shift
		[58]    ABORT_ON_CAL_ERROR =  0
		[60+rp] ENA_RANK_PAIR =       1   // So, rp must be [0-3]
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_INIT_CAL_CONFIG0_P0,
	           ~(PPC_BITMASK(48, 58) | PPC_BITMASK(60, 63)),
	           conf | PPC_BIT(ENA_RANK_PAIR_MSB + rp));

	ccs_execute(id, mca_i);
}
