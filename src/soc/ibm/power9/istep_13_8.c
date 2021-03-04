/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <cpu/power/vpd_data.h>
#include <console/console.h>

#include "istep_13_scom.h"

#define ATTR_PG			0xE000000000000000ull
#define FREQ_PB_MHZ		1866

/*
 * This function was generated from initfiles. Some of the registers used here
 * are not documented, except for occasional name of a constant written to it.
 * They also access registers at addresses for chiplet ID = 5 (Nest west), even
 * though the specified target is MCA. It is not clear if MCA offset has to be
 * added to SCOM address for those registers or not. Even logs from debug
 * version of Hostboot don't list the addresses explicitly, but by comparing
 * them with values read with 'pdbg' it seems that they use a stride of 0x10.
 *
 * Undocumented registers are marked with (?) in the comments.
 */
static void p9n_mca_scom(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	const int mca_mul = 0x10;
	/*
	 * Mixing rules:
	 * - rank configurations are the same for both DIMMs
	 * - fields for unpopulated DIMMs are initialized to all 0
	 *
	 * With those two assumptions values can be logically ORed to produce a
	 * common value without conditionals.
	 */
	int n_dimms = (mca->dimm[0].present && mca->dimm[1].present) ? 2 : 1;
	int mranks = mca->dimm[0].mranks | mca->dimm[1].mranks;
	int log_ranks = mca->dimm[0].log_ranks | mca->dimm[1].log_ranks;
	bool is_8H = (log_ranks / mranks) == 8;
	chiplet_id_t nest = mcs_to_nest[id];
	int vpd_idx = mca->dimm[0].present ? (mca->dimm[0].mranks == 2 ? 2 : 0) :
	                                     (mca->dimm[1].mranks == 2 ? 2 : 0);
	if (mca->dimm[0].present && mca->dimm[1].present)
		vpd_idx++;

	/* P9N2_MCS_PORT02_MCPERF0 (?)
	    [22-27] = 0x20                              // AMO_LIMIT
	*/
	scom_and_or_for_chiplet(nest, 0x05010823 + mca_i * mca_mul,
	                        ~PPC_BITMASK(22,27), PPC_SHIFT(0x20, 27));

	/* P9N2_MCS_PORT02_MCPERF2 (?)
	    [0-2]   = 1                                 // PF_DROP_VALUE0
	    [3-5]   = 3                                 // PF_DROP_VALUE1
	    [6-8]   = 5                                 // PF_DROP_VALUE2
	    [9-11]  = 7                                 // PF_DROP_VALUE3
	    [13-15] =                                   // REFRESH_BLOCK_CONFIG
		if has only one DIMM in MCA:
		  0b000 : if master ranks = 1
		  0b001 : if master ranks = 2
		  0b100 : if master ranks = 4
		// Per allowable DIMM mixing rules, we cannot mix different number of ranks on any single port
		if has both DIMMs in MCA:
		  0b010 : if master ranks = 1
		  0b011 : if master ranks = 2
		  0b100 : if master ranks = 4           // 4 mranks is the same for one and two DIMMs in MCA
	    [16] =                                      // ENABLE_REFRESH_BLOCK_SQ
	    [17] =                                      // ENABLE_REFRESH_BLOCK_NSQ, always the same value as [16]
		1 : if (1 < (DIMM0 + DIMM1 logical ranks) <= 8 && not (one DIMM, 4 mranks, 2H 3DS)
		0 : otherwise
	    [18]    = 0                                 // ENABLE_REFRESH_BLOCK_DISP
	    [28-31] = 0b0100                            // SQ_LFSR_CNTL
	    [50-54] = 0b11100                           // NUM_RMW_BUF
	    [61] = ATTR_ENABLE_MEM_EARLY_DATA_SCOM      // EN_ALT_ECR_ERR, 0?
	*/
	uint64_t ref_blk_cfg = mranks == 4 ? 0x4 :
	                       mranks == 2 ? (n_dimms == 1 ? 0x1 : 0x3) :
	                       n_dimms == 1 ? 0x0 : 0x2;
	uint64_t en_ref_blk = (log_ranks <= 1 || log_ranks > 8) ? 0 :
	                      (n_dimms == 1 && mranks == 4 && log_ranks == 8) ? 0 : 3;

	scom_and_or_for_chiplet(nest, 0x05010824 + mca_i * mca_mul,
	                        /* and */
	                        ~(PPC_BITMASK(0,11) | PPC_BITMASK(13,18) | PPC_BITMASK(28,31)
	                          | PPC_BITMASK(28,31) | PPC_BITMASK(50,54) | PPC_BIT(61)),
	                        /* or */
	                        PPC_SHIFT(1, 2) | PPC_SHIFT(3, 5) | PPC_SHIFT(5, 8)
	                        | PPC_SHIFT(7, 11) /* PF_DROP_VALUEs */
	                        | PPC_SHIFT(ref_blk_cfg, 15) | PPC_SHIFT(en_ref_blk, 17)
	                        | PPC_SHIFT(0x4, 31) | PPC_SHIFT(0x1C, 54));

	/* P9N2_MCS_PORT02_MCAMOC (?)
	    [1]     = 0                                 // FORCE_PF_DROP0
	    [4-28]  = 0x19fffff                         // WRTO_AMO_COLLISION_RULES
	    [29-31] = 1                                 // AMO_SIZE_SELECT, 128B_RW_64B_DATA
	*/
	scom_and_or_for_chiplet(nest, 0x05010825 + mca_i * mca_mul,
	                        ~(PPC_BIT(1) | PPC_BITMASK(4,31)),
	                        PPC_SHIFT(0x19FFFFF, 28) | PPC_SHIFT(1, 31));

	/* P9N2_MCS_PORT02_MCEPSQ (?)
	    [0-7]   = 1                                 // JITTER_EPSILON
	    // ATTR_PROC_EPS_READ_CYCLES_T* are calculated in 8.6
	    // Rounded up?
	    [8-15]  = (ATTR_PROC_EPS_READ_CYCLES_T0 + 6) / 4        // LOCAL_NODE_EPSILON
	    [16-23] = (ATTR_PROC_EPS_READ_CYCLES_T1 + 6) / 4        // NEAR_NODAL_EPSILON
	    [24-31] = (ATTR_PROC_EPS_READ_CYCLES_T1 + 6) / 4        // GROUP_EPSILON
	    [32-39] = (ATTR_PROC_EPS_READ_CYCLES_T2 + 6) / 4        // REMOTE_NODAL_EPSILON
	    [40-47] = (ATTR_PROC_EPS_READ_CYCLES_T2 + 6) / 4        // VECTOR_GROUP_EPSILON
	 */
	scom_and_or_for_chiplet(nest, 0x05010826 + mca_i * mca_mul, ~PPC_BITMASK(0,47),
	                        PPC_SHIFT(1, 7) /* FIXME: fill the rest with non-hardcoded values*/
	                        | PPC_SHIFT(4, 15) | PPC_SHIFT(4, 23) | PPC_SHIFT(4, 31)
	                        | PPC_SHIFT(0x19, 39) | PPC_SHIFT(0x19, 47));
//~ static const uint32_t EPSILON_R_T0_LE[] = {    7,    7,    8,    8,   10,   22 };  // T0, T1
//~ static const uint32_t EPSILON_R_T2_LE[] = {   67,   69,   71,   74,   79,  103 };  // T2

	/* P9N2_MCS_PORT02_MCBUSYQ (?)
	    [0]     = 1                                 // ENABLE_BUSY_COUNTERS
	    [1-3]   = 1                                 // BUSY_COUNTER_WINDOW_SELECT, 1024 cycles
	    [4-13]  = 38                                // BUSY_COUNTER_THRESHOLD0
	    [14-23] = 51                                // BUSY_COUNTER_THRESHOLD1
	    [24-33] = 64                                // BUSY_COUNTER_THRESHOLD2
	*/
	scom_and_or_for_chiplet(nest, 0x05010827 + mca_i * mca_mul, ~PPC_BITMASK(0,33),
	                        PPC_BIT(0) | PPC_SHIFT(1, 3) | PPC_SHIFT(38, 13)
	                        | PPC_SHIFT(51, 23) | PPC_SHIFT(64, 33));

	/* P9N2_MCS_PORT02_MCPERF3 (?)
	    [31] = 1                                    // ENABLE_CL0
	    [41] = 1                                    // ENABLE_AMO_MSI_RMW_ONLY
	    [43] = !ATTR_ENABLE_MEM_EARLY_DATA_SCOM     // ENABLE_CP_M_MDI0_LOCAL_ONLY, !0 = 1?
	    [44] = 1                                    // DISABLE_WRTO_IG
	    [45] = 1                                    // AMO_LIMIT_SEL
	*/
	scom_and_or_for_chiplet(nest, 0x0501082B + mca_i * mca_mul, ~PPC_BIT(43),
	                        PPC_BIT(31) | PPC_BIT(41) | PPC_BIT(44) | PPC_BIT(45));

	/* MC01.PORT0.SRQ.MBA_DSM0Q =
	    // These are set per port so all latencies should be calculated from both DIMMs (if present)
	    [0-5]   MBA_DSM0Q_CFG_RODT_START_DLY =  ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL
	    [6-11]  MBA_DSM0Q_CFG_RODT_END_DLY =    ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 5
	    [12-17] MBA_DSM0Q_CFG_WODT_START_DLY =  0
	    [18-23] MBA_DSM0Q_CFG_WODT_END_DLY =    5
	    [24-29] MBA_DSM0Q_CFG_WRDONE_DLY =      24
	    [30-35] MBA_DSM0Q_CFG_WRDATA_DLY =      ATTR_EFF_DRAM_CWL + ATTR_MSS_EFF_DPHY_WLO - 8
	    // Assume RDIMM, non-NVDIMM only
	    [36-41] MBA_DSM0Q_CFG_RDTAG_DLY =
		MSS_FREQ_EQ_1866:                   ATTR_EFF_DRAM_CL + 7
		MSS_FREQ_EQ_2133:                   ATTR_EFF_DRAM_CL + 7
		MSS_FREQ_EQ_2400:                   ATTR_EFF_DRAM_CL + 8
		MSS_FREQ_EQ_2666:                   ATTR_EFF_DRAM_CL + 9
	*/
	/* ATTR_MSS_EFF_DPHY_WLO = 1 from VPD, 3 from dump? */
	uint64_t rdtag_dly = mem_data.speed == 2666 ? 9 :
	                     mem_data.speed == 2400 ? 8 : 7;
	mca_and_or(id, mca_i, MBA_DSM0Q, ~PPC_BITMASK(0,41),
	           PPC_SHIFT(mca->cl - mem_data.cwl, MBA_DSM0Q_CFG_RODT_START_DLY) |
	           PPC_SHIFT(mca->cl - mem_data.cwl + 5, MBA_DSM0Q_CFG_RODT_END_DLY) |
	           PPC_SHIFT(5, MBA_DSM0Q_CFG_WODT_END_DLY) |
	           PPC_SHIFT(24, MBA_DSM0Q_CFG_WRDONE_DLY) |
	           PPC_SHIFT(mem_data.cwl + /* 1 */ 3 - 8, MBA_DSM0Q_CFG_WRDATA_DLY) |
	           PPC_SHIFT(mca->cl + rdtag_dly, MBA_DSM0Q_CFG_RDTAG_DLY));

	/* MC01.PORT0.SRQ.MBA_TMR0Q =
	    [0-3]   MBA_TMR0Q_RRDM_DLY =
		MSS_FREQ_EQ_1866:             8
		MSS_FREQ_EQ_2133:             9
		MSS_FREQ_EQ_2400:             10
		MSS_FREQ_EQ_2666:             11
	    [4-7]   MBA_TMR0Q_RRSMSR_DLY =  4
	    [8-11]  MBA_TMR0Q_RRSMDR_DLY =  4
	    [12-15] MBA_TMR0Q_RROP_DLY =    ATTR_EFF_DRAM_TCCD_L
	    [16-19] MBA_TMR0Q_WWDM_DLY =
		MSS_FREQ_EQ_1866:             8
		MSS_FREQ_EQ_2133:             9
		MSS_FREQ_EQ_2400:             10
		MSS_FREQ_EQ_2666:             11
	    [20-23] MBA_TMR0Q_WWSMSR_DLY =  4
	    [24-27] MBA_TMR0Q_WWSMDR_DLY =  4
	    [28-31] MBA_TMR0Q_WWOP_DLY =    ATTR_EFF_DRAM_TCCD_L
	    [32-36] MBA_TMR0Q_RWDM_DLY =        // same as below
	    [37-41] MBA_TMR0Q_RWSMSR_DLY =      // same as below
	    [42-46] MBA_TMR0Q_RWSMDR_DLY =
		MSS_FREQ_EQ_1866:             ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 8
		MSS_FREQ_EQ_2133:             ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 9
		MSS_FREQ_EQ_2400:             ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 10
		MSS_FREQ_EQ_2666:             ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 11
	    [47-50] MBA_TMR0Q_WRDM_DLY =
		MSS_FREQ_EQ_1866:             ATTR_EFF_DRAM_CWL - ATTR_EFF_DRAM_CL + 8
		MSS_FREQ_EQ_2133:             ATTR_EFF_DRAM_CWL - ATTR_EFF_DRAM_CL + 9
		MSS_FREQ_EQ_2400:             ATTR_EFF_DRAM_CWL - ATTR_EFF_DRAM_CL + 10
		MSS_FREQ_EQ_2666:             ATTR_EFF_DRAM_CWL - ATTR_EFF_DRAM_CL + 11
	    [51-56] MBA_TMR0Q_WRSMSR_DLY =      // same as below
	    [57-62] MBA_TMR0Q_WRSMDR_DLY =    ATTR_EFF_DRAM_CWL + ATTR_EFF_DRAM_TWTR_S + 4
	*/
	uint64_t var_dly = mem_data.speed == 2666 ? 11 :
	                   mem_data.speed == 2400 ? 10 :
	                   mem_data.speed == 2133 ? 9 : 8;
	mca_and_or(id, mca_i, MBA_TMR0Q, PPC_BIT(63),
	           PPC_SHIFT(var_dly, MBA_TMR0Q_RRDM_DLY) |
	           PPC_SHIFT(4, MBA_TMR0Q_RRSMSR_DLY) |
	           PPC_SHIFT(4, MBA_TMR0Q_RRSMDR_DLY) |
	           PPC_SHIFT(mca->nccd_l, MBA_TMR0Q_RROP_DLY) |
	           PPC_SHIFT(var_dly, MBA_TMR0Q_WWDM_DLY) |
	           PPC_SHIFT(4, MBA_TMR0Q_WWSMSR_DLY) |
	           PPC_SHIFT(4, MBA_TMR0Q_WWSMDR_DLY) |
	           PPC_SHIFT(mca->nccd_l, MBA_TMR0Q_WWOP_DLY) |
	           PPC_SHIFT(mca->cl - mem_data.cwl + var_dly, MBA_TMR0Q_RWDM_DLY) |
	           PPC_SHIFT(mca->cl - mem_data.cwl + var_dly, MBA_TMR0Q_RWSMSR_DLY) |
	           PPC_SHIFT(mca->cl - mem_data.cwl + var_dly, MBA_TMR0Q_RWSMDR_DLY) |
	           PPC_SHIFT(mem_data.cwl - mca->cl + var_dly, MBA_TMR0Q_WRDM_DLY) |
	           PPC_SHIFT(mem_data.cwl + mca->nwtr_s + 4, MBA_TMR0Q_WRSMSR_DLY) |
	           PPC_SHIFT(mem_data.cwl + mca->nwtr_s + 4, MBA_TMR0Q_WRSMDR_DLY));

	/* MC01.PORT0.SRQ.MBA_TMR1Q =
	    [0-3]   MBA_TMR1Q_RRSBG_DLY =   ATTR_EFF_DRAM_TCCD_L
	    [4-9]   MBA_TMR1Q_WRSBG_DLY =   ATTR_EFF_DRAM_CWL + ATTR_EFF_DRAM_TWTR_L + 4
	    [10-15] MBA_TMR1Q_CFG_TFAW =    ATTR_EFF_DRAM_TFAW
	    [16-20] MBA_TMR1Q_CFG_TRCD =    ATTR_EFF_DRAM_TRCD
	    [21-25] MBA_TMR1Q_CFG_TRP =     ATTR_EFF_DRAM_TRP
	    [26-31] MBA_TMR1Q_CFG_TRAS =    ATTR_EFF_DRAM_TRAS
	    [41-47] MBA_TMR1Q_CFG_WR2PRE =  ATTR_EFF_DRAM_CWL + ATTR_EFF_DRAM_TWR + 4
	    [48-51] MBA_TMR1Q_CFG_RD2PRE =  ATTR_EFF_DRAM_TRTP
	    [52-55] MBA_TMR1Q_TRRD =        ATTR_EFF_DRAM_TRRD_S
	    [56-59] MBA_TMR1Q_TRRD_SBG =    ATTR_EFF_DRAM_TRRD_L
	    [60-63] MBA_TMR1Q_CFG_ACT_TO_DIFF_RANK_DLY =    // var_dly from above
		MSS_FREQ_EQ_1866:           8
		MSS_FREQ_EQ_2133:           9
		MSS_FREQ_EQ_2400:           10
		MSS_FREQ_EQ_2666:           11
	*/
	mca_and_or(id, mca_i, MBA_TMR1Q, 0,
	           PPC_SHIFT(mca->nccd_l, MBA_TMR1Q_RRSBG_DLY) |
	           PPC_SHIFT(mem_data.cwl + mca->nwtr_l + 4, MBA_TMR1Q_WRSBG_DLY) |
	           PPC_SHIFT(mca->nfaw, MBA_TMR1Q_CFG_TFAW) |
	           PPC_SHIFT(mca->nrcd, MBA_TMR1Q_CFG_TRCD) |
	           PPC_SHIFT(mca->nrp, MBA_TMR1Q_CFG_TRP) |
	           PPC_SHIFT(mca->nras, MBA_TMR1Q_CFG_TRAS) |
	           PPC_SHIFT(mem_data.cwl + mca->nwr + 4, MBA_TMR1Q_CFG_WR2PRE) |
	           PPC_SHIFT(mem_data.nrtp, MBA_TMR1Q_CFG_RD2PRE) |
	           PPC_SHIFT(mca->nrrd_s, MBA_TMR1Q_TRRD) |
	           PPC_SHIFT(mca->nrrd_l, MBA_TMR1Q_TRRD_SBG) |
	           PPC_SHIFT(var_dly, MBA_TMR1Q_CFG_ACT_TO_DIFF_RANK_DLY));

	/* MC01.PORT0.SRQ.MBA_WRQ0Q =
	    [5]     MBA_WRQ0Q_CFG_WRQ_FIFO_MODE =               0   // ATTR_MSS_REORDER_QUEUE_SETTING, 0 = reorder
	    [6]     MBA_WRQ0Q_CFG_DISABLE_WR_PG_MODE =          1
	    [55-58] MBA_WRQ0Q_CFG_WRQ_ACT_NUM_WRITES_PENDING =  8
	*/
	mca_and_or(id, mca_i, MBA_WRQ0Q,
	           ~(PPC_BIT(MBA_WRQ0Q_CFG_WRQ_FIFO_MODE) |
	             PPC_BIT(MBA_WRQ0Q_CFG_DISABLE_WR_PG_MODE) |
	             PPC_BITMASK(55, 58)),
	           PPC_BIT(MBA_WRQ0Q_CFG_DISABLE_WR_PG_MODE) |
	           PPC_SHIFT(8, MBA_WRQ0Q_CFG_WRQ_ACT_NUM_WRITES_PENDING));

	/* MC01.PORT0.SRQ.MBA_RRQ0Q =
	    [6]     MBA_RRQ0Q_CFG_RRQ_FIFO_MODE =               0   // ATTR_MSS_REORDER_QUEUE_SETTING
	    [57-60] MBA_RRQ0Q_CFG_RRQ_ACT_NUM_READS_PENDING =   8
	*/
	mca_and_or(id, mca_i, MBA_RRQ0Q,
	           ~(PPC_BIT(MBA_RRQ0Q_CFG_RRQ_FIFO_MODE) | PPC_BITMASK(57, 60)),
	           PPC_SHIFT(8, MBA_RRQ0Q_CFG_RRQ_ACT_NUM_READS_PENDING));

	/* MC01.PORT0.SRQ.MBA_FARB0Q =
	    if (l_TGT3_ATTR_MSS_MRW_DRAM_2N_MODE == 0x02 || (l_TGT3_ATTR_MSS_MRW_DRAM_2N_MODE == 0x00 && l_TGT2_ATTR_MSS_VPD_MR_MC_2N_MODE_AUTOSET == 0x02))
	      [17] MBA_FARB0Q_CFG_2N_ADDR =         1     // Default is auto for mode, 1N from VPD, so [17] = 0
	    [38] MBA_FARB0Q_CFG_PARITY_AFTER_CMD =  1
	    [61-63] MBA_FARB0Q_CFG_OPT_RD_SIZE =    3
	*/
	mca_and_or(id, mca_i, MBA_FARB0Q,
	           ~(PPC_BIT(MBA_FARB0Q_CFG_2N_ADDR) |
	             PPC_BIT(MBA_FARB0Q_CFG_PARITY_AFTER_CMD) |
	             PPC_BITMASK(61, 63)),
	           PPC_BIT(MBA_FARB0Q_CFG_PARITY_AFTER_CMD) |
	           PPC_SHIFT(3, MBA_FARB0Q_CFG_OPT_RD_SIZE));

	/* MC01.PORT0.SRQ.MBA_FARB1Q =
	    [0-2]   MBA_FARB1Q_CFG_SLOT0_S0_CID = 0
	    [3-5]   MBA_FARB1Q_CFG_SLOT0_S1_CID = 4
	    [6-8]   MBA_FARB1Q_CFG_SLOT0_S2_CID = 2
	    [9-11]  MBA_FARB1Q_CFG_SLOT0_S3_CID = 6
	    if (DIMM0 is 8H 3DS)
	      [12-14] MBA_FARB1Q_CFG_SLOT0_S4_CID =   1
	      [15-17] MBA_FARB1Q_CFG_SLOT0_S5_CID =   5
	      [18-20] MBA_FARB1Q_CFG_SLOT0_S6_CID =   3
	      [21-23] MBA_FARB1Q_CFG_SLOT0_S7_CID =   7
	    else
	      [12-14] MBA_FARB1Q_CFG_SLOT0_S4_CID =   0
	      [15-17] MBA_FARB1Q_CFG_SLOT0_S5_CID =   4
	      [18-20] MBA_FARB1Q_CFG_SLOT0_S6_CID =   2
	      [21-23] MBA_FARB1Q_CFG_SLOT0_S7_CID =   6
	    if (DIMM0 has 4 master ranks)
	      [12-14] MBA_FARB1Q_CFG_SLOT0_S4_CID =   4     // TODO: test if all slots with 4R DIMMs works with that
	    [24-26] MBA_FARB1Q_CFG_SLOT1_S0_CID = 0
	    [27-29] MBA_FARB1Q_CFG_SLOT1_S1_CID = 4
	    [30-32] MBA_FARB1Q_CFG_SLOT1_S2_CID = 2
	    [33-35] MBA_FARB1Q_CFG_SLOT1_S3_CID = 6
	    if (DIMM1 is 8H 3DS)
	      [36-38] MBA_FARB1Q_CFG_SLOT1_S4_CID =   1
	      [39-41] MBA_FARB1Q_CFG_SLOT1_S5_CID =   5
	      [42-44] MBA_FARB1Q_CFG_SLOT1_S6_CID =   3
	      [45-47] MBA_FARB1Q_CFG_SLOT1_S7_CID =   7
	    else
	      [36-38] MBA_FARB1Q_CFG_SLOT1_S4_CID =   0
	      [39-41] MBA_FARB1Q_CFG_SLOT1_S5_CID =   4
	      [42-44] MBA_FARB1Q_CFG_SLOT1_S6_CID =   2
	      [45-47] MBA_FARB1Q_CFG_SLOT1_S7_CID =   6
	    if (DIMM1 has 4 master ranks)
	      [36-38] MBA_FARB1Q_CFG_SLOT1_S4_CID =   4     // TODO: test if all slots with 4R DIMMs works with that
	*/
	/* Due to allowable DIMM mixing rules, ranks of both DIMMs are the same */
	uint64_t cids_even = (0 << 9) | (4 << 6) | (2 << 3) | (6 << 0);
	uint64_t cids_odd =  (1 << 9) | (5 << 6) | (3 << 3) | (7 << 0);
	uint64_t cids_4_7 = is_8H ? cids_odd : cids_even;
	/* Not sure if this is even supported, there is no MT VPD data for this case */
	if (mranks == 4)
		cids_4_7 = (cids_4_7 & ~(7ull << 9)) | (4 << 9);

	mca_and_or(id, mca_i, MBA_FARB1Q, ~PPC_BITMASK(0, 47),
	           PPC_SHIFT(cids_even, MBA_FARB1Q_CFG_SLOT0_S3_CID) |
	           PPC_SHIFT(cids_4_7, MBA_FARB1Q_CFG_SLOT0_S7_CID) |
	           PPC_SHIFT(cids_even, MBA_FARB1Q_CFG_SLOT1_S3_CID) |
	           PPC_SHIFT(cids_4_7, MBA_FARB1Q_CFG_SLOT1_S7_CID));

	/* MC01.PORT0.SRQ.MBA_FARB2Q =
	    F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of uint8_t X, big endian numbering
	    [0-3]   MBA_FARB2Q_CFG_RANK0_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][0][0])
	    [4-7]   MBA_FARB2Q_CFG_RANK1_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][0][1])
	    [8-11]  MBA_FARB2Q_CFG_RANK2_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][0][2])  // always 0
	    [12-15] MBA_FARB2Q_CFG_RANK3_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][0][3])  // always 0
	    [16-19] MBA_FARB2Q_CFG_RANK4_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][1][0])
	    [20-23] MBA_FARB2Q_CFG_RANK5_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][1][1])
	    [24-27] MBA_FARB2Q_CFG_RANK6_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][1][2])  // always 0
	    [28-31] MBA_FARB2Q_CFG_RANK7_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][1][3])  // always 0
	    [32-35] MBA_FARB2Q_CFG_RANK0_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][0][0])
	    [36-39] MBA_FARB2Q_CFG_RANK1_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][0][1])
	    [40-43] MBA_FARB2Q_CFG_RANK2_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][0][2])  // always 0
	    [44-47] MBA_FARB2Q_CFG_RANK3_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][0][3])  // always 0
	    [48-51] MBA_FARB2Q_CFG_RANK4_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][1][0])
	    [52-55] MBA_FARB2Q_CFG_RANK5_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][1][1])
	    [56-59] MBA_FARB2Q_CFG_RANK6_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][1][2])  // always 0
	    [60-63] MBA_FARB2Q_CFG_RANK7_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][1][3])  // always 0
	*/
	#define F(X)	((((X) >> 4) & 0xc) | (((X) >> 2) & 0x3))
	mca_and_or(id, mca_i, MBA_FARB2Q, 0,
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][0][0]), MBA_FARB2Q_CFG_RANK0_RD_ODT) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][0][1]), MBA_FARB2Q_CFG_RANK1_RD_ODT) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][0]), MBA_FARB2Q_CFG_RANK4_RD_ODT) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][1]), MBA_FARB2Q_CFG_RANK5_RD_ODT) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][0]), MBA_FARB2Q_CFG_RANK0_WR_ODT) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][1]), MBA_FARB2Q_CFG_RANK1_WR_ODT) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][1][0]), MBA_FARB2Q_CFG_RANK4_WR_ODT) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][1][1]), MBA_FARB2Q_CFG_RANK5_WR_ODT) );
	#undef F

	/* MC01.PORT0.SRQ.PC.MBAREF0Q =
	    [5-7]   MBAREF0Q_CFG_REFRESH_PRIORITY_THRESHOLD = 3
	    [8-18]  MBAREF0Q_CFG_REFRESH_INTERVAL =           ATTR_EFF_DRAM_TREFI / (8 * (DIMM0 + DIMM1 logical ranks))
	    [30-39] MBAREF0Q_CFG_TRFC =                       ATTR_EFF_DRAM_TRFC
	    [40-49] MBAREF0Q_CFG_REFR_TSV_STACK =             ATTR_EFF_DRAM_TRFC_DLR
	    [50-60] MBAREF0Q_CFG_REFR_CHECK_INTERVAL =        ((ATTR_EFF_DRAM_TREFI / 8) * 6) / 5
	*/
	/*
	 * Hostboot writes slightly lower REFR_CHECK_INTERVAL, 1544 vs 1560, because
	 * it uses 99% of tREFI in 7.4 in eff_dimm::dram_trefi(). If this causes any
	 * issues we can do the same, but for now let's try to avoid floating point
	 * arithmetic.
	 */
	mca_and_or(id, mca_i, MBAREF0Q, ~(PPC_BITMASK(5, 18) | PPC_BITMASK(30, 60)),
	           PPC_SHIFT(3, MBAREF0Q_CFG_REFRESH_PRIORITY_THRESHOLD) |
	           PPC_SHIFT(mem_data.nrefi / (8 * 2 * log_ranks), MBAREF0Q_CFG_REFRESH_INTERVAL) |
	           PPC_SHIFT(mca->nrfc, MBAREF0Q_CFG_TRFC) |
	           PPC_SHIFT(mca->nrfc_dlr, MBAREF0Q_CFG_REFR_TSV_STACK) |
	           PPC_SHIFT(((mem_data.nrefi / 8) * 6) / 5, MBAREF0Q_CFG_REFR_CHECK_INTERVAL));

	/* MC01.PORT0.SRQ.PC.MBARPC0Q =
	    [6-10]  MBARPC0Q_CFG_PUP_AVAIL =
	      MSS_FREQ_EQ_1866: 6
	      MSS_FREQ_EQ_2133: 7
	      MSS_FREQ_EQ_2400: 8
	      MSS_FREQ_EQ_2666: 9
	    [11-15] MBARPC0Q_CFG_PDN_PUP =
	      MSS_FREQ_EQ_1866: 5
	      MSS_FREQ_EQ_2133: 6
	      MSS_FREQ_EQ_2400: 6
	      MSS_FREQ_EQ_2666: 7
	    [16-20] MBARPC0Q_CFG_PUP_PDN =
	      MSS_FREQ_EQ_1866: 5
	      MSS_FREQ_EQ_2133: 6
	      MSS_FREQ_EQ_2400: 6
	      MSS_FREQ_EQ_2666: 7
	    [21] MBARPC0Q_RESERVED_21 =         // MCP_PORT0_SRQ_PC_MBARPC0Q_CFG_QUAD_RANK_ENC
	      (l_def_MASTER_RANKS_DIMM0 == 4): 1
	      (l_def_MASTER_RANKS_DIMM0 != 4): 0
	*/
	/* Perhaps these can be done by ns_to_nck(), but Hostboot used a forest of ifs */
	uint64_t pup_avail = mem_data.speed == 1866 ? 6 :
	                     mem_data.speed == 2133 ? 7 :
	                     mem_data.speed == 2400 ? 8 : 9;
	uint64_t p_up_dn =   mem_data.speed == 1866 ? 5 :
	                     mem_data.speed == 2666 ? 7 : 6;
	mca_and_or(id, mca_i, MBARPC0Q, ~PPC_BITMASK(6, 21),
	           PPC_SHIFT(pup_avail, MBARPC0Q_CFG_PUP_AVAIL) |
	           PPC_SHIFT(p_up_dn, MBARPC0Q_CFG_PDN_PUP) |
	           PPC_SHIFT(p_up_dn, MBARPC0Q_CFG_PUP_PDN) |
	           (mranks == 4 ? PPC_BIT(MBARPC0Q_RESERVED_21) : 0));

	/* MC01.PORT0.SRQ.PC.MBASTR0Q =
	    [12-16] MBASTR0Q_CFG_TCKESR = 5
	    [17-21] MBASTR0Q_CFG_TCKSRE =
	      MSS_FREQ_EQ_1866: 10
	      MSS_FREQ_EQ_2133: 11
	      MSS_FREQ_EQ_2400: 12
	      MSS_FREQ_EQ_2666: 14
	    [22-26] MBASTR0Q_CFG_TCKSRX =
	      MSS_FREQ_EQ_1866: 10
	      MSS_FREQ_EQ_2133: 11
	      MSS_FREQ_EQ_2400: 12
	      MSS_FREQ_EQ_2666: 14
	    [27-37] MBASTR0Q_CFG_TXSDLL =
	      MSS_FREQ_EQ_1866: 597
	      MSS_FREQ_EQ_2133: 768
	      MSS_FREQ_EQ_2400: 768
	      MSS_FREQ_EQ_2666: 939
	    [46-56] MBASTR0Q_CFG_SAFE_REFRESH_INTERVAL = ATTR_EFF_DRAM_TREFI / (8 * (DIMM0 + DIMM1 logical ranks))
	*/
	uint64_t tcksr_ex = mem_data.speed == 1866 ? 10 :
	                    mem_data.speed == 2133 ? 11 :
	                    mem_data.speed == 2400 ? 12 : 14;
	uint64_t txsdll = mem_data.speed == 1866 ? 597 :
	                  mem_data.speed == 2666 ? 939 : 768;
	mca_and_or(id, mca_i, MBASTR0Q, ~(PPC_BITMASK(12, 37) | PPC_BITMASK(46, 56)),
	           PPC_SHIFT(5, MBASTR0Q_CFG_TCKESR) |
	           PPC_SHIFT(tcksr_ex, MBASTR0Q_CFG_TCKSRE) |
	           PPC_SHIFT(tcksr_ex, MBASTR0Q_CFG_TCKSRX) |
	           PPC_SHIFT(txsdll, MBASTR0Q_CFG_TXSDLL) |
	           PPC_SHIFT(mem_data.nrefi /
	                     (8 * (mca->dimm[0].log_ranks + mca->dimm[1].log_ranks)),
	                     MBASTR0Q_CFG_SAFE_REFRESH_INTERVAL));

	/* MC01.PORT0.ECC64.SCOM.RECR =
	    [16-18] MBSECCQ_VAL_TO_DATA_DELAY =
	      l_TGT4_ATTR_MC_SYNC_MODE == 1:  5
	      l_def_mn_freq_ratio < 915:      3
	      l_def_mn_freq_ratio < 1150:     4
	      l_def_mn_freq_ratio < 1300:     5
	      l_def_mn_freq_ratio >= 1300:    6
	    [19]    MBSECCQ_DELAY_VALID_1X =  0
	    [20-21] MBSECCQ_NEST_VAL_TO_DATA_DELAY =
	      l_TGT4_ATTR_MC_SYNC_MODE == 1:  1
	      l_def_mn_freq_ratio < 1040:     1
	      l_def_mn_freq_ratio < 1150:     0
	      l_def_mn_freq_ratio < 1215:     1
	      l_def_mn_freq_ratio < 1300:     0
	      l_def_mn_freq_ratio < 1400:     1
	      l_def_mn_freq_ratio >= 1400:    0
	    [22]    MBSECCQ_DELAY_NONBYPASS =
	      l_TGT4_ATTR_MC_SYNC_MODE == 1:  0
	      l_def_mn_freq_ratio < 1215:     0
	      l_def_mn_freq_ratio >= 1215:    1
	    [40]    MBSECCQ_RESERVED_36_43 =        // MCP_PORT0_ECC64_ECC_SCOM_MBSECCQ_BYPASS_TENURE_3
	      l_TGT4_ATTR_MC_SYNC_MODE == 1:  0
	      l_TGT4_ATTR_MC_SYNC_MODE == 0:  1
	*/
	/* Assume asynchronous mode */
	/*
	 * From Hostboot:
	 *     l_def_mn_freq_ratio = 1000 * ATTR_MSS_FREQ / ATTR_FREQ_PB_MHZ;
	 * ATTR_MSS_FREQ is in MT/s (sigh), ATTR_FREQ_PB_MHZ is 1866 MHz (from talos.xml).
	 */
	uint64_t mn_freq_ratio = 1000 * mem_data.speed / FREQ_PB_MHZ;
	uint64_t val_to_data = mn_freq_ratio < 915 ? 3 :
	                       mn_freq_ratio < 1150 ? 4 :
	                       mn_freq_ratio < 1300 ? 5 : 6;
	uint64_t nest_val_to_data = mn_freq_ratio < 1040 ? 1 :
	                            mn_freq_ratio < 1150 ? 0 :
	                            mn_freq_ratio < 1215 ? 1 :
	                            mn_freq_ratio < 1300 ? 0 :
	                            mn_freq_ratio < 1400 ? 1 : 0;
	mca_and_or(id, mca_i, RECR, ~(PPC_BITMASK(16, 22) | PPC_BIT(MBSECCQ_RESERVED_40)),
	           PPC_SHIFT(val_to_data, MBSECCQ_VAL_TO_DATA_DELAY) |
	           PPC_SHIFT(nest_val_to_data, MBSECCQ_NEST_VAL_TO_DATA_DELAY) |
	           (mn_freq_ratio < 1215 ? 0 : PPC_BIT(MBSECCQ_DELAY_NONBYPASS)) |
	           PPC_BIT(MBSECCQ_RESERVED_40));

	/* MC01.PORT0.ECC64.SCOM.DBGR =
	    [9]     DBGR_ECC_WAT_ACTION_SELECT =  0
	    [10-11] DBGR_ECC_WAT_SOURCE =         0
	*/
	mca_and_or(id, mca_i, DBGR, ~PPC_BITMASK(9, 11), 0);

	/* MC01.PORT0.WRITE.WRTCFG =
	    [9] = 1     // MCP_PORT0_WRITE_NEW_WRITE_64B_MODE   this is marked as RO const 0 for bits 8-63 in docs!
	*/
	mca_and_or(id, mca_i, WRTCFG, ~0ull, PPC_BIT(9));
}

