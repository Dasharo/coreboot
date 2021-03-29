/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <cpu/power/vpd_data.h>
#include <console/console.h>
#include <timer.h>

#include "istep_13_scom.h"

static void setup_and_execute_zqcal(int mcs_i, int mca_i, int d)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int mirrored = mca->dimm[d].spd[136] & 1; /* Maybe add this to mca_data_t? */
	mrs_cmd_t cmd = ddr4_get_zqcal_cmd(DDR4_ZQCAL_LONG);
	enum rank_selection ranks;

	if (d == 0) {
		if (mca->dimm[d].mranks == 2)
			ranks = DIMM0_ALL_RANKS;
		else
			ranks = DIMM0_RANK0;
	}
	else {
		if (mca->dimm[d].mranks == 2)
			ranks = DIMM1_ALL_RANKS;
		else
			ranks = DIMM1_RANK0;
	}

	/*
	 * JEDEC: "All banks must be precharged and tRP met before ZQCL or ZQCS
	 * commands are issued by the controller" - not sure if this is ensured.
	 * A refresh during the calibration probably would impact the results. Also,
	 * "No other activities should be performed on the DRAM channel by the
	 * controller for the duration of tZQinit, tZQoper, or tZQCS" - this means
	 * we have to insert a delay after every ZQCL, not only after the last one.
	 * As a possible improvement, perhaps we could reorder this step a bit and
	 * send ZQCL on all ports "simultaneously" (without delays) and add a delay
	 * just between different DIMMs/ranks, but those delays cannot be done by
	 * CCS and we don't have a timer with enough precision to make it worth the
	 * effort.
	 */
	ccs_add_mrs(id, cmd, ranks, mirrored, tZQinit);
	ccs_execute(id, mca_i);
}

static void clear_initial_cal_errors(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* Whole lot of zeroing
		IOM0.DDRPHY_DP16_RD_VREF_CAL_ERROR_P0_{0-4},
		IOM0.DDRPHY_DP16_WR_ERROR0_P0_{0-4},
		IOM0.DDRPHY_DP16_RD_STATUS0_P0_{0-4},
		IOM0.DDRPHY_DP16_RD_LVL_STATUS2_P0_{0-4},
		IOM0.DDRPHY_DP16_RD_LVL_STATUS0_P0_{0-4},
		IOM0.DDRPHY_DP16_WR_VREF_ERROR0_P0_{0-4},
		IOM0.DDRPHY_DP16_WR_VREF_ERROR1_P0_{0-4},
			[all] 0
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_CAL_ERROR_P0_0, 0, 0);
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_ERROR0_P0_0, 0, 0);
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_STATUS0_P0_0, 0, 0);
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_LVL_STATUS2_P0_0, 0, 0);
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_LVL_STATUS0_P0_0, 0, 0);
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR0_P0_0, 0, 0);
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR1_P0_0, 0, 0);
	}

	/* IOM0.DDRPHY_APB_CONFIG0_P0 =
		[49]  RESET_ERR_RPT = 1, then 0
	*/
	mca_and_or(id, mca_i, DDRPHY_APB_CONFIG0_P0, ~0, PPC_BIT(RESET_ERR_RPT));
	mca_and_or(id, mca_i, DDRPHY_APB_CONFIG0_P0, ~PPC_BIT(RESET_ERR_RPT), 0);

	/* IOM0.DDRPHY_APB_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, DDRPHY_APB_ERROR_STATUS0_P0, 0, 0);

	/* IOM0.DDRPHY_RC_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, DDRPHY_RC_ERROR_STATUS0_P0, 0, 0);

	/* IOM0.DDRPHY_SEQ_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, DDRPHY_SEQ_ERROR_STATUS0_P0, 0, 0);

	/* IOM0.DDRPHY_WC_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, DDRPHY_WC_ERROR_STATUS0_P0, 0, 0);

	/* IOM0.DDRPHY_PC_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_ERROR_STATUS0_P0, 0, 0);

	/* IOM0.DDRPHY_PC_INIT_CAL_ERROR_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_INIT_CAL_ERROR_P0, 0, 0);

	/* IOM0.IOM_PHY0_DDRPHY_FIR_REG =
		[56]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2 = 0
	*/
	mca_and_or(id, mca_i, IOM_PHY0_DDRPHY_FIR_REG,
	           ~PPC_BIT(IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2), 0);
}

static void dump_cal_errors(int mcs_i, int mca_i)
{
#if CONFIG(DEBUG_RAM_SETUP)
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	/*
	 * Values are printed before names for two reasons:
	 * - it is easier to align,
	 * - BMC buffers host's serial output both in 'obmc-console-client' and in
	 *   Serial over LAN and may not print few last characters.
	 */
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

	/* 0x8000 on success for first rank, 0x4000 for second */
	printk(BIOS_ERR, "%#16.16llx - PC_INIT_CAL_STATUS\n",
	       mca_read(id, mca_i, DDRPHY_PC_INIT_CAL_STATUS_P0));

	printk(BIOS_ERR, "%#16.16llx - IOM_PHY0_DDRPHY_FIR_REG\n",
	       mca_read(id, mca_i, IOM_PHY0_DDRPHY_FIR_REG));

	printk(BIOS_ERR, "%#16.16llx - MBACALFIRQ\n",
	       mca_read(id, mca_i, MBACALFIR));
#endif
}

/* Based on ATTR_MSS_MRW_RESET_DELAY_BEFORE_CAL, by default do it. */
static void dp16_reset_delay_values(int mcs_i, int mca_i, enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	/*
	 * It iterates over enabled rank pairs. See 13.8 for where these "pairs"
	 * (which may have up to 4 elements) were set.
	 */
	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP0_P0_{0-4} = 0 */
		if (ranks_present & DIMM0_RANK0)
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DQS_GATE_DELAY_RP0_P0_0, 0, 0);
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP1_P0_{0-4} = 0 */
		if (ranks_present & DIMM0_RANK1)
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DQS_GATE_DELAY_RP1_P0_0, 0, 0);
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP2_P0_{0-4} = 0 */
		if (ranks_present & DIMM1_RANK0)
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DQS_GATE_DELAY_RP2_P0_0, 0, 0);
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP3_P0_{0-4} = 0 */
		if (ranks_present & DIMM1_RANK1)
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DQS_GATE_DELAY_RP3_P0_0, 0, 0);
	}
}

