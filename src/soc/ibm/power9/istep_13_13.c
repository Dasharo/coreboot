/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>

#include "istep_13_scom.h"

/*
 * Set up the MC port <-> DIMM address translation registers.
 *
 * These are not documented in specs, everything described here comes from the
 * code (and comments). Depending on how you count them, there are 2 or 3 base
 * configurations, and the rest is a modification of one of the bases or its
 * derived forms. Each level usually adds one row bit, but sometimes it removes
 * it or modifies rank bits. In most cases when it happens, the rest of bits
 * must be shifted.
 *
 * There are two pairs of identical settings for each master/slave rank
 * configurations: 4Gb x4 is always the same as 8Gb x8, and 8Gb x4 is the same
 * as 16Gb x8.
 *
 * Base configurations are:
 * - 1 rank, non-3DS 4Gb x4 and second DIMM is also 1R (not necessarily 4Gb x4)
 *   - special case when the other DIMM is not an 1R device (because of allowed
 *     DIMM mixing this can only mean tha the other slot is not populated)
 * - 2 rank, non-3DS 4Gb x4
 *
 * The special case uses different column, bank and bank group addressing, the
 * other two cases use identical mapping. This is due to the fact that for one
 * 1R DIMM there is no port address bit with index 7, which is used as C9 in
 * other cases. Hostboot divides those cases as listed above, but it might make
 * more sense to separate special case and use uniform logic for the rest.
 * However, for two 1R DIMMs port address 29 is always assigned to D-bit (more
 * about it later), because bit map fields for rows use only 3 bit for encoding,
 * meaning that only port bits 0-7 can be mapped to row bits 15-17.
 *
 * According to code, port addresses 0-7 and 22-32 can be configured in the
 * register - 19 possibilities total, encoded, so bit field in register is 5
 * bits long, except for row bitmaps, which are only 3 bits long (addresses 0-7
 * only). Column, bank and bank group addressing is always the same (DDR4 always
 * has 10 column, 2 bank, 2 bank group bits), difference is in row bit mapping
 * (we may or may not have bits 15, 16 and 17, those are indexed from 0 so we
 * can have 15-18 row bits in total) and ranks mapping (up to 2 bits for master
 * ranks, up to 3 bits for slave ranks in 3DS devices).
 *
 * TODO: what about 16Gb bank groups? Those should use just 1 bit, but the code
 * doesn't change it.
 *
 * Apart from already mentioned bits there is also D-bit (D is short for DIMM).
 * It is used to tell the controller which DIMM has to be accessed. To avoid
 * holes in the memory map, larger DIMM must be mapped to lower addresses. For
 * example, when we have 4GB and 8GB DIMMs:
 *
 * 0       4       8      12      16 ...      memory space, in GB
 * |   DIMM X      |DIMM Y | hole  | ...             <- this is good
 * |DIMM Y | hole  |   DIMM X      | ...             <- this is bad
 *
 * Whether DIMM X is in DIMM0 or DIMM1 slot doesn't matter. The example is
 * simplified - the addresses do not translate directly to CPU address space.
 * There are multiple MCAs in the system, they are grouped together later in
 * 14.5, based on the mappings calculated in 7.4.
 *
 * There are two pieces to configure for D-bit:
 * - D_BIT_MAP - which address bit is used to decide which DIMM is used (this
 *   corresponds to 8GB in the example above), this is common to both DIMMs,
 * - SLOTn_D_VALUE - what value should the D-bit have to access DIMMn, each DIMM
 *   has its own SLOTn_D_VALUE, when one DIMM has this bit set, the other one
 *   must have it cleared; in the (good) example above DIMM Y should have this
 *   bit set. When both DIMMs have the same size then only one D_VALUE must be
 *   set, but it doesn't matter which one.
 *
 * TODO: what if only one DIMM is present? Do we have to set these to something
 * sane (0 and 0 should work) or is it enough that VALID bit is clear for the
 * other DIMM?
 *
 * If bits are assigned in a proper order, we can use a constant table with
 * mappings and assign values from that table to registers describing address
 * bits in a sparse manner, depending on a number of rank and row bits used by
 * a given DIMM. The order is based on the cost of changing individual bits on
 * the DIMM side (considering data locality):
 * 1. Bank group, bank and column bits are sent with every read/write command.
 *    It takes RL = AL + CL + PL after the read command until first DQ bits
 *    appear on the bus. In practice we usually don't care about write delays,
 *    when data is sent to the controller the CPU can already execute further
 *    code, it doesn't have to wait until it is actually written to DRAM. This
 *    is a cheap change.
 *    TODO: this is for DRAM, any additional delay caused by RCD, PHY or MC?
 * 2. Ranks (both master and slave) are selected by CS (and Chip ID for slave
 *    ranks) bits, they are also sent with each command. Depending on MR4
 *    settings, we may need to wait for additional tCAL (CS to Command Address
 *    Latency), DRAM needs some time to "wake up" before it can parse commands.
 *    If tCAL is not used (default in Hostboot), the cost is the same as for BG,
 *    BA, column bits. It doesn't matter whether master or slave ranks are
 *    assigned first, but Hostboot starts with slave ranks - it has 5 bits per
 *    bit map, so it can encode higher numbers.
 * 3. Row bits - these are expensive. Row must be activated before its column
 *    are accessed. Each bank can have one activated row at a time. If there was
 *    an open row (different than the one we want to access), it must be
 *    precharged (it takes tRP before next activation command can be issued),
 *    and then the new row can be activated (after which we have to wait for
 *    tRCD before sending the read/write command). A row cannot be opened
 *    indefinitely, there is both a minimal and maximal period between ACT and
 *    PRE commands (tRAS), and minimums for read to precharge (tRTP), ACT to ACT
 *    for different banks (tRRD, with differentiation between the same and
 *    different bank groups) and Four Activate Window (tFAW). When row changes
 *    don't happen too often, we usually have to wait for tRCD and sometimes
 *    also tRP, on top of the previous delays.
 * 4. D bit. Two DIMMs on a channel share all of its lines except CLK, CS, ODT
 *    and CKE bits. Because we don't have to change CS for a given DIMM, the
 *    cost is the same as 1 (assuming hardware holds CS between commands).
 *    However, this bit has to be assigned lastly (i.e. it has to be the most
 *    significant port address bit) to not introduce holes in the memory space
 *    for two differently sized DIMMs.
 *     * TODO: can we safely map it closer to LSB (at least before row bits)
 *       when we have two DIMMs with the same size?
 *
 * TODO: what about bad DQ bits? Do they impact this in any way? Probably not,
 * unless a whole DIMM is disabled.
 *
 * Below are registers layouts reconstructed from
 * import/chips/p9/common/include/p9n2_mc_scom_addresses_fld.H:
 *	0x05010820                       // P9N2_MCS_PORT02_MCP0XLT0, also PORT13 on +0x10 SCOM addresses
 *		[0]     SLOT0_VALID          // set if DIMM present
 *		[1]     SLOT0_D_VALUE        // set if both DIMMs present and size of DIMM1 > DIMM0
 *		[2]     12GB_ENABLE          // unused (maybe for 12Gb/24Gb DRAM?)
 *		[5]     SLOT0_M0_VALID
 *		[6]     SLOT0_M1_VALID
 *		[9]     SLOT0_S0_VALID
 *		[10]    SLOT0_S1_VALID
 *		[11]    SLOT0_S2_VALID
 *		[12]    SLOT0_B2_VALID       // Hmmm...
 *		[13]    SLOT0_ROW15_VALID
 *		[14]    SLOT0_ROW16_VALID
 *		[15]    SLOT0_ROW17_VALID
 *		[16]    SLOT1_VALID          // set if DIMM present
 *		[17]    SLOT1_D_VALUE        // set if both DIMMs present and size of DIMM1 <= DIMM0
 *		[21]    SLOT1_M0_VALID
 *		[22]    SLOT1_M1_VALID
 *		[25]    SLOT1_S0_VALID
 *		[26]    SLOT1_S1_VALID
 *		[27]    SLOT1_S2_VALID
 *		[28]    SLOT1_B2_VALID       // Hmmm...
 *		[29]    SLOT1_ROW15_VALID
 *		[30]    SLOT1_ROW16_VALID
 *		[31]    SLOT1_ROW17_VALID
 *		[35-39] D_BIT_MAP
 *		[41-43] M0_BIT_MAP           // 3b for M0 but 5b for M1
 *		[47-51] M1_BIT_MAP
 *		[53-55] R17_BIT_MAP
 *		[57-59] R16_BIT_MAP
 *		[61-63] R15_BIT_MAP
 *
 *	0x05010821                       // P9N2_MCS_PORT02_MCP0XLT1
 *		[3-7]   S0_BIT_MAP
 *		[11-15] S1_BIT_MAP
 *		[19-23] S2_BIT_MAP
 *		[35-39] COL4_BIT_MAP
 *		[43-47] COL5_BIT_MAP
 *		[51-55] COL6_BIT_MAP
 *		[59-63] COL7_BIT_MAP
 *
 *	0x05010822                       // P9N2_MCS_PORT02_MCP0XLT2
 *		[3-7]   COL8_BIT_MAP
 *		[11-15] COL9_BIT_MAP
 *		[19-23] BANK0_BIT_MAP
 *		[27-31] BANK1_BIT_MAP
 *		[35-39] BANK2_BIT_MAP        // Hmmm...
 *		[43-47] BANK_GROUP0_BIT_MAP
 *		[51-55] BANK_GROUP1_BIT_MAP
 *
 * All *_BIT_MAP fields above are encoded. Note that some of them are 3b long,
 * those can map only PA 0 through 7.
 */