static void thermal_throttle_scominit(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/* Set power control register */
	/* MC01.PORT0.SRQ.PC.MBARPC0Q =
	    [3-5]   MBARPC0Q_CFG_MIN_MAX_DOMAINS =                          0
	    [22]    MBARPC0Q_CFG_MIN_DOMAIN_REDUCTION_ENABLE =
	      if ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == PD_AND_STR_OFF:    0     // default
	      else:                                                         1
	    [23-32] MBARPC0Q_CFG_MIN_DOMAIN_REDUCTION_TIME =                959
	*/
	mca_and_or(id, mca_i, MBARPC0Q, ~(PPC_BITMASK(3, 5) | PPC_BITMASK(22, 32)),
	           PPC_SHIFT(959, MBARPC0Q_CFG_MIN_DOMAIN_REDUCTION_TIME));

	/* Set STR register */
	/* MC01.PORT0.SRQ.PC.MBASTR0Q =
	    [0]     MBASTR0Q_CFG_STR_ENABLE =
	      ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == PD_AND_STR:           1
	      ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == PD_AND_STR_CLK_STOP:  1
	      ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == POWER_DOWN:           0
	      ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == PD_AND_STR_OFF:       0     // default
	    [2-11]  MBASTR0Q_CFG_ENTER_STR_TIME =                           1023
	*/
	mca_and_or(id, mca_i, MBASTR0Q, ~(PPC_BIT(0) | PPC_BITMASK(2, 11)),
	           PPC_SHIFT(1023, MBASTR0Q_CFG_ENTER_STR_TIME));

	/* Set N/M throttling control register */
	/* TODO: these attributes are calculated in 7.4, implement this */
	/* MC01.PORT0.SRQ.MBA_FARB3Q =
	    [0-14]  MBA_FARB3Q_CFG_NM_N_PER_SLOT = ATTR_MSS_RUNTIME_MEM_THROTTLED_N_COMMANDS_PER_SLOT[mss::index(MCA)]
	    [15-30] MBA_FARB3Q_CFG_NM_N_PER_PORT = ATTR_MSS_RUNTIME_MEM_THROTTLED_N_COMMANDS_PER_PORT[mss::index(MCA)]
	    [31-44] MBA_FARB3Q_CFG_NM_M =          ATTR_MSS_MRW_MEM_M_DRAM_CLOCKS     // default 0x200
	    [45-47] MBA_FARB3Q_CFG_NM_RAS_WEIGHT = 0
	    [48-50] MBA_FARB3Q_CFG_NM_CAS_WEIGHT = 1
	    // Set to disable permanently due to hardware design bug (HW403028) that won't be changed
	    [53]    MBA_FARB3Q_CFG_NM_CHANGE_AFTER_SYNC = 0
	*/
	printk(BIOS_EMERG, "Please FIXME: ATTR_MSS_RUNTIME_MEM_THROTTLED_N_COMMANDS_PER_SLOT\n");
	/* Values dumped after Hostboot's calculations, may be different for other DIMMs */
	uint64_t nm_n_per_slot = 0x80;
	uint64_t nm_n_per_port = 0x80;
	mca_and_or(id, mca_i, MBA_FARB3Q, ~(PPC_BITMASK(0, 50) | PPC_BIT(53)),
	           PPC_SHIFT(nm_n_per_slot, MBA_FARB3Q_CFG_NM_N_PER_SLOT) |
	           PPC_SHIFT(nm_n_per_port, MBA_FARB3Q_CFG_NM_N_PER_PORT) |
	           PPC_SHIFT(0x200, MBA_FARB3Q_CFG_NM_M) |
	           PPC_SHIFT(1, MBA_FARB3Q_CFG_NM_CAS_WEIGHT));

	/* Set safemode throttles */
	/* MC01.PORT0.SRQ.MBA_FARB4Q =
	    [27-41] MBA_FARB4Q_EMERGENCY_N = ATTR_MSS_RUNTIME_MEM_THROTTLED_N_COMMANDS_PER_PORT[mss::index(MCA)]  // BUG? var name says per_slot...
	    [42-55] MBA_FARB4Q_EMERGENCY_M = ATTR_MSS_MRW_MEM_M_DRAM_CLOCKS
	*/
	mca_and_or(id, mca_i, MBA_FARB4Q, ~PPC_BITMASK(27, 55),
	           PPC_SHIFT(nm_n_per_port, MBA_FARB4Q_EMERGENCY_N) |
	           PPC_SHIFT(0x200, MBA_FARB4Q_EMERGENCY_M));
}