static void dqs_align_turn_on_refresh(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];

	/* IOM0.DDRPHY_SEQ_MEM_TIMING_PARAM0_P0
	// > May need to add freq/tRFI attr dependency later but for now use this value
	// > Provided by Ryan King
	    [60-63] TRFC_CYCLES = 9             // tRFC = 2^9 = 512 memcycles
	*/
	/* See note in seq_reset() in 13.8. This may not be necessary. */
	mca_and_or(id, mca_i, DDRPHY_SEQ_MEM_TIMING_PARAM0_P0, ~PPC_BITMASK(60, 63),
	           PPC_SHIFT(9, TRFC_CYCLES));

	/* IOM0.DDRPHY_PC_INIT_CAL_CONFIG1_P0
	    // > Hard coded settings provided by Ryan King for this workaround
	    [48-51] REFRESH_COUNT =     0xf
	    // TODO: see "Read clock align - pre-workaround" below. Why not 1 until
	    // calibration finishes? Does it pull in refresh commands?
	    [52-53] REFRESH_CONTROL =   3       // refresh commands may interrupt calibration routines
	    [54]    REFRESH_ALL_RANKS = 1
	    [55]    CMD_SNOOP_DIS =     0
	    [57-63] REFRESH_INTERVAL =  0x13    // Worst case: 6.08us for 1866 (max tCK). Must be not more than 7.8us
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_INIT_CAL_CONFIG1_P0,
	           ~(PPC_BITMASK(48, 55) | PPC_BITMASK(57, 63)),
	           PPC_SHIFT(0xF, REFRESH_COUNT) | PPC_SHIFT(3, REFRESH_CONTROL) |
	           PPC_BIT(REFRESH_ALL_RANKS) | PPC_SHIFT(0x13, REFRESH_INTERVAL));
}

static void wr_level_pre(int mcs_i, int mca_i, int rp,
                         enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int d = rp / 2;
	int vpd_idx = (mca->dimm[d].mranks - 1) * 2 + (!!mca->dimm[d ^ 1].present);
	int mirrored = mca->dimm[d].spd[136] & 1;
	mrs_cmd_t mrs;
	enum rank_selection rank = 1 << rp;
	int i;

	/*
	 * JEDEC specification requires disabling RTT_WR during WR_LEVEL, and
	 * enabling equivalent terminations.
	 */
	if (ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx] != 0) {
		/* MR2 =               // redo the rest of the bits
		  [A11-A9]  0
		*/
		mrs = ddr4_get_mr2(DDR4_MR2_WR_CRC_DISABLE,
                           vpd_to_rtt_wr(0),
                           DDR4_MR2_ASR_MANUAL_EXTENDED_RANGE,
                           mem_data.cwl);
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);

		/* MR1 =               // redo the rest of the bits
		  // Write properly encoded RTT_WR value as RTT_NOM
		  [A8-A10]  240/ATTR_MSS_VPD_MT_DRAM_RTT_WR
		*/
		mrs = ddr4_get_mr1(DDR4_MR1_QOFF_ENABLE,
                           mca->dimm[d].width == WIDTH_x8 ? DDR4_MR1_TQDS_ENABLE :
                                                            DDR4_MR1_TQDS_DISABLE,
                           vpd_to_rtt_nom(ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx]),
                           DDR4_MR1_WRLVL_DISABLE,
                           DDR4_MR1_ODIMP_RZQ_7,
                           DDR4_MR1_AL_DISABLE,
                           DDR4_MR1_DLL_ENABLE);
		/*
		 * Next command for this rank is REF, done by PHY hardware, so use tMOD.
		 *
		 * There are possible MRS commands to be send to other ranks, maybe we
		 * can subtract those. On the other hand, with microsecond precision for
		 * delays in ccs_execute(), this probably doesn't matter anyway.
		 */
		ccs_add_mrs(id, mrs, rank, mirrored, tMOD);

		/*
		 * This block is done after MRS commands in Hostboot, but we do not call
		 * ccs_execute() until the end of this function anyway. It doesn't seem
		 * to make a difference.
		 */
		switch (rp) {
		case 0:
			/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG0_P0 =
			  [48] = 1
			*/
			mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_WR_CONFIG0_P0, ~0, PPC_BIT(48));
			break;
		case 1:
			/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG0_P0 =
			  [57] = 1
			*/
			mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_WR_CONFIG0_P0, ~0, PPC_BIT(57));
			break;
		case 2:
			/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG1_P0 =
			  [50] = 1
			*/
			mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_WR_CONFIG0_P0, ~0, PPC_BIT(50));
			break;
		case 3:
			/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG1_P0 =
			  [59] = 1
			*/
			mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_WR_CONFIG0_P0, ~0, PPC_BIT(59));
			break;
		}

		// mss::workarounds::seq::odt_config();		// Not needed on DD2

	}

	/* Different workaround, executed even if RTT_WR == 0 */
	/* workarounds::wr_lvl::configure_non_calibrating_ranks()
	for each rank on MCA except current primary rank:
	  MR1 =               // redo the rest of the bits
		  [A7]  = 1       // Write Leveling Enable
		  [A12] = 1       // Outputs disabled (DQ, DQS)
	*/
	for (i = 0; i < 4; i++) {
		rank = 1 << i;
		if (i == rp || !(ranks_present & rank))
			continue;

		/*
		 * VPD index stays the same (DIMM mixing rules), but I'm not sure about
		 * mirroring. Better safe than sorry, assume mirrored and non-mirrored
		 * DIMMs can be mixed.
		 */
		mirrored = mca->dimm[i/2].spd[136] & 1;

		mrs = ddr4_get_mr1(DDR4_MR1_QOFF_DISABLE,
                           mca->dimm[d].width == WIDTH_x8 ? DDR4_MR1_TQDS_ENABLE :
                                                            DDR4_MR1_TQDS_DISABLE,
                           vpd_to_rtt_nom(ATTR_MSS_VPD_MT_DRAM_RTT_NOM[vpd_idx]),
                           DDR4_MR1_WRLVL_ENABLE,
                           DDR4_MR1_ODIMP_RZQ_7,
                           DDR4_MR1_AL_DISABLE,
                           DDR4_MR1_DLL_ENABLE);
		/*
		 * Delays apply to commands sent to the same rank, but we are changing
		 * ranks. Can we get away with 0 delay? Is it worth it? Remember that
		 * the same delay is currently used between sides of RCD.
		 */
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);
	}

	/* TODO: maybe drop it, next ccs_phy_hw_step() would call it anyway. */
	//ccs_execute(id, mca_i);
}

static uint64_t wr_level_time(mca_data_t *mca)
{
	/*
	 * "Note: the following equation is taken from the PHY workbook - leaving
	 * the naked numbers in for parity to the workbook
	 *
	 * This step runs for approximately (80 + TWLO_TWLOE) x NUM_VALID_SAMPLES x
	 * (384/(BIG_STEP + 1) + (2 x (BIG_STEP + 1))/(SMALL_STEP + 1)) + 20 memory
	 * clock cycles per rank."
	 *
	 * TWLO_TWLOE for every defined speed bin is 9.5 + 2 = 11.5 ns, this needs
	 * to be converted to clock cycles, it is the only non-constant component of
	 * the equation.
	 */
	const int big_step = 7;
	const int small_step = 0;
	const int num_valid_samples = 5;
	const int twlo_twloe = ps_to_nck(11500);

	return (80 + twlo_twloe) * num_valid_samples * (384 / (big_step + 1) +
	       (2 * (big_step + 1)) / (small_step + 1)) + 20;
}