static uint64_t dimms_rank_config(mca_data_t *mca, uint64_t xlt0, int update_d_bit)
{
	uint64_t val = 0;
	int me;
	int max_row_bits = 0;

	for (me = 0; me < DIMMS_PER_MCA; me++) {
		if (mca->dimm[me].present) {
			int other = me ^ 1;
			int height = mca->dimm[me].log_ranks / mca->dimm[me].mranks;
			/*
			 * Note: this depends on width/density having values as encoded in
			 * SPD and istep_13.h. Please do not change them.
			 */
			int row_bits = 12 + mca->dimm[me].density - mca->dimm[me].width;
			if (row_bits > max_row_bits)
				max_row_bits = row_bits;

			val |= PPC_BIT(0 + 16*me);

			/* When mixing rules are followed, bigger density = bigger size */
			if (mca->dimm[other].present &&
			    mca->dimm[other].density > mca->dimm[me].density)
				val |= PPC_BIT(1 + 16*me);

			/* M1 is used first, then M0 */
			if (mca->dimm[me].mranks > 1)
				val |= PPC_BIT(6 + 16*me);

			if (mca->dimm[me].mranks > 2)
				val |= PPC_BIT(5 + 16*me);

			/* Same with S2, S1, S0 */
			if (height > 1)
				val |= PPC_BIT(11 + 16*me);

			if (height > 2)
				val |= PPC_BIT(10 + 16*me);

			if (height > 4)
				val |= PPC_BIT(9 + 16*me);

			/* Row bits */
			if (row_bits > 15)
				val |= PPC_BIT(13 + 16*me);

			if (row_bits > 16)
				val |= PPC_BIT(14 + 16*me);

			if (row_bits > 17)
				val |= PPC_BIT(15 + 16*me);
		}
	}

	/* When both DIMMs are present and have the same sizes, D_VALUE was not set. */
	if (mca->dimm[0].density == mca->dimm[1].density)
		val |= PPC_BIT(1);

	val |= xlt0;

	if (update_d_bit) {
		/*
		 * In order for this to work:
		 * - old D-bit must have the value it would have for 18 row bits
		 * - changes happen only in PA0-PA7 range
		 * - D-bit is always numerically the lowest assigned PA index
		 *
		 * These assumptions are always true except for non-3DS 1R DIMMs, but
		 * those do not set update_d_bit.
		 */
		uint64_t dbit = xlt0 & PPC_BITMASK(35, 39);
		dbit += ((uint64_t)(18 - max_row_bits)) << 39;
		val = (val & ~PPC_BITMASK(35, 39)) | dbit;
	}

	return val;
}