/*
 * Values set in this function are mostly for magic MCA, other (functional) MCAs
 * are set later. If all of these registers are later written with proper values
 * for functional MCAs, maybe this can be called just for magic, non-functional
 * ones to save time, but for now do it in a way the Hostboot does it.
 */
static void p9n_ddrphy_scom(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;
	/*
	 * Hostboot sets this to proper value in phy_scominit(), but I don't see
	 * why. Speed is the same for whole MCBIST anyway.
	 */
	uint64_t strength = mem_data.speed == 1866 ? 1 :
			    mem_data.speed == 2133 ? 2 :
			    mem_data.speed == 2400 ? 4 : 8;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_DLL_VREG_CONTROL0_P0_{0,1,2,3,4} =
		  [48-50] RXREG_VREG_COMPCON_DC = 3
		  [52-59] = 0x74:
			  [53-55] RXREG_VREG_DRVCON_DC =  0x7
			  [56-58] RXREG_VREG_REF_SEL_DC = 0x2
		  [62-63] = 0:
			  [62] DLL_DRVREN_MODE =      POWER8 mode (thermometer style, enabling all drivers up to the one that is used)
			  [63] DLL_CAL_CKTS_ACTIVE =  After VREG calibration, some analog circuits are powered down
		*/
		/* Same as default value after reset? */
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DLL_VREG_CONTROL0_P0_0,
		              ~(PPC_BITMASK(48, 50) | PPC_BITMASK(52, 59) | PPC_BITMASK(62, 63)),
		              PPC_SHIFT(3, RXREG_VREG_COMPCON_DC) |
		              PPC_SHIFT(7, RXREG_VREG_DRVCON_DC) |
		              PPC_SHIFT(2, RXREG_VREG_REF_SEL_DC));

		/* IOM0.DDRPHY_DP16_DLL_VREG_CONTROL1_P0_{0,1,2,3,4} =
		  [48-50] RXREG_VREG_COMPCON_DC = 3
		  [52-59] = 0x74:
			  [53-55] RXREG_VREG_DRVCON_DC =  0x7
			  [56-58] RXREG_VREG_REF_SEL_DC = 0x2
		  [62-63] = 0:
			  [62] DLL_DRVREN_MODE =      POWER8 mode (thermometer style, enabling all drivers up to the one that is used)
			  [63] DLL_CAL_CKTS_ACTIVE =  After VREG calibration, some analog circuits are powered down
		*/
		/* Same as default value after reset? */
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DLL_VREG_CONTROL1_P0_0,
		              ~(PPC_BITMASK(48, 50) | PPC_BITMASK(52, 59) | PPC_BITMASK(62, 63)),
		              PPC_SHIFT(3, RXREG_VREG_COMPCON_DC) |
		              PPC_SHIFT(7, RXREG_VREG_DRVCON_DC) |
		              PPC_SHIFT(2, RXREG_VREG_REF_SEL_DC));

		/* IOM0.DDRPHY_DP16_WRCLK_PR_P0_{0,1,2,3,4} =
		  // For zero delay simulations, or simulations where the delay of the SysClk tree and the WrClk tree are equal,
		  // set this field to 60h
		  [49-55] TSYS_WRCLK = 0x60
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WRCLK_PR_P0_0,
		              ~PPC_BITMASK(49, 55), PPC_SHIFT(0x60, TSYS_WRCLK));

		/* IOM0.DDRPHY_DP16_IO_TX_CONFIG0_P0_{0,1,2,3,4} =
		  [48-51] STRENGTH =                    0x4 // 2400 MT/s
		  [52]    DD2_RESET_READ_FIX_DISABLE =  0   // Enable the DD2 function to remove the register reset on read feature
							    // on status registers
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_IO_TX_CONFIG0_P0_0,
		              ~PPC_BITMASK(48, 52),
		              PPC_SHIFT(strength, DDRPHY_DP16_IO_TX_CONFIG0_STRENGTH));

		/* IOM0.DDRPHY_DP16_DLL_CONFIG1_P0_{0,1,2,3,4} =
		  [48-63] = 0x0006:
			  [48-51] HS_DLLMUX_SEL_0_0_3 = 0
			  [53-56] HS_DLLMUX_SEL_1_0_3 = 0
			  [61]    S0INSDLYTAP =         1 // For proper functional operation, this bit must be 0b
			  [62]    S1INSDLYTAP =         1 // For proper functional operation, this bit must be 0b
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DLL_CONFIG1_P0_0,
		              ~(PPC_BITMASK(48, 63)),
		              PPC_BIT(S0INSDLYTAP) | PPC_BIT(S1INSDLYTAP));

		/* IOM0.DDRPHY_DP16_IO_TX_FET_SLICE_P0_{0,1,2,3,4} =
		  [48-63] = 0x7f7f:
			  [59-55] EN_SLICE_N_WR = 0x7f
			  [57-63] EN_SLICE_P_WR = 0x7f
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_IO_TX_FET_SLICE_P0_0,
		              ~PPC_BITMASK(48, 63),
		              PPC_SHIFT(0x7F, EN_SLICE_N_WR) |
		              PPC_SHIFT(0x7F, EN_SLICE_P_WR));
	}

	for (dp = 0; dp < 4; dp++) {
		/* IOM0.DDRPHY_ADR_BIT_ENABLE_P0_ADR{0,1,2,3} =
		  [48-63] = 0xffff
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_ADR_BIT_ENABLE_P0_ADR0,
		              ~PPC_BITMASK(48, 63), PPC_SHIFT(0xFFFF, 63));
	}

	/* IOM0.DDRPHY_ADR_DIFFPAIR_ENABLE_P0_ADR1 =
	  [48-63] = 0x5000:
		  [49] DI_ADR2_ADR3: 1 = Lanes 2 and 3 are a differential clock pair
		  [51] DI_ADR6_ADR7: 1 = Lanes 6 and 7 are a differential clock pair
	*/
	dp_mca_and_or(id, dp, mca_i, DDRPHY_ADR_DIFFPAIR_ENABLE_P0_ADR1,
	              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x5000, 63));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR1 =
	  [48-63] = 0x4040:
		  [49-55] ADR_DELAY2 = 0x40
		  [57-63] ADR_DELAY3 = 0x40
	*/
	dp_mca_and_or(id, dp, mca_i, DDRPHY_ADR_DELAY1_P0_ADR1,
	              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x4040, 63));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR1 =
	  [48-63] = 0x4040:
		  [49-55] ADR_DELAY6 = 0x40
		  [57-63] ADR_DELAY7 = 0x40
	*/
	dp_mca_and_or(id, dp, mca_i, DDRPHY_ADR_DELAY3_P0_ADR1,
	              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x4040, 63));

	for (dp = 0; dp < 2; dp ++) {
		/* IOM0.DDRPHY_ADR_DLL_VREG_CONFIG_1_P0_ADR32S{0,1} =
		  [48-63] = 0x0008:
			  [48-51] HS_DLLMUX_SEL_0_3 = 0
			  [59-62] STRENGTH =          4 // 2400 MT/s
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_ADR_DLL_VREG_CONFIG_1_P0_ADR32S0,
		              ~PPC_BITMASK(48, 63),
		              PPC_SHIFT(strength, DDRPHY_ADR_DLL_VREG_CONFIG_1_STRENGTH));

		/* IOM0.DDRPHY_ADR_MCCLK_WRCLK_PR_STATIC_OFFSET_P0_ADR32S{0,1} =
		  [48-63] = 0x6000
		      // For zero delay simulations, or simulations where the delay of the
		      // SysClk tree and the WrClk tree are equal, set this field to 60h
		      [49-55] TSYS_WRCLK = 0x60
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_ADR_MCCLK_WRCLK_PR_STATIC_OFFSET_P0_ADR32S0,
		              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x60, TSYS_WRCLK));

		/* IOM0.DDRPHY_ADR_DLL_VREG_CONTROL_P0_ADR32S{0,1} =
		  [48-50] RXREG_VREG_COMPCON_DC =         3
		  [52-59] = 0x74:
			  [53-55] RXREG_VREG_DRVCON_DC =  0x7
			  [56-58] RXREG_VREG_REF_SEL_DC = 0x2
		  [63] DLL_CAL_CKTS_ACTIVE =  0   // After VREG calibration, some analog circuits are powered down
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_ADR_DLL_VREG_CONTROL_P0_ADR32S0,
		              ~PPC_BITMASK(48, 63),
		              PPC_SHIFT(3, RXREG_VREG_COMPCON_DC) |
		              PPC_SHIFT(7, RXREG_VREG_DRVCON_DC) |
		              PPC_SHIFT(2, RXREG_VREG_REF_SEL_DC));
	}

	/* IOM0.DDRPHY_PC_CONFIG0_P0 =
	  [48-63] = 0x0202:
		  [48-51] PDA_ENABLE_OVERRIDE =     0
		  [52]    2TCK_PREAMBLE_ENABLE =    0
		  [53]    PBA_ENABLE =              0
		  [54]    DDR4_CMD_SIG_REDUCTION =  1
		  [55]    SYSCLK_2X_MEMINTCLKO =    0
		  [56]    RANK_OVERRIDE =           0
		  [57-59] RANK_OVERRIDE_VALUE =     0
		  [60]    LOW_LATENCY =             0
		  [61]    DDR4_IPW_LOOP_DIS =       0
		  [62]    DDR4_VLEVEL_BANK_GROUP =  1
		  [63]    VPROTH_PSEL_MODE =        0
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_CONFIG0_P0, ~PPC_BITMASK(48, 63),
	           PPC_BIT(DDR4_CMD_SIG_REDUCTION) |
	           PPC_BIT(DDR4_VLEVEL_BANK_GROUP));
}

static void p9n_mcbist_scom(int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG0AQ =
	    [0-47]  WATCFG0AQ_CFG_WAT_EVENT_SEL =  0x400000000000
	*/
	scom_and_or_for_chiplet(id, WATCFG0AQ, ~PPC_BITMASK(0, 47),
	                        PPC_SHIFT(0x400000000000, WATCFG0AQ_CFG_WAT_EVENT_SEL));

	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG0BQ =
	    [0-43]  WATCFG0BQ_CFG_WAT_MSKA =  0x3fbfff
	    [44-60] WATCFG0BQ_CFG_WAT_CNTL =  0x10000
	*/
	scom_and_or_for_chiplet(id, WATCFG0BQ, ~PPC_BITMASK(0, 60),
	                        PPC_SHIFT(0x3fbfff, WATCFG0BQ_CFG_WAT_MSKA) |
	                        PPC_SHIFT(0x10000, WATCFG0BQ_CFG_WAT_CNTL));

	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG0DQ =
	    [0-43]  WATCFG0DQ_CFG_WAT_PATA =  0x80200004000
	*/
	scom_and_or_for_chiplet(id, WATCFG0DQ, ~PPC_BITMASK(0, 43),
	                        PPC_SHIFT(0x80200004000, WATCFG0DQ_CFG_WAT_PATA));

	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG3AQ =
	    [0-47]  WATCFG3AQ_CFG_WAT_EVENT_SEL = 0x800000000000
	*/
	scom_and_or_for_chiplet(id, WATCFG3AQ, ~PPC_BITMASK(0, 47),
	                        PPC_SHIFT(0x800000000000, WATCFG3AQ_CFG_WAT_EVENT_SEL));

	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG3BQ =
	    [0-43]  WATCFG3BQ_CFG_WAT_MSKA =  0xfffffffffff
	    [44-60] WATCFG3BQ_CFG_WAT_CNTL =  0x10400
	*/
	scom_and_or_for_chiplet(id, WATCFG3BQ, ~PPC_BITMASK(0, 60),
	                        PPC_SHIFT(0xfffffffffff, WATCFG3BQ_CFG_WAT_MSKA) |
	                        PPC_SHIFT(0x10400, WATCFG3BQ_CFG_WAT_CNTL));

	/* MC01.MCBIST.MBA_SCOMFIR.MCBCFGQ =
	    [36]    MCBCFGQ_CFG_LOG_COUNTS_IN_TRACE = 0
	*/
	scom_and_for_chiplet(id, MCBCFGQ, ~PPC_BIT(MCBCFGQ_CFG_LOG_COUNTS_IN_TRACE));

	/* MC01.MCBIST.MBA_SCOMFIR.DBGCFG0Q =
	    [0]     DBGCFG0Q_CFG_DBG_ENABLE =         1
	    [23-33] DBGCFG0Q_CFG_DBG_PICK_MCBIST01 =  0x780
	*/
	scom_and_or_for_chiplet(id, DBGCFG0Q, ~PPC_BITMASK(23, 33),
	                        PPC_BIT(DBGCFG0Q_CFG_DBG_ENABLE) |
	                        PPC_SHIFT(0x780, DBGCFG0Q_CFG_DBG_PICK_MCBIST01));

	/* MC01.MCBIST.MBA_SCOMFIR.DBGCFG1Q =
	    [0]     DBGCFG1Q_CFG_WAT_ENABLE = 1
	*/
	scom_or_for_chiplet(id, DBGCFG1Q, PPC_BIT(DBGCFG1Q_CFG_WAT_ENABLE));

	/* MC01.MCBIST.MBA_SCOMFIR.DBGCFG2Q =
	    [0-19]  DBGCFG2Q_CFG_WAT_LOC_EVENT0_SEL = 0x10000
	    [20-39] DBGCFG2Q_CFG_WAT_LOC_EVENT1_SEL = 0x08000
	*/
	scom_and_or_for_chiplet(id, DBGCFG2Q, ~PPC_BITMASK(0, 39),
	                        PPC_SHIFT(0x10000, DBGCFG2Q_CFG_WAT_LOC_EVENT0_SEL) |
	                        PPC_SHIFT(0x08000, DBGCFG2Q_CFG_WAT_LOC_EVENT1_SEL));

	/* MC01.MCBIST.MBA_SCOMFIR.DBGCFG3Q =
	    [20-22] DBGCFG3Q_CFG_WAT_GLOB_EVENT0_SEL =      0x4
	    [23-25] DBGCFG3Q_CFG_WAT_GLOB_EVENT1_SEL =      0x4
	    [37-40] DBGCFG3Q_CFG_WAT_ACT_SET_SPATTN_PULSE = 0x4
	*/
	scom_and_or_for_chiplet(id, DBGCFG3Q, ~(PPC_BITMASK(20, 25) | PPC_BITMASK(37, 40)),
	                        PPC_SHIFT(0x4, DBGCFG3Q_CFG_WAT_GLOB_EVENT0_SEL) |
	                        PPC_SHIFT(0x4, DBGCFG3Q_CFG_WAT_GLOB_EVENT1_SEL) |
	                        PPC_SHIFT(0x4, DBGCFG3Q_CFG_WAT_ACT_SET_SPATTN_PULSE));
}