/* Undo the pre-workaround, basically */
static void wr_level_post(int mcs_i, int mca_i, int rp,
                          enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int d = rp / 2;
	int vpd_idx = (mca->dimm[d].mranks - 1) * 2 + (!!mca->dimm[d ^ 1].present);
	int mirrored = mca->dimm[d].spd[136] & 1;
	mrs_cmd_t mrs;
	enum rank_selection rank = 1 << rp;
	int i;

	/*
	 * JEDEC specification requires disabling RTT_WR during WR_LEVEL, and
	 * enabling equivalent terminations.
	 */
	if (ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx] != 0) {
		#define F(x) ((((x) >> 4) & 0xc) | (((x) >> 2) & 0x3))
		/* Originally done in seq_reset() in 13.8 */
		/* IOM0.DDRPHY_SEQ_ODT_RD_CONFIG1_P0 =
			  F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
			  [all]   0
			  [48-51] ODT_RD_VALUES0 =
				count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][1][0])
				count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][2])
			  [56-59] ODT_RD_VALUES1 =
				count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][1][1])
				count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][3])
		*/
		/* 2 DIMMs -> odd vpd_idx */
		uint64_t val = 0;
		if (vpd_idx % 2)
			val = PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][0]), ODT_RD_VALUES0) |
				  PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][1]), ODT_RD_VALUES1);

		mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_RD_CONFIG1_P0, 0, val);


		/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG0_P0 =
			  F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
			  [all]   0
			  [48-51] ODT_WR_VALUES0 = F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][0])
			  [56-59] ODT_WR_VALUES1 = F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][1])
		*/
		mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_WR_CONFIG0_P0, 0,
				   PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][0]), ODT_RD_VALUES0) |
				   PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][1]), ODT_RD_VALUES1));
		#undef F

		/* MR2 =               // redo the rest of the bits
		  [A11-A9]  ATTR_MSS_VPD_MT_DRAM_RTT_WR
		*/
		mrs = ddr4_get_mr2(DDR4_MR2_WR_CRC_DISABLE,
                           vpd_to_rtt_wr(ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx]),
                           DDR4_MR2_ASR_MANUAL_EXTENDED_RANGE,
                           mem_data.cwl);
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);

		/* MR1 =               // redo the rest of the bits
		  // Write properly encoded RTT_NOM value
		  [A8-A10]  240/ATTR_MSS_VPD_MT_DRAM_RTT_NOM
		*/
		mrs = ddr4_get_mr1(DDR4_MR1_QOFF_ENABLE,
                           mca->dimm[d].width == WIDTH_x8 ? DDR4_MR1_TQDS_ENABLE :
                                                            DDR4_MR1_TQDS_DISABLE,
                           vpd_to_rtt_nom(ATTR_MSS_VPD_MT_DRAM_RTT_NOM[vpd_idx]),
                           DDR4_MR1_WRLVL_DISABLE,
                           DDR4_MR1_ODIMP_RZQ_7,
                           DDR4_MR1_AL_DISABLE,
                           DDR4_MR1_DLL_ENABLE);
		/*
		 * Next command for this rank should be REF before Initial Pattern Write,
		 * done by PHY hardware, so use tMOD.
		 */
		ccs_add_mrs(id, mrs, rank, mirrored, tMOD);

		// mss::workarounds::seq::odt_config();		// Not needed on DD2
	}

	/* Different workaround, executed even if RTT_WR == 0 */
	/* workarounds::wr_lvl::configure_non_calibrating_ranks()
	for each rank on MCA except current primary rank:
	  MR1 =               // redo the rest of the bits
		  [A7]  = 1       // Write Leveling Enable
		  [A12] = 1       // Outputs disabled (DQ, DQS)
	*/
	for (i = 0; i < 4; i++) {
		rank = 1 << i;
		if (i == rp || !(ranks_present & rank))
			continue;

		/*
		 * VPD index stays the same (DIMM mixing rules), but I'm not sure about
		 * mirroring. Better safe than sorry, assume mirrored and non-mirrored
		 * DIMMs can be mixed.
		 */
		mirrored = mca->dimm[i/2].spd[136] & 1;

		mrs = ddr4_get_mr1(DDR4_MR1_QOFF_ENABLE,
                           mca->dimm[d].width == WIDTH_x8 ? DDR4_MR1_TQDS_ENABLE :
                                                            DDR4_MR1_TQDS_DISABLE,
                           vpd_to_rtt_nom(ATTR_MSS_VPD_MT_DRAM_RTT_NOM[vpd_idx]),
                           DDR4_MR1_WRLVL_DISABLE,
                           DDR4_MR1_ODIMP_RZQ_7,
                           DDR4_MR1_AL_DISABLE,
                           DDR4_MR1_DLL_ENABLE);
		/*
		 * Delays apply to commands sent to the same rank, but we are changing
		 * ranks. Can we get away with 0 delay? Is it worth it? Remember that
		 * the same delay is currently used between sides of RCD.
		 */
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);
	}

	/* TODO: maybe drop it, next ccs_phy_hw_step() would call it anyway. */
	//ccs_execute(id, mca_i);
}

static uint64_t initial_pat_wr_time(mca_data_t *mca)
{
	/*
	 * "Not sure how long this should take, so we're gonna use 1 to make sure we
	 * get at least one polling loop"
	 *
	 * Hostboot polls every 10 us, but in coreboot this value results in minimal
	 * delay of 2 us (one microsecond for delay_nck() and another for wait_us()
	 * in ccs_execute()). Tests show that it is not enough.
	 *
	 * What has to be done to write pattern to MPR in general:
	 * - write to MR3 to enable MPR access (tMOD)
	 * - write to MPRs (tWR_MPR for back-to-back writes, there are 4 MPRs;
	 *   tWR_MPR is tMOD + AL + PL, but AL and PL is 0 here)
	 * - write to MR3 to disable MPR access (tMOD or tMRD, depending on what is
	 *   the next command).
	 *
	 * This gives 6 * tMOD, but because there is RCD with sides A and B this is
	 * 12 * tMOD = 288 nCK. However, we have to add to calculations refresh
	 * commands, as set in dqs_align_turn_on_refresh() - 15 commands, each takes
	 * 512 nCK. This is kind of consistent for 2666 MT/s DIMM with 5 us I've
	 * seen in tests.
	 *
	 * There is no limit about how many refresh commands can be issued (as long
	 * as tRFC isn't violated), but only 8 of them are "pulling in" further
	 * refreshes, meaning that DRAM will survive 9*tREFI without a refresh
	 * (8 pulled in and 1 regular interval) - this is useful for longer
	 * calibration steps. Another 9*tREFI can be postponed - REF commands are
	 * sent after a longer pause, but this (probably) isn't relevant here.
	 *
	 * There may be more refreshes sent in the middle of the most of steps due
	 * to REFRESH_CONTROL setting.
	 *
	 * These additional cycles should be added to all calibration steps. I don't
	 * think they are included in Hostboot, then again I don't know what exactly
	 * is added in "equations taken from the PHY workbook". This may be the
	 * reason why Hostboot multiplies every timeout by 4 AND assumes worst case
	 * wherever possible AND polls so rarely.
	 *
	 * From the lack of better ideas, return 10 us.
	 */
	return ns_to_nck(10 * 1000);
}

static uint64_t dqs_align_time(mca_data_t *mca)
{
	/*
	 * "This step runs for approximately 6 x 600 x 4 DRAM clocks per rank pair."
	 *
	 * In tests this is a bit less than that, but not enough to impact total
	 * times because we start busy polling earlier.
	 */
	return 6 * 600 * 4;
}