enum pa_encoding {
	NA = 0,
	PA0 = 0,
	PA1,
	PA2,
	PA3,
	PA4,
	PA5,
	PA6,
	PA7,
	PA22 = 8,	// Defined but not used by Hostboot
	PA23,
	PA24,
	PA25,
	PA26,
	PA27,
	PA28,
	PA29,
	PA30,
	PA31,
	PA32	// 0b10010
};

/*
 * M - master aka package ranks
 * H - height (1 for non-3DS devices)
 */
enum mc_rank_config {
	/* SDP, DDP, QDP DIMMs */
	M1H1_ONE_DIMM,
	M1H1_TWO_DIMMS,
	M2H1,
	M4H1,
	/* TODO: add 3DS DIMMs when needed */
};

#define MCP0XLT0(D, M0, M1, R17, R16, R15) \
(PPC_SHIFT((D), 39) | PPC_SHIFT((M0), 43) | PPC_SHIFT((M1), 51) | \
 PPC_SHIFT((R17), 55) | PPC_SHIFT((R16), 59) | PPC_SHIFT((R15), 63))

#define MCP0XLT1(S0, S1, S2, COL4, COL5, COL6, COL7) \
(PPC_SHIFT((S0), 7) | PPC_SHIFT((S1), 15) | PPC_SHIFT((S2), 23) | \
 PPC_SHIFT((COL4), 39) | PPC_SHIFT((COL5), 47) | PPC_SHIFT((COL6), 55) | \
 PPC_SHIFT((COL7), 63))

#define MCP0XLT2(COL8, COL9, BA0, BA1, BG0, BG1) \
(PPC_SHIFT((COL8), 7) | PPC_SHIFT((COL9), 15) | PPC_SHIFT((BA0), 23) | \
 PPC_SHIFT((BA1), 31) | PPC_SHIFT((BG0), 47) | PPC_SHIFT((BG1), 55))