static void set_rank_pairs(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	/*
	 * Assumptions:
	 * - non-LR DIMMs (platform wiki),
	 * - no ATTR_EFF_RANK_GROUP_OVERRIDE,
	 * - mixing rules followed - the same rank configuration for both DIMMs.
	 *
	 * Because rank pairs are defined for each MCA, we can only have up to two
	 * 2R DIMMs. For such configurations, RP0 primary is rank 0 on DIMM 0,
	 * RP1 primary - rank 1 DIMM 0, RP2 primary - rank 0 DIMM 1,
	 * RP3 primary - rank 1 DIMM 1. There are no secondary (this is true for
	 * RDIMM only), tertiary or quaternary rank pairs.
	 */

	static const uint16_t F[] = {0, 0xf000, 0xf0f0, 0xfff0, 0xffff};

	/* TODO: can we mix mirrored and non-mirrored 2R DIMMs in one port? */

	/* IOM0.DDRPHY_PC_RANK_PAIR0_P0 =
	    // rank_countX is the number of master ranks on DIMM X.
	    [48-63] = 0x1537 & F[rank_count0]:      // F = {0, 0xf000, 0xf0f0, 0xfff0, 0xffff}
		[48-50] RANK_PAIR0_PRI = 0
		[51]    RANK_PAIR0_PRI_V = 1: if (rank_count0 >= 1)
		[52-54] RANK_PAIR0_SEC = 2
		[55]    RANK_PAIR0_SEC_V = 1: if (rank_count0 >= 3)
		[56-58] RANK_PAIR1_PRI = 1
		[59]    RANK_PAIR1_PRI_V = 1: if (rank_count0 >= 2)
		[60-62] RANK_PAIR1_SEC = 3
		[63]    RANK_PAIR1_SEC_V = 1: if (rank_count0 == 4)
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_RANK_PAIR0_P0, ~PPC_BITMASK(48, 63),
	           PPC_SHIFT(0x1537 & F[mca->dimm[0].mranks], 63));

	/* IOM0.DDRPHY_PC_RANK_PAIR1_P0 =
	    [48-63] = 0x1537 & F[rank_count1]:     // F = {0, 0xf000, 0xf0f0, 0xfff0, 0xffff}
		[48-50] RANK_PAIR2_PRI = 0
		[51]    RANK_PAIR2_PRI_V = 1: if (rank_count1 >= 1)
		[52-54] RANK_PAIR2_SEC = 2
		[55]    RANK_PAIR2_SEC_V = 1: if (rank_count1 >= 3)
		[56-58] RANK_PAIR3_PRI = 1
		[59]    RANK_PAIR3_PRI_V = 1: if (rank_count1 >= 2)
		[60-62] RANK_PAIR3_SEC = 3
		[63]    RANK_PAIR3_SEC_V = 1: if (rank_count1 == 4)
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_RANK_PAIR1_P0, ~PPC_BITMASK(48, 63),
	           PPC_SHIFT(0x1537 & F[mca->dimm[1].mranks], 63));

	/* IOM0.DDRPHY_PC_RANK_PAIR2_P0 =
	    [48-63] = 0
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_RANK_PAIR2_P0, ~PPC_BITMASK(48, 63), 0);

	/* IOM0.DDRPHY_PC_RANK_PAIR3_P0 =
	    [48-63] = 0
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_RANK_PAIR3_P0, ~PPC_BITMASK(48, 63), 0);

	/* IOM0.DDRPHY_PC_CSID_CFG_P0 =
	    [0-63]  0xf000:
		[48]  CS0_INIT_CAL_VALUE = 1
		[49]  CS1_INIT_CAL_VALUE = 1
		[50]  CS2_INIT_CAL_VALUE = 1
		[51]  CS3_INIT_CAL_VALUE = 1
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_CSID_CFG_P0, ~PPC_BITMASK(48, 63),
	           PPC_SHIFT(0xF000, 63));

	/* IOM0.DDRPHY_PC_MIRROR_CONFIG_P0 =
	    [all] = 0
	    // A rank is mirrored if all are true:
	    //  - the rank is valid (RANK_PAIRn_XXX_V   ==  1)
	    //  - the rank is odd   (RANK_PAIRn_XXX % 2 ==  1)
	    //  - the mirror mode attribute is set for the rank's DIMM (SPD[136])
	    //  - We are not in quad encoded mode (so master ranks <= 2)
	    [48]    ADDR_MIRROR_RP0_PRI
		...
	    [55]    ADDR_MIRROR_RP3_SEC
	    [58]    ADDR_MIRROR_A3_A4 = 1
	    [59]    ADDR_MIRROR_A5_A6 = 1
	    [60]    ADDR_MIRROR_A7_A8 = 1
	    [61]    ADDR_MIRROR_A11_A13 = 1
	    [62]    ADDR_MIRROR_BA0_BA1 = 1
	    [63]    ADDR_MIRROR_BG0_BG1 = 1
	*/
	/*
	 * Assumptions:
	 * - primary and secondary have the same evenness,
	 * - RP1 and RP3 have odd ranks,
	 * - both DIMMs have SPD[136] set or both have it unset, no mixing allowed,
	 * - when rank is not valid, it doesn't matter if it is mirrored,
	 * - no quad encoded mode - no data for it in MT VPD anyway.
	 *
	 * With all of the above, ADDR_MIRROR_RP{1,3}_{PRI,SEC} = SPD[136].
	 */
	uint64_t mirr = mca->dimm[0].present ? mca->dimm[0].spd[136] :
	                                       mca->dimm[1].spd[136];
	mca_and_or(id, mca_i, DDRPHY_PC_MIRROR_CONFIG_P0, ~PPC_BITMASK(48, 63),
	           PPC_SHIFT(mirr, ADDR_MIRROR_RP1_PRI) |
	           PPC_SHIFT(mirr, ADDR_MIRROR_RP1_SEC) |
	           PPC_SHIFT(mirr, ADDR_MIRROR_RP3_PRI) |
	           PPC_SHIFT(mirr, ADDR_MIRROR_RP3_SEC) |
	           PPC_BITMASK(58, 63));

	/* IOM0.DDRPHY_PC_RANK_GROUP_EXT_P0 =  // 0x8000C0350701103F
	    [all] = 0
	    // Same rules as above
	    [48]    ADDR_MIRROR_RP0_TER
		...
	    [55]    ADDR_MIRROR_RP3_QUA
	*/
	/* These are not valid anyway, so don't bother setting anything. */
}