static void rdclk_align_pre(int mcs_i, int mca_i, int rp,
                            enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];

	/*
	 * TODO: we just set it before starting calibration steps. As we don't have
	 * any precious data in RAM yet, maybe we can use 0 there and just change it
	 * to 3 in the post-workaround?
	 */

	/* Turn off refresh, we don't want it to interfere here
	IOM0.DDRPHY_PC_INIT_CAL_CONFIG1_P0
		[52-53] REFRESH_CONTROL =   0       // refresh commands are only sent at start of initial calibration
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_INIT_CAL_CONFIG1_P0, ~PPC_BITMASK(52, 53), 0);
}

static uint64_t rdclk_align_time(mca_data_t *mca)
{
	/*
	 * "This step runs for approximately 24 x ((1024/COARSE_CAL_STEP_SIZE +
	 * 4 x COARSE_CAL_STEP_SIZE) x 4 + 32) DRAM clocks per rank pair"
	 *
	 * COARSE_CAL_STEP_SIZE = 4
	 *
	 * In tests this finishes in about a third of this time (7 us instead of
	 * calculated 20.16 us).
	 */
	const int coarse_cal_step_size = 4;
	return 24 * ((1024/coarse_cal_step_size + 4*coarse_cal_step_size) * 4 + 32);
}

static void rdclk_align_post(int mcs_i, int mca_i, int rp,
                             enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;
	uint64_t val;
	const uint64_t mul = 0x0000010000000000;

	/*
	 * "In DD2.*, We adjust the red waterfall to account for low VDN settings.
	 * We move the waterfall forward by one"
	IOM0.DDRPHY_DP16_DQS_RD_PHASE_SELECT_RANK_PAIR{0-3}_P0_{0-3}
	    [48-49] DQSCLK_SELECT0 = (++DQSCLK_SELECT0 % 4)
	    [52-53] DQSCLK_SELECT1 = (++DQSCLK_SELECT1 % 4)
	    [56-57] DQSCLK_SELECT2 = (++DQSCLK_SELECT2 % 4)
	    [60-61] DQSCLK_SELECT3 = (++DQSCLK_SELECT3 % 4)
	IOM0.DDRPHY_DP16_DQS_RD_PHASE_SELECT_RANK_PAIR{0-3}_P0_4
	    [48-49] DQSCLK_SELECT0 = (++DQSCLK_SELECT0 % 4)
	    [52-53] DQSCLK_SELECT1 = (++DQSCLK_SELECT1 % 4)
	    // Can't change non-existing quads
	*/
	for (dp = 0; dp < 4; dp++) {
		val = dp_mca_read(id, dp, mca_i,
		                  DDRPHY_DP16_DQS_RD_PHASE_SELECT_RANK_PAIR0_P0_0 + rp * mul);
		val += PPC_BIT(49) | PPC_BIT(53) | PPC_BIT(57) | PPC_BIT(61);
		val &= PPC_BITMASK(48, 49) | PPC_BITMASK(52, 53) | PPC_BITMASK(56, 57) |
		       PPC_BITMASK(60, 61);
		/* TODO: this can be done with just one read */
		dp_mca_and_or(id, dp, mca_i,
		              DDRPHY_DP16_DQS_RD_PHASE_SELECT_RANK_PAIR0_P0_0 + rp * mul,
		              ~(PPC_BITMASK(48, 49) | PPC_BITMASK(52, 53) |
		                PPC_BITMASK(56, 57) | PPC_BITMASK(60, 61)),
		              val);
	}

	val = dp_mca_read(id, 4, mca_i,
	                  DDRPHY_DP16_DQS_RD_PHASE_SELECT_RANK_PAIR0_P0_0 + rp * mul);
	val += PPC_BIT(49) | PPC_BIT(53);
	val &= PPC_BITMASK(48, 49) | PPC_BITMASK(52, 53);
	dp_mca_and_or(id, dp, mca_i,
	              DDRPHY_DP16_DQS_RD_PHASE_SELECT_RANK_PAIR0_P0_0 + rp * mul,
				  ~(PPC_BITMASK(48, 49) | PPC_BITMASK(52, 53)),
				  val);

	/* Turn on refresh */
	dqs_align_turn_on_refresh(mcs_i, mca_i);
}

static void read_ctr_pre(int mcs_i, int mca_i, int rp,
                         enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	/* Turn off refresh
	IOM0.DDRPHY_PC_INIT_CAL_CONFIG1_P0
		[52-53] REFRESH_CONTROL =   0       // refresh commands are only sent at start of initial calibration
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_INIT_CAL_CONFIG1_P0, ~PPC_BITMASK(52, 53), 0);

	for (dp = 0; dp < 5; dp++) {
		/*
		IOM0.DDRPHY_DP16_CONFIG0_P0_{0-4}
			[62]  1         // part of ATESTSEL_0_4 field
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_CONFIG0_P0_0, ~0, PPC_BIT(62));

		/*
		 * This was a part of main calibration in Hostboot, not pre-workaround,
		 * but this is easier this way.
		IOM0.DDRPHY_DP16_RD_VREF_CAL_EN_P0_{0-4}
			[all] 0
			[48-63] VREF_CAL_EN = 0xffff    // We already did this in reset_rd_vref() in 13.8
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_CAL_EN_P0_0, 0,
		              PPC_SHIFT(0xFFFF, 63));
	}

	/* This also was part of main
	IOM0.DDRPHY_RC_RDVREF_CONFIG1_P0
		[60]  CALIBRATION_ENABLE =  1
		[61]  SKIP_RDCENTERING =    0
	*/
	mca_and_or(id, mca_i, DDRPHY_RC_RDVREF_CONFIG1_P0,
	           ~PPC_BIT(SKIP_RDCENTERING),
	           PPC_BIT(CALIBRATION_ENABLE));
}

static uint64_t read_ctr_time(mca_data_t *mca)
{
	/*
	 * "This step runs for approximately 6 x (512/COARSE_CAL_STEP_SIZE + 4 x
	 *  (COARSE_CAL_STEP_SIZE + 4 x CONSEQ_PASS)) x 24 DRAM clocks per rank pair."
	 *
	 * COARSE_CAL_STEP_SIZE = 4
	 * CONSEQ_PASS = 8
	 *
	 * In tests this step takes more than that (38/30us), probably because of
	 * REF commands that are pulled in before the calibration. It is still much
	 * less than timeout (107us).
	 */
	const int coarse_cal_step_size = 4;
	const int conseq_pass = 8;
	return 6 * (512/coarse_cal_step_size + 4 * (coarse_cal_step_size + 4 * conseq_pass))
	       * 24;
}