/*
 * xlt_tables[rank_configuration][reg_index]
 *
 * rank_configuration: see enum above
 * reg_index: MCP0XLT0, MCP0XLT1, MCP0XLT2
 *
 * Width and density do not matter directly, only through number of row bits.
 * Different widths cannot be mixed on the same port, but densities can, and
 * consequently row bits can, too. Assume that all bitmaps can be configured,
 * as long as 'valid' bits are set properly.
 *
 * For anything else than 1R non-3DS devices D-bit is patched by code. Initial
 * value in tables below is PA that would be assigned for DRAM with 18 row bits.
 * When two DIMMs with different densities are installed in one port, use number
 * of row bits of a bigger DIMM.
 */
static const uint64_t xlt_tables[][3] = {
	/* 1R, one DIMM under port */
	{
		MCP0XLT0(NA, NA, NA, PA5, PA6, PA7),
		MCP0XLT1(NA, NA, NA, PA28, PA27, PA26, PA25),
		MCP0XLT2(PA24, PA23, PA29, PA30, PA31, PA32),
	},
	/* 1R, both DIMMs under port */
	{
		MCP0XLT0(PA29, NA, NA, PA4, PA5, PA6),
		MCP0XLT1(NA, NA, NA, PA28, PA27, PA26, PA25),
		MCP0XLT2(PA24, PA23, PA7, PA30, PA31, PA32),
	},
	/* 2R */
	{
		MCP0XLT0(PA3, NA, PA29, PA4, PA5, PA6),
		MCP0XLT1(NA, NA, NA, PA28, PA27, PA26, PA25),
		MCP0XLT2(PA24, PA23, PA7, PA30, PA31, PA32),
	},
	/* 4R */
	{
		MCP0XLT0(PA2, PA6, PA29, PA3, PA4, PA5),
		MCP0XLT1(NA, NA, NA, PA28, PA27, PA26, PA25),
		MCP0XLT2(PA24, PA23, PA7, PA30, PA31, PA32),
	},
	/* TODO: 3DS */
};

static void setup_xlate_map(int mcs_i, int mca_i)
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
	 *
	 * Note: mixing rules do not specify explicitly if two 3DS of different
	 * heights can be mixed. In that case log_ranks/mranks could have bad value,
	 * but it would still be different than 1.
	 */
	int n_dimms = (mca->dimm[0].present && mca->dimm[1].present) ? 2 : 1;
	int mranks = mca->dimm[0].mranks | mca->dimm[1].mranks;
	int log_ranks = mca->dimm[0].log_ranks | mca->dimm[1].log_ranks;
	int is_3DS = (log_ranks / mranks) != 1;
	int update_d = log_ranks != 1;	// Logically the same as '(mranks != 1) | is_3DS'
	chiplet_id_t nest = mcs_to_nest[id];
	enum mc_rank_config cfg = M1H1_ONE_DIMM;

	if (is_3DS) {
		die("3DS DIMMs not yet supported\n");
	}
	else
	{
		switch (mranks) {
		case 1:
			/* One DIMM is default */
			if (n_dimms == 2)
				cfg = M1H1_TWO_DIMMS;
			break;
		case 2:
			cfg = M2H1;
			break;
		case 4:
			cfg = M4H1;
			break;
		default:
			/* Should be impossible to reach */
			die("Bad number of package ranks: %d\n", mranks);
			break;
		}
	}

	/* MCS_PORT02_MCP0XLT0 (?) */
	write_scom_for_chiplet(nest, 0x05010820 + mca_i * mca_mul,
	                       dimms_rank_config(mca, xlt_tables[cfg][0], update_d));

	/* MCS_PORT02_MCP0XLT1 (?) */
	write_scom_for_chiplet(nest, 0x05010821 + mca_i * mca_mul,
	                       xlt_tables[cfg][1]);

	/* MCS_PORT02_MCP0XLT2 (?) */
	write_scom_for_chiplet(nest, 0x05010822 + mca_i * mca_mul,
	                       xlt_tables[cfg][2]);

}