static void reset_data_bit_enable(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 4; dp++) {
		/* IOM0.DDRPHY_DP16_DQ_BIT_ENABLE0_P0_{0,1,2,3} =
		    [all] = 0
		    [48-63] DATA_BIT_ENABLE_0_15 = 0xffff
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DQ_BIT_ENABLE0_P0_0, 0, 0xFFFF);
	}

	/* IOM0.DDRPHY_DP16_DQ_BIT_ENABLE0_P0_4 =
	    [all] = 0
	    [48-63] DATA_BIT_ENABLE_0_15 = 0xff00
	*/
	dp_mca_and_or(id, 4, mca_i, DDRPHY_DP16_DQ_BIT_ENABLE0_P0_0, 0, 0xFF00);

	/* IOM0.DDRPHY_DP16_DFT_PDA_CONTROL_P0_{0,1,2,3,4} =
	    // This reg is named MCA_DDRPHY_DP16_DATA_BIT_ENABLE1_P0_n in the code.
	    // Probably the address changed for DD2 but the documentation did not.
	    [all] = 0
	*/
	for (dp = 0; dp < 5; dp++) {
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DFT_PDA_CONTROL_P0_0, 0, 0);
	}
}

/* 5 DP16, 8 MCA */
/*
 * These tables specify which clock/strobes pins (16-23) of DP16 are used to
 * capture outgoing/incoming data on which data pins (0-16). Those will
 * eventually arrive to DIMM as DQS and DQ, respectively. The mapping must be
 * the same for write and read, but for some reason HW has two separate sets of
 * registers.
 */
/*
 * TODO: after we know how MCAs are numbered we can drop half of x8 table.
 * I'm 90% sure it is 0,1,4,5, but for now I'll leave the rest in comments.
 */
static const uint16_t x4_clk[5] = {0x8640, 0x8640, 0x8640, 0x8640, 0x8400};
static const uint16_t x8_clk[8][5] = {
	{0x0CC0, 0xC0C0, 0x0CC0, 0x0F00, 0x0C00}, /* Port 0 */
	{0xC0C0, 0x0F00, 0x0CC0, 0xC300, 0x0C00}, /* Port 1 */
//	{0xC300, 0xC0C0, 0xC0C0, 0x0F00, 0x0C00}, /* Port 2 */
//	{0x0F00, 0x0F00, 0xC300, 0xC300, 0xC000}, /* Port 3 */
	{0x0CC0, 0xC0C0, 0x0F00, 0x0F00, 0xC000}, /* Port 4 */
	{0xC300, 0x0CC0, 0x0CC0, 0xC300, 0xC000}, /* Port 5 */
//	{0x0CC0, 0x0CC0, 0x0CC0, 0xC0C0, 0x0C00}, /* Port 6 */
//	{0x0CC0, 0xC0C0, 0x0F00, 0xC300, 0xC000}, /* Port 7 */
};

static void reset_clock_enable(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	/* Assume the same rank configuration for both DIMMs */
	int dp;
	int width = mca->dimm[0].present ? mca->dimm[0].width :
	                                   mca->dimm[1].width;
	int mranks[2] = {mca->dimm[0].mranks, mca->dimm[1].mranks};
	 /* Index for x8_clk depends on how MCAs are numbered... */
	const uint16_t *clk = width == WIDTH_x4 ? x4_clk :
	                                          x8_clk[mcs_i * MCA_PER_MCS + mca_i];

	/* IOM0.DDRPHY_DP16_WRCLK_EN_RP0_P0_{0,1,2,3,4}
	    [all] = 0
	    [48-63] QUADn_CLKxx
	*/
	/* IOM0.DDRPHY_DP16_READ_CLOCK_RANK_PAIR0_P0_{0,1,2,3,4}
	    [all] = 0
	    [48-63] QUADn_CLKxx
	*/
	for (dp = 0; dp < 5; dp++) {
		/* Note that these correspond to valid rank pairs */
		if (mranks[0] > 0) {
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WRCLK_EN_RP0_P0_0,
			              0, clk[dp]);
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_READ_CLOCK_RANK_PAIR0_P0_0,
			              0, clk[dp]);
		}

		if (mranks[0] > 1) {
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WRCLK_EN_RP1_P0_0,
			              0, clk[dp]);
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_READ_CLOCK_RANK_PAIR1_P0_0,
			              0, clk[dp]);
		}

		if (mranks[1] > 0) {
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WRCLK_EN_RP2_P0_0,
			              0, clk[dp]);
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_READ_CLOCK_RANK_PAIR2_P0_0,
			              0, clk[dp]);
		}

		if (mranks[1] > 1) {
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WRCLK_EN_RP3_P0_0,
			              0, clk[dp]);
			dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_READ_CLOCK_RANK_PAIR3_P0_0,
			              0, clk[dp]);
		}
	}
}

static void reset_rd_vref(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

	int dp;
	int vpd_idx = mca->dimm[0].present ? (mca->dimm[0].mranks == 2 ? 2 : 0) :
	                                     (mca->dimm[1].mranks == 2 ? 2 : 0);
	if (mca->dimm[0].present && mca->dimm[1].present)
		vpd_idx++;

	/*       RD_VREF_DVDD * (100000 - ATTR_MSS_VPD_MT_VREF_MC_RD) / RD_VREF_DAC_STEP
	vref_bf =     12      * (100000 - ATTR_MSS_VPD_MT_VREF_MC_RD) / 6500
	IOM0.DDRPHY_DP16_RD_VREF_DAC_{0-7}_P0_{0-3},
	IOM0.DDRPHY_DP16_RD_VREF_DAC_{0-3}_P0_4 =     // only half of last DP16 is used
	      [49-55] BIT0_VREF_DAC = vref_bf
	      [57-63] BIT1_VREF_DAC = vref_bf
	*/
	const uint64_t vref_bf = 12 * (100000 - ATTR_MSS_VPD_MT_VREF_MC_RD[vpd_idx]) / 6500;
	for (dp = 0; dp < 5; dp++) {

		/* SCOM addresses are not regular for DAC, so no inner loop. */
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_DAC_0_P0_0,
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, BIT0_VREF_DAC) |
		              PPC_SHIFT(vref_bf, BIT1_VREF_DAC));

		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_DAC_1_P0_0,
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, BIT0_VREF_DAC) |
		              PPC_SHIFT(vref_bf, BIT1_VREF_DAC));

		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_DAC_2_P0_0,
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, BIT0_VREF_DAC) |
		              PPC_SHIFT(vref_bf, BIT1_VREF_DAC));

		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_DAC_3_P0_0,
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, BIT0_VREF_DAC) |
		              PPC_SHIFT(vref_bf, BIT1_VREF_DAC));

		if (dp == 4) break;

		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_DAC_4_P0_0,
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, BIT0_VREF_DAC) |
		              PPC_SHIFT(vref_bf, BIT1_VREF_DAC));

		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_DAC_5_P0_0,
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, BIT0_VREF_DAC) |
		              PPC_SHIFT(vref_bf, BIT1_VREF_DAC));

		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_DAC_6_P0_0,
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, BIT0_VREF_DAC) |
		              PPC_SHIFT(vref_bf, BIT1_VREF_DAC));

		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_DAC_7_P0_0,
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, BIT0_VREF_DAC) |
		              PPC_SHIFT(vref_bf, BIT1_VREF_DAC));
	}

	/* IOM0.DDRPHY_DP16_RD_VREF_CAL_EN_P0_{0-4}
	      [48-63] VREF_CAL_EN = 0xffff          // enable = 0xffff, disable = 0x0000
	*/
	for (dp = 0; dp < 5; dp++) {
		/* Is it safe to set this before VREF_DAC? If yes, may use one loop for both */
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_VREF_CAL_EN_P0_0,
		              0, PPC_BITMASK(48, 63));
	}
}

static void pc_reset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/* These are from VPD */
	/*
	uint64_t ATTR_MSS_EFF_DPHY_WLO = mem_data.speed == 1866 ? 1 : 2;
	uint64_t ATTR_MSS_EFF_DPHY_RLO = mem_data.speed == 1866 ? 4 :
	                                 mem_data.speed == 2133 ? 5 :
	                                 mem_data.speed == 2400 ? 6 : 7;
	*/

	/* IOM0.DDRPHY_PC_CONFIG0_P0 has been reset in p9n_ddrphy_scom() */

	/* IOM0.DDRPHY_PC_CONFIG1_P0 =
	      [48-51] WRITE_LATENCY_OFFSET =  ATTR_MSS_EFF_DPHY_WLO
	      [52-55] READ_LATENCY_OFFSET =   ATTR_MSS_EFF_DPHY_RLO
			+1: if 2N mode (ATTR_MSS_VPD_MR_MC_2N_MODE_AUTOSET, ATTR_MSS_MRW_DRAM_2N_MODE)  // Gear-down mode in JEDEC
	      // Assume no LRDIMM
	      [59-61] MEMORY_TYPE =           0x5     // 0x7 for LRDIMM
	      [62]    DDR4_LATENCY_SW =       1
	*/
	/*
	 * FIXME: I have no idea where Hostboot gets these values from, they should
	 * be the same as in VPD, yet WLO is 3 and RLO is 5 when written to SCOM...
	 *
	 * These are from VPD:
	 * uint64_t ATTR_MSS_EFF_DPHY_WLO = mem_data.speed == 1866 ? 1 : 2;
	 * uint64_t ATTR_MSS_EFF_DPHY_RLO = mem_data.speed == 1866 ? 4 :
	 *                                  mem_data.speed == 2133 ? 5 :
	 *                                  mem_data.speed == 2400 ? 6 : 7;
	 */
	mca_and_or(id, mca_i, DDRPHY_PC_CONFIG1_P0,
	           ~(PPC_BITMASK(48, 55) | PPC_BITMASK(59, 62)),
	           PPC_SHIFT(/* ATTR_MSS_EFF_DPHY_WLO */ 3, WRITE_LATENCY_OFFSET) |
	           PPC_SHIFT(/* ATTR_MSS_EFF_DPHY_RLO */ 5, READ_LATENCY_OFFSET) |
	           PPC_SHIFT(0x5, MEMORY_TYPE) | PPC_BIT(DDR4_LATENCY_SW));

	/* IOM0.DDRPHY_PC_ERROR_STATUS0_P0 =
	      [all]   0
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_ERROR_STATUS0_P0, 0, 0);

	/* IOM0.DDRPHY_PC_INIT_CAL_ERROR_P0 =
	      [all]   0
	*/
	mca_and_or(id, mca_i, DDRPHY_PC_INIT_CAL_ERROR_P0, 0, 0);
}