static void read_ctr_post(int mcs_i, int mca_i, int rp,
                          enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	/* Does not apply to DD2 */
	// workarounds::dp16::rd_dq::fix_delay_values();

	/* Turn on refresh */
	dqs_align_turn_on_refresh(mcs_i, mca_i);

	for (dp = 0; dp < 5; dp++) {
		/*
		IOM0.DDRPHY_DP16_CONFIG0_P0_{0-4}
			[62]  0         // part of ATESTSEL_0_4 field
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_CONFIG0_P0_0, ~PPC_BIT(62), 0);
	}
}

/* Assume 18 DRAMs per DIMM ((8 data + 1 ECC) * 2), even for x8 */
static uint16_t write_delays[18];

static void write_ctr_pre(int mcs_i, int mca_i, int rp,
                          enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int mirrored = mca->dimm[rp/2].spd[136] & 1;
	mrs_cmd_t mrs;
	enum rank_selection rank = 1 << rp;
	int vpd_idx = (mca->dimm[rp/2].mranks - 1) * 2 + (!!mca->dimm[(rp/2) ^ 1].present);
	int dram;

	/*
	 * Write VREF Latching
	 *
	 * This may be considered a separate step, but with current dispatch logic
	 * we cannot add a step that isn't accelerated by PHY hardware so do this as
	 * a part of pre-workaround of next step.
	 *
	 * "JEDEC has a 3 step latching process for WR VREF
	 *  1) enter into VREFDQ training mode, with the desired range value is XXXXXX
	 *  2) set the VREFDQ value while in training mode - this actually latches the value
	 *  3) exit VREFDQ training mode and go into normal operation mode"
	 *
	 * Each step is followed by a 150ns (tVREFDQE or tVREFDQX) stream of DES
	 * commands before next one.
	 */
	uint64_t tVREFDQ_E_X = ns_to_nck(150);

	/* Fill MRS command once, then flip VREFDQ training mode bit as needed */
	mrs = ddr4_get_mr6(mca->nccd_l,
                       DDR4_MR6_VREFDQ_TRAINING_ENABLE,
                       (ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx] & 0x40) >> 6,
                       ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx] & 0x3F);

	/* Step 1 - enter VREFDQ training mode */
	ccs_add_mrs(id, mrs, rank, mirrored, tVREFDQ_E_X);

	/* Step 2 - latch VREFDQ value, command exactly the same as step 1 */
	ccs_add_mrs(id, mrs, rank, mirrored, tVREFDQ_E_X);

	/* Step 3 - exit VREFDQ training mode */
	mrs ^= 1 << 7;		// A7 - VREFDQ Training Enable
	ccs_add_mrs(id, mrs, rank, mirrored, tVREFDQ_E_X);

	/* TODO: maybe drop it, next ccs_phy_hw_step() would call it anyway. */
	//ccs_execute(id, mca_i);

	/* End of VREF Latching, beginning of Write Centering pre-workaround */

	/*
	 * DRAM is one IC on the DIMM module, there are 9 DRAMs for x8 and 18 for
	 * x4 devices (DQ bits/width) per rank. Before centering the delays are the
	 * same for each DQ of a given DRAM, meaning it is enough to save just one
	 * value per DRAM. For simplicity, save every 4th DQ even on x8 devices.
	 */
	for (dram = 0; dram < ARRAY_SIZE(write_delays); dram++) {
		int dp = (dram * 4) / 16;
		int val_idx = (dram * 4) % 16;
		const uint64_t rp_mul =  0x0000010000000000;
		const uint64_t val_mul = 0x0000000100000000;
		/* IOM0.DDRPHY_DP16_WR_DELAY_VALUE_<val_idx>_RP<rp>_REG_P0_<dp> */
		uint64_t val = dp_mca_read(id, dp, mca_i,
		                           DDRPHY_DP16_WR_DELAY_VALUE_0_RP0_REG_P0_0 +
		                           rp * rp_mul + val_idx * val_mul);
		write_delays[dram] = (uint16_t) val;
	}
}

static uint64_t write_ctr_time(mca_data_t *mca)
{
	/*
	 * "1000 + (NUM_VALID_SAMPLES * (FW_WR_RD + FW_RD_WR + 16) *
	 * (1024/(SMALL_STEP +1) + 128/(BIG_STEP +1)) + 2 * (BIG_STEP+1)/(SMALL_STEP+1)) * 24
	 * DRAM clocks per rank pair."
	 *
	 * Yes, write leveling values are used for write centering, this is not an
	 * error (or is it? CONFIG0 says BIG_STEP = 1)
	 * WR_LVL_BIG_STEP = 7
	 * WR_LVL_SMALL_STEP = 0
	 * WR_LVL_NUM_VALID_SAMPLES = 5
	 *
	 * "Per PHY spec, defaults to 0. Would need an attribute to drive differently"
	 * FW_WR_RD = 0
	 *
	 * "From the PHY spec. Also confirmed with S. Wyatt as this is different
	 * than the calculation used in Centaur. This field must be set to the
	 * larger of the two values in number of memory clock cycles.
	 * FW_RD_WR = max(tWTR + 11, AL + tRTP + 3)
	 * Note from J. Bialas: The difference between tWTR_S and tWTR_L is that _S
	 * is write to read time to different bank groups, while _L is to the same.
	 * The algorithm should be switching bank groups so tWTR_S can be used"
	 *
	 * tRTP = 7.5ns (this comes from DDR4 spec)
	 * AL = 0
	 *
	 * For tWTR_S = 2.5ns this should give ~2.9-4.5ms, + 2 * 3 * 150ns from MRS
	 * commands in pre-workaround (insignificantly small compared to total time).
	 * In tests this is ~7.5ms, with 10.5ms timeout, mostly because the equation
	 * below probably doesn't account for REF commands. This leaves rather small
	 * margin for error.
	 */
	const int big_step = 7;
	const int small_step = 0;
	const int num_valid_samples = 5;
	int fw_rd_wr = MAX(mca->nwtr_s + 11, ps_to_nck(7500) + 3);
	return 1000 + (num_valid_samples * (fw_rd_wr + 16) *
	               (1024/(small_step + 1) + 128/(big_step + 1)) +
	               2 * (big_step + 1)/(small_step + 1)) * 24;
}

static void write_ctr_post(int mcs_i, int mca_i, int rp,
                           enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;
	uint64_t bad_bits = 0;

	/*
	 * TODO: this just tests if workaround is needed, real workaround is not
	 * yet implemented.
	 */
	for (dp = 0; dp < 5; dp++) {
		bad_bits |= dp_mca_read(id, dp, mca_i, DDRPHY_DP16_DQ_BIT_DISABLE_RP0_P0_0);
	}

	if (!bad_bits)
		return;

	/*
	 * Full workaround consists of:
	 * - enabling PDA mode (per DRAM addressing) on MC
	 * - reverting initial WR Vref values in MC
	 * - reverting WR delays saved in pre-workaround
	 * - clearing bad DQ bits (because this calibration step will be re-run)
	 * - entering PDA mode on DRAMs
	 * - reverting initial VREFDQ values in bad DRAM(s)
	 * - exiting PDA mode on DRAMs (this point has its own workaround)
	 * - exiting PDA mode on MC
	 * - finding a median of RD Vref DAC values and disabling all DQ bits except
	 *   one known to be good (close to median)
	 * - rerunning main calibration, exit on success
	 * - if it still fails, re-enable all DQ bits (bad and good), set 1D only
	 *   write centering and rerun again
	 */
	die("Write Centering post-workaround required, but not yet implemented\n");
}

static uint64_t coarse_wr_rd_time(mca_data_t *mca)
{
	/*
	 * "40 cycles for WR, 32 for RD"
	 *
	 * With number of cycles set to just the above this step times out, add time
	 * for 15 REF commands as set in dqs_align_turn_on_refresh().
	 */
	return 40 + 32 + 15 * 512;
}