static void enable_pm(int mcs_i, int mca_i)
{
	const int ATTR_MSS_MRW_POWER_CONTROL_REQUESTED = 0;
	/*
	 * Enable Power management based off of mrw_power_control_requested
	 * "Before enabling power controls, run the parity disable workaround"
	 * This is a loop over MCAs inside a loop over MCAs. Is this necessary?
	 *	for each functional MCA
	 *	  // > The workaround is needed iff
	 *	  // > 1) greater than or equal to DD2
	 *	  // > 2) self time refresh is enabled
	 *	  // > 3) the DIMM's are not TSV                // TSV = 3DS
	 *	  // > 4) a 4R DIMM is present
	 *	  // TODO: skip for now, we do not have any 4R, non-3DS sticks to test it
	 *	  str_non_tsv_parity()
	 */

	/* MC01.PORT0.SRQ.PC.MBARPC0Q
	  if ATTR_MSS_MRW_POWER_CONTROL_REQUESTED ==            // default 0 == off
		  ENUM_ATTR_MSS_MRW_IDLE_POWER_CONTROL_REQUESTED_POWER_DOWN         ||        // 1
		  ENUM_ATTR_MSS_MRW_IDLE_POWER_CONTROL_REQUESTED_PD_AND_STR_CLK     ||        // 2
		  ENUM_ATTR_MSS_MRW_IDLE_POWER_CONTROL_REQUESTED_PD_AND_STR_CLK_STOP:         // 3
		[2] MBARPC0Q_CFG_MIN_MAX_DOMAINS_ENABLE = 1
	*/
	if (ATTR_MSS_MRW_POWER_CONTROL_REQUESTED)
		mca_and_or(mcs_ids[mcs_i], mca_i, MBARPC0Q, ~0,
		           PPC_BIT(MBARPC0Q_CFG_MIN_MAX_DOMAINS_ENABLE));
}

static void apply_mark_store(int mcs_i, int mca_i)
{
	/*
	 * FIXME: where do the values written to MVPD come from? They are all 0s in
	 * SCOM dump, which makes this function no-op.
	 */
	const uint64_t ATTR_MSS_MVPD_FWMS[8] = {0};
	int i;

	for (i = 0; i < ARRAY_SIZE(ATTR_MSS_MVPD_FWMS); i++) {
		if (ATTR_MSS_MVPD_FWMS[i] == 0)
			continue;

		/* MC01.PORT0.ECC64.SCOM.FWMS{0-7}
		  [all]   0
		  [0-22]  from ATTR_MSS_MVPD_FWMS
		*/
		mca_and_or(mcs_ids[mcs_i], mca_i, FWMS0 + i,
		           0, ATTR_MSS_MVPD_FWMS[i]);
	}
}