static void wc_reset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

	/* IOM0.DDRPHY_WC_CONFIG0_P0 =
	      [all]   0
	      // BUG? Mismatch between comment (-,-), code (+,+) and docs (-,+) for operations inside 'max'
	      [48-55] TWLO_TWLOE =        12 + max((twldqsen - tmod), (twlo + twlow))
				   + longest DQS delay in clocks (rounded up) + longest DQ delay in clocks (rounded up)
	      [56]    WL_ONE_DQS_PULSE =  1
	      [57-62] FW_WR_RD =          0x20      // "# dd0 = 17 clocks, now 32 from SWyatt"
	      [63]    CUSTOM_INIT_WRITE = 1         // set to a 1 to get proper values for RD VREF
	*/
	/*
	 * tMOD = max(24 nCK, 15 ns) = 24 nCK for all supported speed bins
	 * tWLDQSEN >= 25 nCK
	 * tWLDQSEN > tMOD + ODTLon + tADC
	 *       0.3 tCK <= tADC <= 0.7 tCK, round to 1
	 *       ODTLon = WL - 2 = CWL + AL + PL - 2; AL = 0, PL = 0
	 * tWLDQSEN = max(25, tMOD + CWL - 2 + 1) = CWL + 23
	 * tWLO = 0 - 9.5 ns, Hostboot uses ATTR_MSS_EFF_DPHY_WLO
	 * tWLOE = 0 - 2 ns, Hostboot uses 2 ns
	 * Longest DQ and DQS delays are both equal 1 nCK.
	 */
	/*
	 * FIXME: again, tWLO = 3 in Hostboot. Why?
	 * This is still much smaller than tWLDQSEN so leave it, for now.
	 */
	uint64_t tWLO = mem_data.speed == 1866 ? 1 : 2;
	uint64_t tWLOE = ns_to_nck(2);
	uint64_t tWLDQSEN = MAX(25, tMOD + (mem_data.cwl - 2) + 1);
	/*
	 * Use the version from the code, it may be longer than necessary but it
	 * works. Note that MAX() always expands to CWL + 23 + 24 = 47 + CWL, which
	 * means that we can just write 'tWLO_tWLOE = 61 + CWL'. Leaving full
	 * version below, it will be easier to fix.
	 */
	/*
	 * FIXME: relative to Hostboot, we are 2 nCK short for tWLDQSEN (37 vs 39).
	 * It doesn't have '- 2' in its calculations (timing.H). However, this is
	 * JEDEC way of doing it so it _should_ work.
	 */
	uint64_t tWLO_tWLOE = 12 + MAX((tWLDQSEN + tMOD), (tWLO + tWLOE)) + 1 + 1;
	mca_and_or(id, mca_i, DDRPHY_WC_CONFIG0_P0, 0,
	           PPC_SHIFT(tWLO_tWLOE, TWLO_TWLOE) | PPC_BIT(WL_ONE_DQS_PULSE) |
	           PPC_SHIFT(0x20, FW_WR_RD) | PPC_BIT(CUSTOM_INIT_WRITE));

	/* IOM0.DDRPHY_WC_CONFIG1_P0 =
	      [all]   0
	      [48-51] BIG_STEP =          7
	      [52-54] SMALL_STEP =        0
	      [55-60] WR_PRE_DLY =        0x2a (42)
	*/
	mca_and_or(id, mca_i, DDRPHY_WC_CONFIG1_P0, 0,
	           PPC_SHIFT(7, BIG_STEP) | PPC_SHIFT(0x2A, WR_PRE_DLY));

	/* IOM0.DDRPHY_WC_CONFIG2_P0 =
	      [all]   0
	      [48-51] NUM_VALID_SAMPLES = 5
	      [52-57] FW_RD_WR =          max(tWTR_S + 11, AL + tRTP + 3)
	      [58-61] IPW_WR_WR =         5     // results in 24 clock cycles
	*/
	/* There is no Additive Latency. */
	mca_and_or(id, mca_i, DDRPHY_WC_CONFIG2_P0, 0,
	           PPC_SHIFT(5, NUM_VALID_SAMPLES) |
	           PPC_SHIFT(MAX(mca->nwtr_s + 11, mem_data.nrtp + 3), FW_RD_WR) |
	           PPC_SHIFT(5, IPW_WR_WR));

	/* IOM0.DDRPHY_WC_CONFIG3_P0 =
	      [all]   0
	      [55-60] MRS_CMD_DQ_OFF =    0x3f
	*/
	mca_and_or(id, mca_i, DDRPHY_WC_CONFIG3_P0, 0, PPC_SHIFT(0x3F, MRS_CMD_DQ_OFF));

	/* IOM0.DDRPHY_WC_RTT_WR_SWAP_ENABLE_P0
	      [48]    WL_ENABLE_RTT_SWAP =            0
	      [49]    WR_CTR_ENABLE_RTT_SWAP =        0
	      [50-59] WR_CTR_VREF_COUNTER_RESET_VAL = 150ns in clock cycles  // JESD79-4C Table 67
	*/
	mca_and_or(id, mca_i, DDRPHY_WC_RTT_WR_SWAP_ENABLE_P0, ~PPC_BITMASK(48, 59),
	           PPC_SHIFT(ns_to_nck(150), WR_CTR_VREF_COUNTER_RESET_VAL));
}

static void rc_reset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

	/* IOM0.DDRPHY_RC_CONFIG0_P0
	      [all]   0
	      [48-51] GLOBAL_PHY_OFFSET = 0x5      // ATTR_MSS_VPD_MR_DPHY_GPO
	      [62]    PERFORM_RDCLK_ALIGN = 1
	*/
	mca_and_or(id, mca_i, DDRPHY_RC_CONFIG0_P0, 0,
	           PPC_SHIFT(0x5, GLOBAL_PHY_OFFSET) | PPC_BIT(PERFORM_RDCLK_ALIGN));

	/* IOM0.DDRPHY_RC_CONFIG1_P0
	      [all]   0
	*/
	mca_and_or(id, mca_i, DDRPHY_RC_CONFIG1_P0, 0, 0);

	/* IOM0.DDRPHY_RC_CONFIG2_P0
	      [all]   0
	      [48-52] CONSEC_PASS = 8
	      [57-58] 3                   // not documented, BURST_WINDOW?
	*/
	mca_and_or(id, mca_i, DDRPHY_RC_CONFIG2_P0, 0,
	           PPC_SHIFT(8, CONSEC_PASS) | PPC_SHIFT(3, 58));

	/* IOM0.DDRPHY_RC_CONFIG3_P0
	      [all]   0
	      [51-54] COARSE_CAL_STEP_SIZE = 4  // 5/128
	*/
	mca_and_or(id, mca_i, DDRPHY_RC_CONFIG3_P0, 0,
	           PPC_SHIFT(4, COARSE_CAL_STEP_SIZE));

	/* IOM0.DDRPHY_RC_RDVREF_CONFIG0_P0 =
	      [all]   0
	      [48-63] WAIT_TIME =
			  0xffff          // as slow as possible, or use calculation from vref_guess_time(), or:
			  MSS_FREQ_EQ_1866: 0x0804
			  MSS_FREQ_EQ_2133: 0x092a
			  MSS_FREQ_EQ_2400: 0x0a50
			  MSS_FREQ_EQ_2666: 0x0b74    // use this value for all freqs maybe?
	*/
	uint64_t wait_time = mem_data.speed == 1866 ? 0x0804 :
	                     mem_data.speed == 2133 ? 0x092A :
	                     mem_data.speed == 2400 ? 0x0A50 : 0x0B74;
	mca_and_or(id, mca_i, DDRPHY_RC_RDVREF_CONFIG0_P0, 0, PPC_SHIFT(wait_time, 63));

	/* IOM0.DDRPHY_RC_RDVREF_CONFIG1_P0 =
	      [all]   0
	      [48-55] CMD_PRECEDE_TIME =  (AL + CL + 15)
	      [56-59] MPR_LOCATION =      4     // "From R. King."
	*/
	mca_and_or(id, mca_i, DDRPHY_RC_RDVREF_CONFIG1_P0, 0,
	           PPC_SHIFT(mca->cl + 15, CMD_PRECEDE_TIME) |
	           PPC_SHIFT(4, MPR_LOCATION));
}

static inline int log2_up(uint32_t x)
{
	int lz;
	asm("cntlzd %0, %1" : "=r"(lz) : "r"((x << 1) - 1));
	return 63 - lz;
}

static void seq_reset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int vpd_idx = mca->dimm[0].present ? (mca->dimm[0].mranks == 2 ? 2 : 0) :
	                                     (mca->dimm[1].mranks == 2 ? 2 : 0);
	if (mca->dimm[0].present && mca->dimm[1].present)
		vpd_idx++;

	/* IOM0.DDRPHY_SEQ_CONFIG0_P0 =
	      [all]   0
	      [49]    TWO_CYCLE_ADDR_EN =
			  2N mode:                1
			  else:                   0
	      [54]    DELAYED_PAR = 1
	      [62]    PAR_A17_MASK =
			  16Gb x4 configuration:  0
			  else:                   1
	*/
	uint64_t par_a17_mask = PPC_BIT(PAR_A17_MASK);
	if ((mca->dimm[0].width == WIDTH_x4 && mca->dimm[0].density == DENSITY_16Gb) ||
	    (mca->dimm[1].width == WIDTH_x4 && mca->dimm[1].density == DENSITY_16Gb))
		par_a17_mask = 0;

	mca_and_or(id, mca_i, DDRPHY_SEQ_CONFIG0_P0, 0,
	           PPC_BIT(DELAYED_PAR) | par_a17_mask);

	/* All log2 values in timing registers are rounded up. */
	/* IOM0.DDRPHY_SEQ_MEM_TIMING_PARAM0_P0 =
	      [all]   0
	      [48-51] TMOD_CYCLES = 5           // log2(max(tMRD, tMOD)) = log2(24), JEDEC tables 169 and 170 and section 13.5
	      [52-55] TRCD_CYCLES = log2(tRCD)
	      [56-59] TRP_CYCLES =  log2(tRP)
	      [60-63] TRFC_CYCLES = log2(tRFC)
	*/
	/*
	 * FIXME or FIXHOSTBOOT: due to a bug in Hostboot TRFC_CYCLES is always 0.
	 * A loop searches for a minimum for all MCAs, but minimum that values are
	 * compared to is initially set to 0. This is a clear violation of RFC
	 * timing. It is fixed later in dqs_align_turn_on_refresh() in 13.11, but
	 * that may not have been necessary if it were written here properly.
	 *
	 * https://github.com/open-power/hostboot/blob/master/src/import/chips/p9/procedures/hwp/memory/lib/phy/seq.C#L142
	 */
	mca_and_or(id, mca_i, DDRPHY_SEQ_MEM_TIMING_PARAM0_P0, 0,
	           PPC_SHIFT(5, TMOD_CYCLES) |
	           PPC_SHIFT(log2_up(mca->nrcd), TRCD_CYCLES) |
	           PPC_SHIFT(log2_up(mca->nrp), TRP_CYCLES) |
	           PPC_SHIFT(log2_up(mca->nrfc), TRFC_CYCLES));

	/* IOM0.DDRPHY_SEQ_MEM_TIMING_PARAM1_P0 =
	      [all]   0
	      [48-51] TZQINIT_CYCLES =  10      // log2(1024), JEDEC tables 169 and 170
	      [52-55] TZQCS_CYCLES =    7       // log2(128), JEDEC tables 169 and 170
	      [56-59] TWLDQSEN_CYCLES = 6       // log2(37) rounded up, JEDEC tables 169 and 170
	      [60-63] TWRMRD_CYCLES =   6       // log2(40) rounded up, JEDEC tables 169 and 170
	*/
	mca_and_or(id, mca_i, DDRPHY_SEQ_MEM_TIMING_PARAM1_P0, 0,
	           PPC_SHIFT(10, TZQINIT_CYCLES) | PPC_SHIFT(7, TZQCS_CYCLES) |
	           PPC_SHIFT(6, TWLDQSEN_CYCLES) | PPC_SHIFT(6, TWRMRD_CYCLES));

	/* IOM0.DDRPHY_SEQ_MEM_TIMING_PARAM2_P0 =
	      [all]   0
	      [48-51] TODTLON_OFF_CYCLES =  log2(CWL + AL + PL - 2)
	      [52-63] reserved =            0x777     // "Reset value of SEQ_TIMING2 is lucky 7's"
	*/
	/* AL and PL are disabled (0) */
	mca_and_or(id, mca_i, DDRPHY_SEQ_MEM_TIMING_PARAM2_P0, 0,
	           PPC_SHIFT(log2_up(mem_data.cwl - 2), TODTLON_OFF_CYCLES) |
	           PPC_SHIFT(0x777, 63));

	/* IOM0.DDRPHY_SEQ_RD_WR_DATA0_P0 =
	      [all]   0
	      [48-63] RD_RW_DATA_REG0 = 0xaa00
	*/
	mca_and_or(id, mca_i, DDRPHY_SEQ_RD_WR_DATA0_P0, 0,
	           PPC_SHIFT(0xAA00, RD_RW_DATA_REG0));

	/* IOM0.DDRPHY_SEQ_RD_WR_DATA1_P0 =
	      [all]   0
	      [48-63] RD_RW_DATA_REG1 = 0x00aa
	*/
	mca_and_or(id, mca_i, DDRPHY_SEQ_RD_WR_DATA1_P0, 0,
	           PPC_SHIFT(0x00AA, RD_RW_DATA_REG1));

	/*
	 * For all registers below, assume RDIMM (max 2 ranks).
	 *
	 * Remember that VPD data layout is different, code will be slightly
	 * different than the comments.
	 */
#define F(x)	(((x >> 4) & 0xC) | ((x >> 2) & 0x3))

	/* IOM0.DDRPHY_SEQ_ODT_RD_CONFIG0_P0 =
	      F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
	      [all]   0
	      [48-51] ODT_RD_VALUES0 = F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][0])
	      [56-59] ODT_RD_VALUES1 = F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][1])
	*/
	mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_RD_CONFIG0_P0, 0,
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][0][0]), ODT_RD_VALUES0) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][0][1]), ODT_RD_VALUES1));

	/* IOM0.DDRPHY_SEQ_ODT_RD_CONFIG1_P0 =
	      F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
	      [all]   0
	      [48-51] ODT_RD_VALUES2 =
			count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][1][0])
			count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][2])
	      [56-59] ODT_RD_VALUES3 =
			count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][1][1])
			count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][3])
	*/
	/* 2 DIMMs -> odd vpd_idx */
	uint64_t val = 0;
	if (vpd_idx % 2)
		val = PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][0]), ODT_RD_VALUES2) |
		      PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][1]), ODT_RD_VALUES3);

	mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_RD_CONFIG1_P0, 0, val);


	/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG0_P0 =
	      F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
	      [all]   0
	      [48-51] ODT_WR_VALUES0 = F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][0])
	      [56-59] ODT_WR_VALUES1 = F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][1])
	*/
	mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_WR_CONFIG0_P0, 0,
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][0]), ODT_WR_VALUES0) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][1]), ODT_WR_VALUES1));

	/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG1_P0 =
	      F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
	      [all]   0
	      [48-51] ODT_WR_VALUES2 =
			count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][1][0])
			count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][2])
	      [56-59] ODT_WR_VALUES3 =
			count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][1][1])
			count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][3])
	*/
	val = 0;
	if (vpd_idx % 2)
		val = PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][1][0]), ODT_WR_VALUES2) |
		      PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][1][1]), ODT_WR_VALUES3);

	mca_and_or(id, mca_i, DDRPHY_SEQ_ODT_WR_CONFIG1_P0, 0, val);
#undef F
}