typedef void (phy_workaround_t) (int mcs_i, int mca_i, int rp,
                                 enum rank_selection ranks_present);

struct phy_step {
	const char *name;
	enum cal_config cfg;
	phy_workaround_t *pre;
	uint64_t (*time)(mca_data_t *mca);
	phy_workaround_t *post;
};

static struct phy_step steps[] = {
	{
		"Write Leveling",
		CAL_WR_LEVEL,
		wr_level_pre,
		wr_level_time,
		wr_level_post,
	},
	{
		"Initial Pattern Write",
		CAL_INITIAL_PAT_WR,
		NULL,
		initial_pat_wr_time,
		NULL,
	},
	{
		"DQS alignment",
		CAL_DQS_ALIGN,
		NULL,
		dqs_align_time,
		NULL,
	},
	{
		"Read Clock Alignment",
		CAL_RDCLK_ALIGN,
		rdclk_align_pre,
		rdclk_align_time,
		rdclk_align_post,
	},
	{
		"Read Centering",
		CAL_READ_CTR,
		read_ctr_pre,
		read_ctr_time,
		read_ctr_post,
	},
	{
		"Write Centering",
		CAL_WRITE_CTR,
		write_ctr_pre,
		write_ctr_time,
		write_ctr_post,
	},
	{
		"Coarse write/read",
		CAL_INITIAL_COARSE_WR | CAL_COARSE_RD,
		NULL,
		coarse_wr_rd_time,
		NULL,
	},

/*
	// Following are performed in istep 13.12
	CAL_CUSTOM_RD
	CAL_CUSTOM_WR
*/
};

static void dispatch_step(struct phy_step *step, int mcs_i, int mca_i, int rp,
                          enum rank_selection ranks_present)
{
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	printk(BIOS_DEBUG, "%s starting\n", step->name);

	if (step->pre)
		step->pre(mcs_i, mca_i, rp, ranks_present);

	ccs_phy_hw_step(mcs_ids[mcs_i], mca_i, rp, step->cfg, step->time(mca));

	if (step->post)
		step->post(mcs_i, mca_i, rp, ranks_present);

	dump_cal_errors(mcs_i, mca_i);

	if (mca_read(mcs_ids[mcs_i], mca_i, DDRPHY_PC_INIT_CAL_ERROR_P0) != 0)
		die("%s failed, aborting\n", step->name);

	printk(BIOS_DEBUG, "%s done\n", step->name);
}

/* Can we modify dump_cal_errors() for this? */
static int process_initial_cal_errors(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;
	uint64_t err = 0;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_RD_VREF_CAL_ERROR_P0_n */
		err |= dp_mca_read(id, dp, mca_i, DDRPHY_DP16_RD_VREF_CAL_ERROR_P0_0);

		/* Both ERROR_MASK registers were set to 0xFFFF in 13.8 */
		/* IOM0.DDRPHY_DP16_WR_VREF_ERROR0_P0_n &
		 * ~IOM0.DDRPHY_DP16_WR_VREF_ERROR_MASK0_P0_n */
		err |= (dp_mca_read(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR0_P0_0) &
		        ~dp_mca_read(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR_MASK0_P0_0));

		/* IOM0.DDRPHY_DP16_WR_VREF_ERROR1_P0_n &
		 * ~IOM0.DDRPHY_DP16_WR_VREF_ERROR_MASK1_P0_n */
		err |= (dp_mca_read(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR1_P0_0) &
		        ~dp_mca_read(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR_MASK1_P0_0));
	}

	/* IOM0.DDRPHY_PC_INIT_CAL_ERROR_P0 */
	err |= mca_read(id, mca_i, DDRPHY_PC_INIT_CAL_ERROR_P0);

	if (err)
		return 1;

	/*
	 * err == 0 at this point can be either a true success or an error of the
	 * calibration engine itself. Check for latter.
	 */
	/* IOM0.IOM_PHY0_DDRPHY_FIR_REG */
	if (read_scom_for_chiplet(id, IOM_PHY0_DDRPHY_FIR_REG) &
	    PPC_BIT(IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2)) {
		/*
		 * "Clear the PHY FIR ERROR 2 bit so we don't keep failing training and
		 * training advance on this port"
		 */
		scom_and_or_for_chiplet(id, IOM_PHY0_DDRPHY_FIR_REG,
		                        ~PPC_BIT(IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2),
		                        0);

		return 1;
	}

	return 0;
}

static int can_recover(int mcs_i, int mca_i, int rp)
{
	/*
	 * We can recover from 1 nibble + 1 bit (or less) bad lines. Anything more
	 * and DIMM is beyond repair. A bad nibble is a nibble with any number of
	 * bad bits. If a DQS is bad (either true or complementary signal, or both),
	 * a whole nibble (for x4 DRAMs) or byte (x8) is considered bad.
	 *
	 * Check both DQS and DQ registers in one loop, iterating over DP16s - that
	 * way it is easier to sum bad bits/nibbles.
	 *
	 * See reset_clock_enable() in 13.8 or an array in process_bad_bits() in
	 * phy/dp16.C for mapping of DQS bits in x8 and mask bits from this register
	 * accordingly.
	 */
	int bad_nibbles = 0;
	int bad_bits = 0;
	int dp;
	chiplet_id_t id = mcs_ids[mcs_i];
	uint8_t width = mem_data.mcs[mcs_i].mca[mca_i].dimm[rp/2].width;

	for (dp = 0; dp < 5; dp++) {
		uint64_t reg;
		uint64_t nibbles_mask = 0xFFFF;
		/*
		IOM0.DDRPHY_DP16_DQS_BIT_DISABLE_RP<rp>_P0_{0-4}:
		  // This calculates how many (DQS_t | DQS_c) failed - if _t and _c failed
		  // for the same DQS, we count it as one.
		  bad_dqs = bit_count((reg & 0x5500) | ((reg & 0xaa00) >> 1))
		  if x8 && bad_dqs > 0: DIMM is FUBAR, return error
		  total_bad_nibbles += bad_dqs
		  // If we are already past max possible number, we might as well return now
		  if total_bad_nibbles > 1: DIMM is FUBAR, return error
		*/
		const uint64_t rp_mul = 0x0000010000000000;
		reg = dp_mca_read(id, dp, mca_i,
		                  DDRPHY_DP16_DQS_BIT_DISABLE_RP0_P0_0 + rp * rp_mul);

		/* One bad DQS on x8 is already bad 2 nibbles, can't recover from that. */
		if (reg != 0 && width == WIDTH_x8)
			return 0;

		if (reg & (PPC_BIT(48) | PPC_BIT(49)))		// quad 0
			nibbles_mask &= 0x0FFF;
		if (reg & (PPC_BIT(50) | PPC_BIT(51)))		// quad 1
			nibbles_mask &= 0xF0FF;
		if (reg & (PPC_BIT(52) | PPC_BIT(53)))		// quad 2
			nibbles_mask &= 0xFF0F;
		if (reg & (PPC_BIT(54) | PPC_BIT(55)))		// quad 3
			nibbles_mask &= 0xFFF0;

		bad_nibbles += __builtin_popcount((reg & 0x5500) | ((reg & 0xAA00) >> 1));

		/*
		IOM0.DDRPHY_DP16_DQ_BIT_DISABLE_RP<rp>_P0_{0-4}:
		  nibble = {[48-51], [52-55], [56-59], [60-63]}
		  for each nibble:
			if bit_count(nibble) >  1: total_bad_nibbles += 1
			if bit_count(nibble) == 1: total_bad_bits += 1
			// We can't have two bad bits, one of them must be treated as bad nibble
			if total_bad_bits    >  1: total_bad_nibbles += 1, total_bad_bits -= 1
			if total_bad_nibbles >  1: DIMM is FUBAR, return error?
		*/
		reg = dp_mca_read(id, dp, mca_i,
		                  DDRPHY_DP16_DQ_BIT_DISABLE_RP0_P0_0 + rp * rp_mul);

		/* Exclude nibbles corresponding to a bad DQS, it won't get worse. */
		reg &= nibbles_mask;

		/* Add bits in nibbles */
		reg = ((reg & 0x1111) >> 0) + ((reg & 0x2222) >> 1) +
		      ((reg & 0x4444) >> 2) + ((reg & 0x8888) >> 3);

		/*
		 * We only care if there is 0, 1 or more bad bits. Collapse bits [0-2]
		 * of each nibble into [2], leave [3] unmodified (PPC bit numbering).
		 */
		reg = ((reg & 0x1111) >> 0) | ((reg & 0x2222) >> 0) |
		      ((reg & 0x4444) >> 1) | ((reg & 0x8888) >> 2);

		/* Clear bit [3] if [2] is also set. */
		reg = (reg & 0x2222) | ((reg & 0x1111) & ~((reg & 0x2222) >> 1));

		/* Now [2] is bad nibble, [3] is exactly one bad bit */
		bad_bits += __builtin_popcount(reg & 0x1111);
		if (bad_bits > 1) {
			bad_nibbles += bad_bits - 1;
			bad_bits = 1;
		}
		bad_nibbles += __builtin_popcount(reg & 0x2222);

		/* No need to test for bad single bits, condition above handles it */
		if (bad_nibbles > 1)
			return 0;
	}

	/*
	 * Now, if total_bad_nibbles is less than 2 we know that total_bad_bits is
	 * also less than 2, and DIMM is good enough for recovery.
	 */
	printk(BIOS_WARNING, "MCS%d MCA%d DIMM%d has %d bad nibble(s) and %d bad "
	       "bit(s), but can be recovered\n", mcs_i, mca_i, rp/2, bad_nibbles,
	       bad_bits);
	return 1;
}