static void fir_unmask(int mcs_i)
{

	chiplet_id_t id = mcs_ids[mcs_i];
	int mca_i;

	/*
	 * "All mcbist attentions are already special attentions"
	 *
	 * These include broadcast_out_of_sync() workaround.
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRACT0
		  [3]   MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC =  0
		  [10]  MCBISTFIRQ_MCBIST_PROGRAM_COMPLETE =  1
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRACT1
		  [3]   MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC =  1
		  [10]  MCBISTFIRQ_MCBIST_PROGRAM_COMPLETE =  0
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRMASK
		  [3]   MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC =  0 // recoverable_error (0,1,0)
		  [10]  MCBISTFIRQ_MCBIST_PROGRAM_COMPLETE =  0     // attention (1,0,0)
	 */
	/*
	 * TODO: check if this works with bootblock in SEEPROM too. We don't have
	 * interrupt handlers set up in that case.
	 */
	scom_and_or_for_chiplet(id, MCBISTFIRACT0,
	                        ~(PPC_BIT(MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC) |
	                          PPC_BIT(MCBISTFIRQ_MCBIST_PROGRAM_COMPLETE)),
	                        PPC_BIT(MCBISTFIRQ_MCBIST_PROGRAM_COMPLETE));
	scom_and_or_for_chiplet(id, MCBISTFIRACT1,
	                        ~(PPC_BIT(MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC) |
	                          PPC_BIT(MCBISTFIRQ_MCBIST_PROGRAM_COMPLETE)),
	                        PPC_BIT(MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC));
	scom_and_or_for_chiplet(id, MCBISTFIRMASK,
	                        ~(PPC_BIT(MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC) |
	                          PPC_BIT(MCBISTFIRQ_MCBIST_PROGRAM_COMPLETE)),
	                        0);

	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		if (!mem_data.mcs[mcs_i].mca[mca_i].functional)
			continue;

		/* From broadcast_out_of_sync() workaround:
		MC01.PORT0.ECC64.SCOM.RECR
			[26]  MBSECCQ_ENABLE_UE_NOISE_WINDOW =  0
		*/
		mca_and_or(id, mca_i, RECR, ~PPC_BIT(MBSECCQ_ENABLE_UE_NOISE_WINDOW), 0);

		/*
		MC01.PORT0.ECC64.SCOM.ACTION0
		  [33]  FIR_MAINTENANCE_AUE =                     0
		  [36]  FIR_MAINTENANCE_IAUE =                    0
		  [41]  FIR_SCOM_PARITY_CLASS_STATUS =            0
		  [42]  FIR_SCOM_PARITY_CLASS_RECOVERABLE =       0
		  [43]  FIR_SCOM_PARITY_CLASS_UNRECOVERABLE =     0
		  [44]  FIR_ECC_CORRECTOR_INTERNAL_PARITY_ERROR = 0
		  [45]  FIR_WRITE_RMW_CE =                        0
		  [46]  FIR_WRITE_RMW_UE =                        0
		  [48]  FIR_WDF_OVERRUN_ERROR_0 =                 0
		  [49]  FIR_WDF_OVERRUN_ERROR_1 =                 0
		  [50]  FIR_WDF_SCOM_SEQUENCE_ERROR =             0
		  [51]  FIR_WDF_STATE_MACHINE_ERROR =             0
		  [52]  FIR_WDF_MISC_REGISTER_PARITY_ERROR =      0
		  [53]  FIR_WRT_SCOM_SEQUENCE_ERROR =             0
		  [54]  FIR_WRT_MISC_REGISTER_PARITY_ERROR =      0
		  [55]  FIR_ECC_GENERATOR_INTERNAL_PARITY_ERROR = 0
		  [56]  FIR_READ_BUFFER_OVERFLOW_ERROR =          0
		  [57]  FIR_WDF_ASYNC_INTERFACE_ERROR =           0
		  [58]  FIR_READ_ASYNC_INTERFACE_PARITY_ERROR =   0
		  [59]  FIR_READ_ASYNC_INTERFACE_SEQUENCE_ERROR = 0
		MC01.PORT0.ECC64.SCOM.ACTION1
		  [33]  FIR_MAINTENANCE_AUE =                     1
		  [36]  FIR_MAINTENANCE_IAUE =                    1
		  [41]  FIR_SCOM_PARITY_CLASS_STATUS =            1
		  [42]  FIR_SCOM_PARITY_CLASS_RECOVERABLE =       1
		  [43]  FIR_SCOM_PARITY_CLASS_UNRECOVERABLE =     0
		  [44]  FIR_ECC_CORRECTOR_INTERNAL_PARITY_ERROR = 0
		  [45]  FIR_WRITE_RMW_CE =                        1
		  [46]  FIR_WRITE_RMW_UE =                        0
		  [48]  FIR_WDF_OVERRUN_ERROR_0 =                 0
		  [49]  FIR_WDF_OVERRUN_ERROR_1 =                 0
		  [50]  FIR_WDF_SCOM_SEQUENCE_ERROR =             0
		  [51]  FIR_WDF_STATE_MACHINE_ERROR =             0
		  [52]  FIR_WDF_MISC_REGISTER_PARITY_ERROR =      0
		  [53]  FIR_WRT_SCOM_SEQUENCE_ERROR =             0
		  [54]  FIR_WRT_MISC_REGISTER_PARITY_ERROR =      0
		  [55]  FIR_ECC_GENERATOR_INTERNAL_PARITY_ERROR = 0
		  [56]  FIR_READ_BUFFER_OVERFLOW_ERROR =          0
		  [57]  FIR_WDF_ASYNC_INTERFACE_ERROR =           0
		  [58]  FIR_READ_ASYNC_INTERFACE_PARITY_ERROR =   0
		  [59]  FIR_READ_ASYNC_INTERFACE_SEQUENCE_ERROR = 0
		MC01.PORT0.ECC64.SCOM.MASK
		  [33]  FIR_MAINTENANCE_AUE =                     0   // recoverable_error (0,1,0)
		  [36]  FIR_MAINTENANCE_IAUE =                    0   // recoverable_error (0,1,0)
		  [41]  FIR_SCOM_PARITY_CLASS_STATUS =            0   // recoverable_error (0,1,0)
		  [42]  FIR_SCOM_PARITY_CLASS_RECOVERABLE =       0   // recoverable_error (0,1,0)
		  [43]  FIR_SCOM_PARITY_CLASS_UNRECOVERABLE =     0   // checkstop (0,0,0)
		  [44]  FIR_ECC_CORRECTOR_INTERNAL_PARITY_ERROR = 0   // checkstop (0,0,0)
		  [45]  FIR_WRITE_RMW_CE =                        0   // recoverable_error (0,1,0)
		  [46]  FIR_WRITE_RMW_UE =                        0   // checkstop (0,0,0)
		  [48]  FIR_WDF_OVERRUN_ERROR_0 =                 0   // checkstop (0,0,0)
		  [49]  FIR_WDF_OVERRUN_ERROR_1 =                 0   // checkstop (0,0,0)
		  [50]  FIR_WDF_SCOM_SEQUENCE_ERROR =             0   // checkstop (0,0,0)
		  [51]  FIR_WDF_STATE_MACHINE_ERROR =             0   // checkstop (0,0,0)
		  [52]  FIR_WDF_MISC_REGISTER_PARITY_ERROR =      0   // checkstop (0,0,0)
		  [53]  FIR_WRT_SCOM_SEQUENCE_ERROR =             0   // checkstop (0,0,0)
		  [54]  FIR_WRT_MISC_REGISTER_PARITY_ERROR =      0   // checkstop (0,0,0)
		  [55]  FIR_ECC_GENERATOR_INTERNAL_PARITY_ERROR = 0   // checkstop (0,0,0)
		  [56]  FIR_READ_BUFFER_OVERFLOW_ERROR =          0   // checkstop (0,0,0)
		  [57]  FIR_WDF_ASYNC_INTERFACE_ERROR =           0   // checkstop (0,0,0)
		  [58]  FIR_READ_ASYNC_INTERFACE_PARITY_ERROR =   0   // checkstop (0,0,0)
		  [59]  FIR_READ_ASYNC_INTERFACE_SEQUENCE_ERROR = 0   // checkstop (0,0,0)
		*/
		mca_and_or(id, mca_i, ECC_FIR_ACTION0,
		           ~(PPC_BIT(ECC_FIR_MAINTENANCE_AUE) |
		             PPC_BIT(ECC_FIR_MAINTENANCE_IAUE) |
		             PPC_BITMASK(41, 46) | PPC_BITMASK(48, 59)),
		           0);

		mca_and_or(id, mca_i, ECC_FIR_ACTION1,
		           ~(PPC_BIT(ECC_FIR_MAINTENANCE_AUE) |
		             PPC_BIT(ECC_FIR_MAINTENANCE_IAUE) |
		             PPC_BITMASK(41, 46) | PPC_BITMASK(48, 59)),
		           PPC_BIT(ECC_FIR_MAINTENANCE_AUE) |
		           PPC_BIT(ECC_FIR_MAINTENANCE_IAUE) |
		           PPC_BIT(ECC_FIR_SCOM_PARITY_CLASS_STATUS) |
		           PPC_BIT(ECC_FIR_SCOM_PARITY_CLASS_RECOVERABLE) |
		           PPC_BIT(ECC_FIR_WRITE_RMW_CE));

		mca_and_or(id, mca_i, ECC_FIR_MASK,
		           ~(PPC_BIT(ECC_FIR_MAINTENANCE_AUE) |
		             PPC_BIT(ECC_FIR_MAINTENANCE_IAUE) |
		             PPC_BITMASK(41, 46) | PPC_BITMASK(48, 59)),
		           0);
	}
}