static void reset_ac_boost_cntl(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	/* IOM0.DDRPHY_DP16_ACBOOST_CTL_BYTE{0,1}_P0_{0,1,2,3,4} =
	    // For all of the AC Boost attributes, they're laid out in the uint32_t as such:
	    // Bit 0-2   = DP16 Block 0 (DQ Bits 0-7)       BYTE0_P0_0
	    // Bit 3-5   = DP16 Block 0 (DQ Bits 8-15)      BYTE1_P0_0
	    // Bit 6-8   = DP16 Block 1 (DQ Bits 0-7)       BYTE0_P0_1
	    // Bit 9-11  = DP16 Block 1 (DQ Bits 8-15)      BYTE1_P0_1
	    // Bit 12-14 = DP16 Block 2 (DQ Bits 0-7)       BYTE0_P0_2
	    // Bit 15-17 = DP16 Block 2 (DQ Bits 8-15)      BYTE1_P0_2
	    // Bit 18-20 = DP16 Block 3 (DQ Bits 0-7)       BYTE0_P0_3
	    // Bit 21-23 = DP16 Block 3 (DQ Bits 8-15)      BYTE1_P0_3
	    // Bit 24-26 = DP16 Block 4 (DQ Bits 0-7)       BYTE0_P0_4
	    // Bit 27-29 = DP16 Block 4 (DQ Bits 8-15)      BYTE1_P0_4
	    [all]   0?    // function does read prev values from SCOM but then overwrites all non-const-0 fields. Why bother?
	    [48-50] S{0,1}ACENSLICENDRV_DC =  appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_DOWN
	    [51-53] S{0,1}ACENSLICEPDRV_DC =  appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_UP
	    [54-56] S{0,1}ACENSLICEPTERM_DC = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP
	*/
	/*
	 * Both ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_* have a value of 0x24924924
	 * for all rank configurations (two copies for two MCA indices to be exact),
	 * meaning that all 3b fields are 0b001. ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP
	 * equals 0. Last DP16 doesn't require special handling, all DQ bits are
	 * configured.
	 *
	 * Write these fields explicitly instead of shifting and masking for better
	 * readability.
	 */
	for (dp = 0; dp < 5; dp++) {
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_ACBOOST_CTL_BYTE0_P0_0,
		              ~PPC_BITMASK(48, 56),
		              PPC_SHIFT(1, S0ACENSLICENDRV_DC) |
		              PPC_SHIFT(1, S0ACENSLICEPDRV_DC));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_ACBOOST_CTL_BYTE1_P0_0,
		              ~PPC_BITMASK(48, 56),
		              PPC_SHIFT(1, S1ACENSLICENDRV_DC) |
		              PPC_SHIFT(1, S1ACENSLICEPDRV_DC));
	}
}

static void reset_ctle_cntl(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	/* IOM0.DDRPHY_DP16_CTLE_CTL_BYTE{0,1}_P0_{0,1,2,3,4} =
	    // For the capacitance CTLE attributes, they're laid out in the uint64_t as such. The resitance
	    // attributes are the same, but 3 bits long. Notice that DP Block X Nibble 0 is DQ0:3,
	    // Nibble 1 is DQ4:7, Nibble 2 is DQ8:11 and 3 is DQ12:15.
	    // Bit 0-1   = DP16 Block 0 Nibble 0     Bit 16-17 = DP16 Block 2 Nibble 0     Bit 32-33 = DP16 Block 4 Nibble 0
	    // Bit 2-3   = DP16 Block 0 Nibble 1     Bit 18-19 = DP16 Block 2 Nibble 1     Bit 34-35 = DP16 Block 4 Nibble 1
	    // Bit 4-5   = DP16 Block 0 Nibble 2     Bit 20-21 = DP16 Block 2 Nibble 2     Bit 36-37 = DP16 Block 4 Nibble 2
	    // Bit 6-7   = DP16 Block 0 Nibble 3     Bit 22-23 = DP16 Block 2 Nibble 3     Bit 38-39 = DP16 Block 4 Nibble 3
	    // Bit 8-9   = DP16 Block 1 Nibble 0     Bit 24-25 = DP16 Block 3 Nibble 0
	    // Bit 10-11 = DP16 Block 1 Nibble 1     Bit 26-27 = DP16 Block 3 Nibble 1
	    // Bit 12-13 = DP16 Block 1 Nibble 2     Bit 28-29 = DP16 Block 3 Nibble 2
	    // Bit 14-15 = DP16 Block 1 Nibble 3     Bit 30-31 = DP16 Block 3 Nibble 3
	    [48-49] NIB_{0,2}_DQSEL_CAP = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP
	    [53-55] NIB_{0,2}_DQSEL_RES = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES
	    [56-57] NIB_{1,3}_DQSEL_CAP = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP
	    [61-63] NIB_{1,3}_DQSEL_RES = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES
	*/
	/*
	 * For all rank configurations and both MCAs, ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP
	 * is 0x5555555555000000 (so every 2b field is 0b01) and *_RES equals
	 * 0xb6db6db6db6db6d0 (every 3b field is 0b101 = 5).
	 */
	for (dp = 0; dp < 5; dp++) {
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_CTLE_CTL_BYTE0_P0_0,
		              ~(PPC_BITMASK(48, 49) | PPC_BITMASK(53, 57) | PPC_BITMASK(61, 63)),
		              PPC_SHIFT(1, NIB_0_DQSEL_CAP) | PPC_SHIFT(5, NIB_0_DQSEL_RES) |
		              PPC_SHIFT(1, NIB_1_DQSEL_CAP) | PPC_SHIFT(5, NIB_1_DQSEL_RES));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_CTLE_CTL_BYTE1_P0_0,
		              ~(PPC_BITMASK(48, 49) | PPC_BITMASK(53, 57) | PPC_BITMASK(61, 63)),
		              PPC_SHIFT(1, NIB_2_DQSEL_CAP) | PPC_SHIFT(5, NIB_2_DQSEL_RES) |
		              PPC_SHIFT(1, NIB_3_DQSEL_CAP) | PPC_SHIFT(5, NIB_3_DQSEL_RES));
	}
}

static void reset_delay(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

	/* See comments in ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0 for layout */
	int speed_idx = mem_data.speed == 1866 ? 0 :
	                mem_data.speed == 2133 ? 8 :
	                mem_data.speed == 2400 ? 16 : 24;
	int dimm_idx = (mca->dimm[0].present && mca->dimm[1].present) ? 4 : 0;
	/* TODO: second CPU not supported */
	int vpd_idx = speed_idx + dimm_idx + mcs_i;

	/*
	 * From documentation:
	 * "If the reset value is not sufficient for the given system, these
	 * registers must be set via the programming interface."
	 *
	 * Unsure if this is the case. Hostboot sets it, so lets do it too.
	 */
	/* IOM0.DDRPHY_ADR_DELAY0_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY0 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0
	    [57-63] ADR_DELAY1 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_WEN_A14
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY0_P0_ADR0, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_WEN_A14[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY2 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT1
	    [57-63] ADR_DELAY3 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C0
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY1_P0_ADR0, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT1[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C0[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY2_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY4 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA1
	    [57-63] ADR_DELAY5 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A10
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY2_P0_ADR0, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA1[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A10[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY6 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT1
	    [57-63] ADR_DELAY7 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA0
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY3_P0_ADR0, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT1[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA0[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY4_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY8 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A00
	    [57-63] ADR_DELAY9 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT0
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY4_P0_ADR0, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A00[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT0[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY5_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY10 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT0
	    [57-63] ADR_DELAY11 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_CASN_A15
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY5_P0_ADR0, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT0[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_CASN_A15[vpd_idx][mca_i], ADR_DELAY_ODD));


	/* IOM0.DDRPHY_ADR_DELAY0_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY0 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A13
	    [57-63] ADR_DELAY1 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN1
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY0_P0_ADR1, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A13[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN1[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY2 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKN
	    [57-63] ADR_DELAY3 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKP
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY1_P0_ADR1, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKN[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKP[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY2_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY4 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A17
	    [57-63] ADR_DELAY5 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C1
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY2_P0_ADR1, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A17[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C1[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY6 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKN
	    [57-63] ADR_DELAY7 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKP
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY3_P0_ADR1, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKN[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKP[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY4_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY8 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C2
	    [57-63] ADR_DELAY9 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN1
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY4_P0_ADR1, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C2[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN1[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY5_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY10 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A02
	    [57-63] ADR_DELAY11 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_PAR
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY5_P0_ADR1, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A02[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_PAR[vpd_idx][mca_i], ADR_DELAY_ODD));


	/* IOM0.DDRPHY_ADR_DELAY0_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY0 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN0
	    [57-63] ADR_DELAY1 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_RASN_A16
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY0_P0_ADR2, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN0[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_RASN_A16[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY2 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A08
	    [57-63] ADR_DELAY3 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A05
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY1_P0_ADR2, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A08[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A05[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY2_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY4 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A03
	    [57-63] ADR_DELAY5 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A01
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY2_P0_ADR2, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A03[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A01[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY6 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A04
	    [57-63] ADR_DELAY7 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A07
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY3_P0_ADR2, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A04[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A07[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY4_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY8 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A09
	    [57-63] ADR_DELAY9 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A06
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY4_P0_ADR2, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A09[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A06[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY5_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY10 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE1
	    [57-63] ADR_DELAY11 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A12
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY5_P0_ADR2, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE1[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A12[vpd_idx][mca_i], ADR_DELAY_ODD));


	/* IOM0.DDRPHY_ADR_DELAY0_P0_ADR3 =
	    [all]   0
	    [49-55] ADR_DELAY0 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ACTN
	    [57-63] ADR_DELAY1 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A11
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY0_P0_ADR3, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ACTN[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A11[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR3 =
	    [all]   0
	    [49-55] ADR_DELAY2 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG0
	    [57-63] ADR_DELAY3 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE0
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY1_P0_ADR3, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG0[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE0[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY2_P0_ADR3 =
	    [all]   0
	    [49-55] ADR_DELAY4 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE1
	    [57-63] ADR_DELAY5 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG1
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY2_P0_ADR3, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE1[vpd_idx][mca_i], ADR_DELAY_EVEN) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG1[vpd_idx][mca_i], ADR_DELAY_ODD));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR3 =
	    [all]   0
	    [49-55] ADR_DELAY6 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE0
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_DELAY3_P0_ADR3, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE0[vpd_idx][mca_i], ADR_DELAY_EVEN));

}

static void reset_tsys_adr(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int i = mem_data.speed == 1866 ? 0 :
	        mem_data.speed == 2133 ? 1 :
	        mem_data.speed == 2400 ? 2 : 3;

	/* IOM0.DDRPHY_ADR_MCCLK_WRCLK_PR_STATIC_OFFSET_P0_ADR32S{0,1} =
	    [all]   0
	    [49-55] TSYS_WRCLK = ATTR_MSS_VPD_MR_TSYS_ADR
		  // From regs spec:
		  // Set to ‘19’h for 2666 MT/s.
		  // Set to ‘17’h for 2400 MT/s.
		  // Set to ‘14’h for 2133 MT/s.
		  // Set to ‘12’h for 1866 MT/s.
	*/
	/* Has the same stride as DP16. */
	dp_mca_and_or(id, 0, mca_i, DDRPHY_ADR_MCCLK_WRCLK_PR_STATIC_OFFSET_P0_ADR32S0,
	              0, PPC_SHIFT(ATTR_MSS_VPD_MR_TSYS_ADR[i], TSYS_WRCLK));
	dp_mca_and_or(id, 1, mca_i, DDRPHY_ADR_MCCLK_WRCLK_PR_STATIC_OFFSET_P0_ADR32S0,
	              0, PPC_SHIFT(ATTR_MSS_VPD_MR_TSYS_ADR[i], TSYS_WRCLK));
}

static void reset_tsys_data(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int i = mem_data.speed == 1866 ? 0 :
	        mem_data.speed == 2133 ? 1 :
	        mem_data.speed == 2400 ? 2 : 3;
	int dp;

	/* IOM0.DDRPHY_DP16_WRCLK_PR_P0_{0,1,2,3,4} =
	    [all]   0
	    [49-55] TSYS_WRCLK = ATTR_MSS_VPD_MR_TSYS_DATA
		  // From regs spec:
		  // Set to ‘12’h for 2666 MT/s.
		  // Set to ‘10’h for 2400 MT/s.
		  // Set to ‘0F’h for 2133 MT/s.
		  // Set to ‘0D’h for 1866 MT/s.
	*/
	for (dp = 0; dp < 5; dp++) {
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WRCLK_PR_P0_0, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MR_TSYS_DATA[i], TSYS_WRCLK));
	}
}