static void fir_unmask(int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int mca_i;

	/*
	 * "All mcbist attentions are already special attentions"
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRACT0
		  [1] MCBISTFIRQ_COMMAND_ADDRESS_TIMEOUT =  0
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRACT1
		  [1] MCBISTFIRQ_COMMAND_ADDRESS_TIMEOUT =  1
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRMASK
		  [1] MCBISTFIRQ_COMMAND_ADDRESS_TIMEOUT =  0     //recoverable_error (0,1,0)
	 */
	scom_and_or_for_chiplet(id, MCBISTFIRACT0,
	                        ~PPC_BIT(MCBISTFIRQ_COMMAND_ADDRESS_TIMEOUT),
	                        0);
	scom_and_or_for_chiplet(id, MCBISTFIRACT1,
	                        ~PPC_BIT(MCBISTFIRQ_COMMAND_ADDRESS_TIMEOUT),
	                        PPC_BIT(MCBISTFIRQ_COMMAND_ADDRESS_TIMEOUT));
	scom_and_or_for_chiplet(id, MCBISTFIRMASK,
	                        ~PPC_BIT(MCBISTFIRQ_COMMAND_ADDRESS_TIMEOUT),
	                        0);

	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		if (!mem_data.mcs[mcs_i].mca[mca_i].functional)
			continue;

		/*
		MC01.PORT0.SRQ.MBACALFIR_ACTION0
			[2]   MBACALFIR_MASK_REFRESH_OVERRUN =        0
			[5]   MBACALFIR_MASK_DDR_CAL_TIMEOUT_ERR =    0
			[7]   MBACALFIR_MASK_DDR_CAL_RESET_TIMEOUT =  0
			[9]   MBACALFIR_MASK_WRQ_RRQ_HANG_ERR =       0
			[11]  MBACALFIR_MASK_ASYNC_IF_ERROR =         0
			[12]  MBACALFIR_MASK_CMD_PARITY_ERROR =       0
			[14]  MBACALFIR_MASK_RCD_CAL_PARITY_ERROR =   0
		MC01.PORT0.SRQ.MBACALFIR_ACTION1
			[2]   MBACALFIR_MASK_REFRESH_OVERRUN =        1
			[5]   MBACALFIR_MASK_DDR_CAL_TIMEOUT_ERR =    1
			[7]   MBACALFIR_MASK_DDR_CAL_RESET_TIMEOUT =  1
			[9]   MBACALFIR_MASK_WRQ_RRQ_HANG_ERR =       1
			[11]  MBACALFIR_MASK_ASYNC_IF_ERROR =         0
			[12]  MBACALFIR_MASK_CMD_PARITY_ERROR =       0
			[14]  MBACALFIR_MASK_RCD_CAL_PARITY_ERROR =   1
		MC01.PORT0.SRQ.MBACALFIR_MASK
			[2]   MBACALFIR_MASK_REFRESH_OVERRUN =        0   // recoverable_error (0,1,0)
			[5]   MBACALFIR_MASK_DDR_CAL_TIMEOUT_ERR =    0   // recoverable_error (0,1,0)
			[7]   MBACALFIR_MASK_DDR_CAL_RESET_TIMEOUT =  0   // recoverable_error (0,1,0)
			[9]   MBACALFIR_MASK_WRQ_RRQ_HANG_ERR =       0   // recoverable_error (0,1,0)
			[11]  MBACALFIR_MASK_ASYNC_IF_ERROR =         0   // checkstop (0,0,0)
			[12]  MBACALFIR_MASK_CMD_PARITY_ERROR =       0   // checkstop (0,0,0)
			[14]  MBACALFIR_MASK_RCD_CAL_PARITY_ERROR =   0   // recoverable_error (0,1,0)
		*/
		mca_and_or(id, mca_i, MBACALFIR_ACTION0,
		           ~(PPC_BIT(MBACALFIR_REFRESH_OVERRUN) |
		             PPC_BIT(MBACALFIR_DDR_CAL_TIMEOUT_ERR) |
		             PPC_BIT(MBACALFIR_DDR_CAL_RESET_TIMEOUT) |
		             PPC_BIT(MBACALFIR_WRQ_RRQ_HANG_ERR) |
		             PPC_BIT(MBACALFIR_ASYNC_IF_ERROR) |
		             PPC_BIT(MBACALFIR_CMD_PARITY_ERROR) |
		             PPC_BIT(MBACALFIR_RCD_CAL_PARITY_ERROR)),
		           0);
		mca_and_or(id, mca_i, MBACALFIR_ACTION1,
		           ~(PPC_BIT(MBACALFIR_REFRESH_OVERRUN) |
		             PPC_BIT(MBACALFIR_DDR_CAL_TIMEOUT_ERR) |
		             PPC_BIT(MBACALFIR_DDR_CAL_RESET_TIMEOUT) |
		             PPC_BIT(MBACALFIR_WRQ_RRQ_HANG_ERR) |
		             PPC_BIT(MBACALFIR_ASYNC_IF_ERROR) |
		             PPC_BIT(MBACALFIR_CMD_PARITY_ERROR) |
		             PPC_BIT(MBACALFIR_RCD_CAL_PARITY_ERROR)),
		           PPC_BIT(MBACALFIR_REFRESH_OVERRUN) |
		           PPC_BIT(MBACALFIR_DDR_CAL_TIMEOUT_ERR) |
		           PPC_BIT(MBACALFIR_DDR_CAL_RESET_TIMEOUT) |
		           PPC_BIT(MBACALFIR_WRQ_RRQ_HANG_ERR) |
		           PPC_BIT(MBACALFIR_RCD_CAL_PARITY_ERROR));
		mca_and_or(id, mca_i, MBACALFIR_MASK,
		           ~(PPC_BIT(MBACALFIR_REFRESH_OVERRUN) |
		             PPC_BIT(MBACALFIR_DDR_CAL_TIMEOUT_ERR) |
		             PPC_BIT(MBACALFIR_DDR_CAL_RESET_TIMEOUT) |
		             PPC_BIT(MBACALFIR_WRQ_RRQ_HANG_ERR) |
		             PPC_BIT(MBACALFIR_ASYNC_IF_ERROR) |
		             PPC_BIT(MBACALFIR_CMD_PARITY_ERROR) |
		             PPC_BIT(MBACALFIR_RCD_CAL_PARITY_ERROR)),
		           0);
	}
}