/*
 * 13.13 mss_draminit_mc: Hand off control to MC
 *
 * a) p9_mss_draminit_mc.C  (mcbist) - Nimbus
 * b) p9c_mss_draminit_mc.C  (membuf) -Cumulus
 *    - P9 Cumulus -- Set IML complete bit in centaur
 *    - Start main refresh engine
 *    - Refresh, periodic calibration, power controls
 *    - Turn on ECC checking on memory accesses
 *    - Note at this point memory FIRs can be monitored by PRD
 */
void istep_13_13(void)
{
	printk(BIOS_EMERG, "starting istep 13.13\n");
	int mcs_i, mca_i;

	report_istep(13,13);

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		/* No need to initialize a non-functional MCS */
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			chiplet_id_t id = mcs_ids[mcs_i];
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			setup_xlate_map(mcs_i, mca_i);

			/* Set up read pointer delay */
			/* MC01.PORT0.ECC64.SCOM.RECR
				  [6-8] MBSECCQ_READ_POINTER_DELAY = 1    // code sets this to "ON", but this field is numerical value
				  // Not sure where this attr comes from or what is its default value. Assume !0 = 1 -> TCE correction enabled
				  [27]  MBSECCQ_ENABLE_TCE_CORRECTION = !ATTR_MNFG_FLAGS.MNFG_REPAIRS_DISABLED_ATTR
			*/
			mca_and_or(id, mca_i, RECR,
			           ~(PPC_BITMASK(6, 8) | PPC_BIT(MBSECCQ_ENABLE_TCE_CORRECTION)),
			           PPC_SHIFT(1, MBSECCQ_READ_POINTER_DELAY) |
			           PPC_BIT(MBSECCQ_ENABLE_TCE_CORRECTION));

			enable_pm(mcs_i, mca_i);

			/*
			 * This was already done after draminit_cke_helper, search for "Per
			 * conversation with Shelton and Steve..." in 13.10, "however that
			 * might be a work-around so we set it low here kind of like
			 * belt-and-suspenders. BRS"
			 *
			 * MC01.PORT0.SRQ.MBA_FARB5Q
			 *	  [5]   MBA_FARB5Q_CFG_CCS_ADDR_MUX_SEL = 0
			 */
			mca_and_or(id, mca_i, MBA_FARB5Q,
			           ~PPC_BIT(MBA_FARB5Q_CFG_CCS_ADDR_MUX_SEL), 0);

			/* MC01.PORT0.SRQ.MBA_FARB0Q
				  [57]  MBA_FARB0Q_CFG_PORT_FAIL_DISABLE = 0
			*/
			mca_and_or(id, mca_i, MBA_FARB0Q,
			           ~PPC_BIT(MBA_FARB0Q_CFG_PORT_FAIL_DISABLE), 0);

			/*
			 * "MC work around for OE bug (seen in periodics + PHY)
			 * Turn on output-enable always on. Shelton tells me they'll fix
			 * for DD2"
			 *
			 * This is also surrounded by '#ifndef REMOVE_FOR_DD2', but this
			 * name is nowhere else to be found. If this still have to be used,
			 * we may as well merge it with the previous write.
			 *
			 * MC01.PORT0.SRQ.MBA_FARB0Q
			 *	  [55]  MBA_FARB0Q_CFG_OE_ALWAYS_ON = 1
			 */
			mca_and_or(id, mca_i, MBA_FARB0Q, ~0,
			           PPC_BIT(MBA_FARB0Q_CFG_OE_ALWAYS_ON));

			/* MC01.PORT0.SRQ.PC.MBAREF0Q
				  [0] MBAREF0Q_CFG_REFRESH_ENABLE = 1
			*/
			mca_and_or(id, mca_i, MBAREF0Q, ~0, PPC_BIT(MBAREF0Q_CFG_REFRESH_ENABLE));

			/* Enable periodic calibration */
			/*
			 * A large chunk of function enable_periodic_cal() in Hostboot is
			 * disabled, protected by #ifdef TODO_166433_PERIODICS, which also
			 * isn't mentioned anywhere else. This is what is left:
			  MC01.PORT0.SRQ.MBA_CAL3Q
				  [all]   0
				  [0-1]   MBA_CAL3Q_CFG_INTERNAL_ZQ_TB =        0x3
				  [2-9]   MBA_CAL3Q_CFG_INTERNAL_ZQ_LENGTH =    0xff
				  [10-11] MBA_CAL3Q_CFG_EXTERNAL_ZQ_TB =        0x3
				  [12-19] MBA_CAL3Q_CFG_EXTERNAL_ZQ_LENGTH =    0xff
				  [20-21] MBA_CAL3Q_CFG_RDCLK_SYSCLK_TB =       0x3
				  [22-29] MBA_CAL3Q_CFG_RDCLK_SYSCLK_LENGTH =   0xff
				  [30-31] MBA_CAL3Q_CFG_DQS_ALIGNMENT_TB =      0x3
				  [32-39] MBA_CAL3Q_CFG_DQS_ALIGNMENT_LENGTH =  0xff
				  [40-41] MBA_CAL3Q_CFG_MPR_READEYE_TB =        0x3
				  [42-49] MBA_CAL3Q_CFG_MPR_READEYE_LENGTH =    0xff
				  [50-51] MBA_CAL3Q_CFG_ALL_PERIODIC_TB =       0x3
				  [52-59] MBA_CAL3Q_CFG_ALL_PERIODIC_LENGTH =   0xff
				  // Or simpler: 0xfffffffffffffff0
			*/
			mca_and_or(id, mca_i, MBA_CAL3Q, 0, PPC_BITMASK(0, 59));

			/* Enable read ECC
			  MC01.PORT0.ECC64.SCOM.RECR                    // 0x07010A0A
				  [0]     MBSECCQ_DISABLE_MEMORY_ECC_CHECK_CORRECT =  0
				  [1]     MBSECCQ_DISABLE_MEMORY_ECC_CORRECT =        0
				  [29]    MBSECCQ_USE_ADDRESS_HASH =                  1
				  // Docs don't describe the encoding, code suggests this inverts data, toggles checks
				  [30-31] MBSECCQ_DATA_INVERSION =                    3
			*/
			mca_and_or(id, mca_i, RECR,
			           ~(PPC_BITMASK(0, 1) | PPC_BITMASK(29, 31)),
			           PPC_BIT(MBSECCQ_USE_ADDRESS_HASH) |
			           PPC_SHIFT(3, MBSECCQ_DATA_INVERSION));

			apply_mark_store(mcs_i, mca_i);
		}

		fir_unmask(mcs_i);
	}

	printk(BIOS_EMERG, "ending istep 13.13\n");
}