static void reset_io_impedances(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_IO_TX_FET_SLICE_P0_{0,1,2,3,4} =
		    [all]   0
		    // 0 - Hi-Z, otherwise impedance = 240/<num of set bits> Ohms
		    [49-55] EN_SLICE_N_WR = ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS[{0,1,2,3,4}]
		    [57-63] EN_SLICE_P_WR = ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS[{0,1,2,3,4}]
		*/
		/*
		 * For all rank configurations and MCAs, ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS
		 * is 34 Ohms. 240/34 = 7 bits set. According to documentation this is the
		 * default value, but set it just to be safe.
		 */
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_IO_TX_FET_SLICE_P0_0, 0,
		              PPC_SHIFT(0x7F, EN_SLICE_N_WR) |
		              PPC_SHIFT(0x7F, EN_SLICE_P_WR));

		/* IOM0.DDRPHY_DP16_IO_TX_PFET_TERM_P0_{0,1,2,3,4} =
		    [all]   0
		    // 0 - Hi-Z, otherwise impedance = 240/<num of set bits> Ohms
		    [49-55] EN_SLICE_N_WR = ATTR_MSS_VPD_MT_MC_RCV_IMP_DQ_DQS[{0,1,2,3,4}]
		*/
		/* 60 Ohms for all configurations, 240/60 = 4 bits set. */
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_IO_TX_PFET_TERM_P0_0, 0,
		              PPC_SHIFT(0x0F, EN_SLICE_N_WR));
	}

	/* IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR1 =    // yes, ADR1
	    // These are RMW one at a time. I don't see why not all at once, or at least in pairs (P and N of the same clocks)
	    if (ATTR_MSS_VPD_MT_MC_DRV_IMP_CLK == ENUM_ATTR_MSS_VPD_MT_MC_DRV_IMP_CLK_OHM30):
	      [54,52,62,60] SLICE_SELn = 1    // CLK00 P, CLK00 N, CLK01 P, CLK01 N
	    else
	      [54,52,62,60] = 0
	*/
	/* 30 Ohms for all configurations. */
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR1, ~0,
	           PPC_BIT(SLICE_SEL2) | PPC_BIT(SLICE_SEL3) |
	           PPC_BIT(SLICE_SEL6) | PPC_BIT(SLICE_SEL7));

	/*
	 * Following are reordered to minimalize number of register reads/writes
	------------------------------------------------------------------------
	val = (ATTR_MSS_VPD_MT_MC_DRV_IMP_CMD_ADDR == ENUM_ATTR_MSS_VPD_MT_MC_DRV_IMP_CMD_ADDR_OHM30) ? 1 : 0
	// val = 30 for all VPD configurations
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR0 =
	    [50,56,58,62] =           val       // ADDR14/WEN, BA1, ADDR10, BA0
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR0 =
	    [48,54] =                 val       // ADDR0, ADDR15/CAS
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR1 =        // same as CLK, however it uses different VPD
	    [48,56] =                 val       // ADDR13, ADDR17/RAS
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR1 =
	    [52]    =                 val       // ADDR2
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR2 =
	    [50,52,54,56,58,60,62] =  val       // ADDR16/RAS, ADDR8, ADDR5, ADDR3, ADDR1, ADDR4, ADDR7
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR2 =
	    [48,50,54] =              val       // ADDR9, ADDR6, ADDR12
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR3 =
	    [48,50,52,58] =           val       // ACT_N, ADDR11, BG0, BG1
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR0, ~0,
	           PPC_BIT(SLICE_SEL1) | PPC_BIT(SLICE_SEL4) | PPC_BIT(SLICE_SEL5) |
	           PPC_BIT(SLICE_SEL7));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR0, ~0,
	           PPC_BIT(SLICE_SEL0) | PPC_BIT(SLICE_SEL3));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR1, ~0,
	           PPC_BIT(SLICE_SEL0) | PPC_BIT(SLICE_SEL4));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR1, ~0,
	           PPC_BIT(SLICE_SEL2));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR2, ~0,
	           PPC_BIT(SLICE_SEL1) | PPC_BIT(SLICE_SEL2) | PPC_BIT(SLICE_SEL3) |
	           PPC_BIT(SLICE_SEL4) | PPC_BIT(SLICE_SEL5) | PPC_BIT(SLICE_SEL6) |
	           PPC_BIT(SLICE_SEL7));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR2, ~0,
	           PPC_BIT(SLICE_SEL0) | PPC_BIT(SLICE_SEL1) | PPC_BIT(SLICE_SEL3));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR3, ~0,
	           PPC_BIT(SLICE_SEL0) | PPC_BIT(SLICE_SEL1) | PPC_BIT(SLICE_SEL2) |
	           PPC_BIT(SLICE_SEL5));

	/*
	 * Following are reordered to minimalize number of register reads/writes
	------------------------------------------------------------------------
	val = (ATTR_MSS_VPD_MT_MC_DRV_IMP_CNTL == ENUM_ATTR_MSS_VPD_MT_MC_DRV_IMP_CNTL_OHM30) ? 1 : 0
	// val = 30 for all VPD sets
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR0 =        // same as CMD/ADDR, however it uses different VPD
	    [52,60] =                 val       // ODT3, ODT1
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR0 =        // same as CMD/ADDR, however it uses different VPD
	    [50,52] =                 val       // ODT2, ODT0
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR1 =        // same as CMD/ADDR, however it uses different VPD
	    [54] =                    val       // PARITY
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR2 =        // same as CMD/ADDR, however it uses different VPD
	    [52] =                    val       // CKE1
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR3 =        // same as CMD/ADDR, however it uses different VPD
	    [54,56,60,62] =           val       // CKE0, CKE3, CKE2, RESET_N
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR0, ~0,
	           PPC_BIT(SLICE_SEL2) | PPC_BIT(SLICE_SEL6));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR0, ~0,
	           PPC_BIT(SLICE_SEL1) | PPC_BIT(SLICE_SEL2));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR1, ~0,
	           PPC_BIT(SLICE_SEL3));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR2, ~0,
	           PPC_BIT(SLICE_SEL2));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR3, ~0,
	           PPC_BIT(SLICE_SEL3) | PPC_BIT(SLICE_SEL4) | PPC_BIT(SLICE_SEL6) |
	           PPC_BIT(SLICE_SEL7));

	/*
	 * Following are reordered to minimalize number of register reads/writes
	------------------------------------------------------------------------
	val = (ATTR_MSS_VPD_MT_MC_DRV_IMP_CSCID == ENUM_ATTR_MSS_VPD_MT_MC_DRV_IMP_CSCID_OHM30) ? 1 : 0
	// val = 30 for all VPD sets
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR0 =        // same as CMD/ADDR and CNTL, however it uses different VPD
	    [48,54] =                 val       // CS0, CID0
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR1 =        // same as CLK and CMD/ADDR, however it uses different VPD
	    [50,58] =                 val       // CS1, CID1
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR1 =        // same as CMD/ADDR and CNTL, however it uses different VPD
	    [48,50] =                 val       // CS3, CID2
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR2 =        // same as CMD/ADDR, however it uses different VPD
	    [48] =                    val       // CS2
	*/
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR0, ~0,
	           PPC_BIT(SLICE_SEL0) | PPC_BIT(SLICE_SEL3));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR1, ~0,
	           PPC_BIT(SLICE_SEL1) | PPC_BIT(SLICE_SEL5));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR1, ~0,
	           PPC_BIT(SLICE_SEL0) | PPC_BIT(SLICE_SEL1));
	mca_and_or(id, mca_i, DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR2, ~0,
	           PPC_BIT(SLICE_SEL0));

	/*
	 * IO impedance regs summary:            lanes 9-15 have different possible settings (results in 15/30 vs 40/30 Ohm)
	 * MAP0_ADR0: all set                       MAP1_ADR0: lanes 12-15 not set
	 * MAP0_ADR1: all set                       MAP1_ADR1: lanes 12-15 not set
	 * MAP0_ADR2: all set                       MAP1_ADR2: lanes 12-15 not set
	 * MAP0_ADR3: all set                       MAP1_ADR3: not used
	 * This mapping is consistent with ADR_DELAYx_P0_ADRy settings.
	 */
}

static void reset_wr_vref_registers(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int dp;
	int vpd_idx = mca->dimm[0].present ? (mca->dimm[0].mranks == 2 ? 2 : 0) :
	                                     (mca->dimm[1].mranks == 2 ? 2 : 0);
	if (mca->dimm[0].present && mca->dimm[1].present)
		vpd_idx++;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_WR_VREF_CONFIG0_P0_{0,1,2,3,4} =
		    [all]   0
		    [48]    WR_CTR_1D_MODE_SWITCH =       0     // 1 for <DD2
		    [49]    WR_CTR_RUN_FULL_1D =          1
		    [50-52] WR_CTR_2D_SMALL_STEP_VAL =    0     // implicit +1
		    [53-56] WR_CTR_2D_BIG_STEP_VAL =      1     // implicit +1
		    [57-59] WR_CTR_NUM_BITS_TO_SKIP =     0     // skip nothing
		    [60-62] WR_CTR_NUM_NO_INC_VREF_COMP = 7
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_CONFIG0_P0_0, 0,
		              PPC_BIT(WR_CTR_RUN_FULL_1D) |
		              PPC_SHIFT(1, WR_CTR_2D_BIG_STEP_VAL) |
		              PPC_SHIFT(7, WR_CTR_NUM_NO_INC_VREF_COMP));

		/* IOM0.DDRPHY_DP16_WR_VREF_CONFIG1_P0_{0,1,2,3,4} =
		    [all]   0
		    [48]    WR_CTR_VREF_RANGE_SELECT =      0       // range 1 by default (60-92.5%)
		    [49-55] WR_CTR_VREF_RANGE_CROSSOVER =   0x18    // JEDEC table 34
		    [56-62] WR_CTR_VREF_SINGLE_RANGE_MAX =  0x32    // JEDEC table 34
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_CONFIG1_P0_0, 0,
		              PPC_SHIFT(0x18, WR_CTR_VREF_RANGE_CROSSOVER) |
		              PPC_SHIFT(0x32, WR_CTR_VREF_SINGLE_RANGE_MAX));

		/* IOM0.DDRPHY_DP16_WR_VREF_STATUS0_P0_{0,1,2,3,4} =
		    [all]   0
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_STATUS0_P0_0, 0, 0);

		/* IOM0.DDRPHY_DP16_WR_VREF_STATUS1_P0_{0,1,2,3,4} =
		    [all]   0
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_STATUS1_P0_0, 0, 0);

		/* IOM0.DDRPHY_DP16_WR_VREF_ERROR_MASK{0,1}_P0_{0,1,2,3,4} =
		    [all]   0
		    [48-63] 0xffff
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR_MASK0_P0_0, 0,
		              PPC_BITMASK(48, 63));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR_MASK1_P0_0, 0,
		              PPC_BITMASK(48, 63));

		/* IOM0.DDRPHY_DP16_WR_VREF_ERROR{0,1}_P0_{0,1,2,3,4} =
		    [all]   0
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR0_P0_0, 0, 0);
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_ERROR1_P0_0, 0, 0);

		/* Assume RDIMM
		IOM0.DDRPHY_DP16_WR_VREF_VALUE{0,1}_RANK_PAIR0_P0_{0,1,2,3,4} =
		IOM0.DDRPHY_DP16_WR_VREF_VALUE{0,1}_RANK_PAIR1_P0_{0,1,2,3,4} =
		IOM0.DDRPHY_DP16_WR_VREF_VALUE{0,1}_RANK_PAIR2_P0_{0,1,2,3,4} =
		IOM0.DDRPHY_DP16_WR_VREF_VALUE{0,1}_RANK_PAIR3_P0_{0,1,2,3,4} =
		    [all]   0
		    [49]    WR_VREF_RANGE_DRAM{0,2} = ATTR_MSS_VPD_MT_VREF_DRAM_WR & 0x40
		    [50-55] WR_VREF_VALUE_DRAM{0,2} = ATTR_MSS_VPD_MT_VREF_DRAM_WR & 0x3f
		    [57]    WR_VREF_RANGE_DRAM{1,3} = ATTR_MSS_VPD_MT_VREF_DRAM_WR & 0x40
		    [58-63] WR_VREF_VALUE_DRAM{1,3} = ATTR_MSS_VPD_MT_VREF_DRAM_WR & 0x3f
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_VALUE0_RANK_PAIR0_P0_0, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM0) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM1));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_VALUE1_RANK_PAIR0_P0_0, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM2) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM3));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_VALUE0_RANK_PAIR1_P0_0, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM0) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM1));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_VALUE1_RANK_PAIR1_P0_0, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM2) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM3));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_VALUE0_RANK_PAIR2_P0_0, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM0) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM1));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_VALUE1_RANK_PAIR2_P0_0, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM2) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM3));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_VALUE0_RANK_PAIR3_P0_0, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM0) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM1));
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_WR_VREF_VALUE1_RANK_PAIR3_P0_0, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM2) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], WR_VREF_VALUE_DRAM3));
	}
}

static void reset_drift_limits(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_DRIFT_LIMITS_P0_{0,1,2,3,4} =
		    [48-49] DD2_BLUE_EXTEND_RANGE = 1         // always ONE_TO_FOUR due to red waterfall workaround
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DRIFT_LIMITS_P0_0,
		              ~PPC_BITMASK(48, 49),
		              PPC_SHIFT(1, DD2_BLUE_EXTEND_RANGE));
	}
}

static void rd_dia_config5(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_RD_DIA_CONFIG5_P0_{0,1,2,3,4} =
		    // "this isn't an EC feature workaround, it's a incorrect documentation workaround"
		    [all]   0
		    [49]    DYN_MCTERM_CNTL_EN =      1
		    [52]    PER_CAL_UPDATE_DISABLE =  1     // "This bit must be set to 0 for normal operation"
		    [59]    PERCAL_PWR_DIS =          1
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_RD_DIA_CONFIG5_P0_0, 0,
		              PPC_BIT(DYN_MCTERM_CNTL_EN) |
		              PPC_BIT(PER_CAL_UPDATE_DISABLE) |
		              PPC_BIT(PERCAL_PWR_DIS));
	}
}

static void dqsclk_offset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_DQSCLK_OFFSET_P0_{0,1,2,3,4} =
		    // "this isn't an EC feature workaround, it's a incorrect documentation workaround"
		    [all]   0
		    [49-55] DQS_OFFSET = 0x08       // Config provided by S. Wyatt 9/13
		*/
		dp_mca_and_or(id, dp, mca_i, DDRPHY_DP16_DQSCLK_OFFSET_P0_0, 0,
		              PPC_SHIFT(0x08, DQS_OFFSET));
	}
}

static void phy_scominit(int mcs_i, int mca_i)
{
	/* Hostboot here sets strength, we did it in p9n_ddrphy_scom(). */
	set_rank_pairs(mcs_i, mca_i);

	reset_data_bit_enable(mcs_i, mca_i);

	/* Assume there are no bad bits (disabled DQ/DQS lines) for now */
	// reset_bad_bits();

	reset_clock_enable(mcs_i, mca_i);
	reset_rd_vref(mcs_i, mca_i);

	pc_reset(mcs_i, mca_i);
	wc_reset(mcs_i, mca_i);
	rc_reset(mcs_i, mca_i);
	seq_reset(mcs_i, mca_i);

	reset_ac_boost_cntl(mcs_i, mca_i);
	reset_ctle_cntl(mcs_i, mca_i);
	reset_delay(mcs_i, mca_i);
	reset_tsys_adr(mcs_i, mca_i);
	reset_tsys_data(mcs_i, mca_i);
	reset_io_impedances(mcs_i, mca_i);
	reset_wr_vref_registers(mcs_i, mca_i);
	reset_drift_limits(mcs_i, mca_i);

	/* Workarounds */

	/* Doesn't apply to DD2 */
	// dqs_polarity();

	rd_dia_config5(mcs_i, mca_i);
	dqsclk_offset(mcs_i, mca_i);

	/* Doesn't apply to DD2 */
	// odt_config();
}

static void fir_unmask(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/* IOM0.IOM_PHY0_DDRPHY_FIR_REG =
	  [56]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2 = 0   // calibration errors
	  [58]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_4 = 0   // DLL errors
	*/
	mca_and_or(id, mca_i, IOM_PHY0_DDRPHY_FIR_REG,
	           ~(PPC_BIT(IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2) |
	             PPC_BIT(IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_4)),
	           0);

	/* MC01.PORT0.SRQ.MBACALFIR =
	  [4]   MBACALFIR_RCD_PARITY_ERROR = 0
	  [8]   MBACALFIR_DDR_MBA_EVENT_N =  0
	*/
	mca_and_or(id, mca_i, MBACALFIR,
	           ~(PPC_BIT(MBACALFIR_RCD_PARITY_ERROR) |
	             PPC_BIT(MBACALFIR_DDR_MBA_EVENT_N)),
	           0);
}

/*
 * 13.8 mss_scominit: Perform scom inits to MC and PHY
 *
 * - HW units included are MCBIST, MCA/PHY (Nimbus) or membuf, L4, MBAs (Cumulus)
 * - Does not use initfiles, coded into HWP
 * - Uses attributes from previous step
 * - Pushes memory extent configuration into the MBA/MCAs
 *   - Addresses are pulled from attributes, set previously by mss_eff_config
 *   - MBA/MCAs always start at address 0, address map controlled by
 *     proc_setup_bars below
 */
void istep_13_8(void)
{
	printk(BIOS_EMERG, "starting istep 13.8\n");
	int mcs_i, mca_i;

	report_istep(13,8);

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		/* No need to initialize a non-functional MCS */
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
			/*
			 * 0th MCA is 'magic' - it has a logic PHY block that is not contained
			 * in other MCA. The magic MCA must be always initialized, even when it
			 * doesn't have any DIMM installed.
			 */
			if (mca_i != 0 && !mca->functional)
				continue;

			/* Some registers cannot be initialized without data from SPD */
			if (mca->functional) {
				/* Assume DIMM mixing rules are followed - same rank config on both DIMMs*/
				p9n_mca_scom(mcs_i, mca_i);
				thermal_throttle_scominit(mcs_i, mca_i);
			}

			/* The rest can and should be initialized also on magic port */
			p9n_ddrphy_scom(mcs_i, mca_i);
		}
		p9n_mcbist_scom(mcs_i);
	}

	/* This double loop is a part of phy_scominit() in Hostboot, but this is simpler. */
	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
			/* No magic for phy_scominit(). */
			if (mca->functional)
				phy_scominit(mcs_i, mca_i);

			/*
			 * TODO: test this with DIMMs on both MCS. Maybe this has to be done
			 * in a separate loop, after phy_scominit()'s are done on both MCSs.
			 */
			if (mca_i == 0 || mca->functional)
				fir_unmask(mcs_i, mca_i);
		}
	}

	printk(BIOS_EMERG, "ending istep 13.8\n");
}