/*
 * 13.11 mss_draminit_training: Dram training
 *
 * a) p9_mss_draminit_training.C (mcbist) -- Nimbus
 * b) p9c_mss_draminit_training.C (mba) -- Cumulus
 *    - Prior to running this procedure will apply known DQ bad bits to prevent
 *      them from participating in training. This information is extracted from
 *      the bad DQ attribute and applied to Hardware
 *      - Marks the calibration fail array
 *    - External ZQ Calibration
 *    - Execute initial dram calibration (7 step - handled by HW)
 *    - This procedure will update the bad DQ attribute for each dimm based on
 *      its findings
 */
void istep_13_11(void)
{
	printk(BIOS_EMERG, "starting istep 13.11\n");
	int mcs_i, mca_i, dimm, rp;
	enum rank_selection ranks_present;

	report_istep(13,11);

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			ranks_present = NO_RANKS;
			for (dimm = 0; dimm < DIMMS_PER_MCA; dimm++) {
				if (!mca->dimm[dimm].present)
					continue;

				if (mca->dimm[dimm].mranks == 2)
					ranks_present |= DIMM0_ALL_RANKS << (2 * dimm);
				else
					ranks_present |= DIMM0_RANK0 << (2 * dimm);

				setup_and_execute_zqcal(mcs_i, mca_i, dimm);
			}

			/* IOM0.DDRPHY_PC_INIT_CAL_CONFIG0_P0 = 0 */
			mca_and_or(mcs_ids[mcs_i], mca_i, DDRPHY_PC_INIT_CAL_CONFIG0_P0, 0, 0);

			/*
			 * > Disable port fails as it doesn't appear the MC handles initial
			 * > cal timeouts correctly (cal_length.) BRS, see conversation with
			 * > Brad Michael
			MC01.PORT0.SRQ.MBA_FARB0Q =
				  [57]  MBA_FARB0Q_CFG_PORT_FAIL_DISABLE = 1
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, MBA_FARB0Q, ~0,
			           PPC_BIT(MBA_FARB0Q_CFG_PORT_FAIL_DISABLE));

			/*
			 * > The following registers must be configured to the correct
			 * > operating environment:
			 * > These are reset in phy_scominit
			 * > Section 5.2.5.10 SEQ ODT Write Configuration {0-3} on page 422
			 * > Section 5.2.6.1 WC Configuration 0 Register on page 434
			 * > Section 5.2.6.2 WC Configuration 1 Register on page 436
			 * > Section 5.2.6.3 WC Configuration 2 Register on page 438
			 *
			 * It would be nice to have the documentation mentioned above or at
			 * least know what it is about...
			 */

			clear_initial_cal_errors(mcs_i, mca_i);
			dp16_reset_delay_values(mcs_i, mca_i, ranks_present);
			dqs_align_turn_on_refresh(mcs_i, mca_i);

			/*
			 * List of calibration steps for RDIMM, in execution order:
			 * - ZQ calibration - calibrates DRAM output driver and on-die termination
			 *   values (already done)
			 * - Write leveling - compensates for skew caused by a fly-by topology
			 * - Initial pattern write - not exactly a calibration, but prepares patterns
			 *   for next steps
			 * - DQS align
			 * - RDCLK align
			 * - Read centering
			 * - Write Vref latching - not exactly a calibration, but required for next
			 *   steps; there is no help from PHY for that but it is simple to do
			 *   manually
			 * - Write centering
			 * - Coarse write/read
			 * - Custom read and/or write centering - performed in istep 13.12
			 * Some of these steps have pre- or post-workarounds, or both.
			 *
			 * All of those steps (except ZQ calibration) are executed for each rank pair
			 * before going to the next pair. Some of them require that there is no other
			 * activity on the controller so parallelization may not be possible.
			 *
			 * Quick reminder from set_rank_pairs() in 13.8 (RDIMM only):
			 * - RP0 primary - DIMM 0 rank 0
			 * - RP1 primary - DIMM 0 rank 1
			 * - RP2 primary - DIMM 1 rank 0
			 * - RP3 primary - DIMM 1 rank 1
			 */
			for (rp = 0; rp < 4; rp++) {
				if (!(ranks_present & (1 << rp)))
					continue;

				dump_cal_errors(mcs_i, mca_i);

				for (int i = 0; i < ARRAY_SIZE(steps); i++)
					dispatch_step(&steps[i], mcs_i, mca_i, rp, ranks_present);

				if (process_initial_cal_errors(mcs_i, mca_i) &&
				    !can_recover(mcs_i, mca_i, rp)) {
					die("Calibration failed for MCS%d MCA%d DIMM%d\n", mcs_i, mca_i, rp/2);
				}
			}

			/* Does not apply to DD2.* */
			//workarounds::dp16::modify_calibration_results();
		}

		/*
		 * Hostboot just logs the errors reported earlier (i.e. more than
		 * 1 nibble + 1 bit of bad DQ lines) "and lets PRD deconfigure based off
		 * of ATTR_BAD_DQ_BITMAP".
		 * TODO: what is PRD? How does it "deconfigure" and what? Quick glance
		 * at the code: it may have something to do with undocumented 0x0501082X
		 * SCOM registers, there are usr/diag/prdf/*//*.rule files with
		 * yacc/flex files to compile them. It also may be using 'attn'
		 * instruction.
		 */

		fir_unmask(mcs_i);
	}

	printk(BIOS_EMERG, "ending istep 13.11\n");
}
