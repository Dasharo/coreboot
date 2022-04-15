/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_14.h>
#include <cpu/power/istep_13.h>
#include <cpu/power/proc.h>
#include <cpu/power/spr.h>
#include <console/console.h>
#include <timer.h>

#include "istep_13_scom.h"
#include "mcbist.h"

/*
 * 14.1 mss_memdiag: Mainstore Pattern Testing
 *
 * - The following step documents the generalities of this step
 *   - In FW PRD will control mem diags via interrupts. It doesn't use
 *     mss_memdiags.C directly but the HWP subroutines
 *   - In cronus it will execute mss_memdiags.C directly
 * b) p9_mss_memdiags.C (mcbist)--Nimbus
 * c) p9_mss_memdiags.C (mba) -- Cumulus
 *    - Prior to running this procedure will apply known DQ bad bits to prevent
 *      them from participating in training. This information is extracted from
 *      the bad DQ attribute and applied to Hardware
 *    - Nimbus uses the mcbist engine
 *      - Still supports superfast read/init/scrub
 *    - Cumulus/Centaur uses the scrub engine
 *    - Modes:
 *      - Minimal: Write-only with 0's
 *      - Standard: Write of 0's followed by a Read
 *      - Medium: Write-followed by Read, 4 patterns, last of 0's
 *      - Max: Write-followed by Read, 9 patterns, last of 0's
 *    - Run on the host
 *    - This procedure will update the bad DQ attribute for each dimm based on
 *      its findings
 *    - At the end of this procedure sets FIR masks correctly for runtime
 *      analysis
 *    - All subsequent repairs are considered runtime issues
 */

static void fir_unmask(uint8_t chip, int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int mca_i;
	const int is_dd20 = get_dd() == 0x20;
	/* Bits in other registers (act0, mask) are already set properly.
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRACT1
		  [3]   MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC =  0 // checkstop (0,0,0)
	*/
	scom_and_or_for_chiplet(chip, id, MCBISTFIRACT1,
	                        ~PPC_BIT(MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC), 0);

	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		uint64_t val;
		if (!mem_data[chip].mcs[mcs_i].mca[mca_i].functional)
			continue;

		/* From broadcast_out_of_sync() workaround:
		MC01.PORT0.ECC64.SCOM.RECR
			[26]  MBSECCQ_ENABLE_UE_NOISE_WINDOW =  1
		*/
		mca_and_or(chip, id, mca_i, RECR, ~0, PPC_BIT(MBSECCQ_ENABLE_UE_NOISE_WINDOW));

		/*
		 * Read out the wr_done and rd_tag delays and find min and set the RCD
		 * Protect Time to this value.
		 *
		 * MC01.PORT0.SRQ.MBA_DSM0Q
		 *   [24-29] MBA_DSM0Q_CFG_WRDONE_DLY
		 *   [36-41] MBA_DSM0Q_CFG_RDTAG_DLY
		 *
		 * MC01.PORT0.SRQ.MBA_FARB0Q
		 *   [48-53] MBA_FARB0Q_CFG_RCD_PROTECTION_TIME
		 */
		val = mca_read(chip, id, mca_i, MBA_DSM0Q);
		val = MIN((val & PPC_BITMASK(24, 29)) >> 29,
		          (val & PPC_BITMASK(36, 41)) >> 41);
		mca_and_or(chip, id, mca_i, MBA_FARB0Q,
		           ~PPC_BITMASK(48, 53),
		           PPC_PLACE(val, MBA_FARB0Q_CFG_RCD_PROTECTION_TIME,
		                     MBA_FARB0Q_CFG_RCD_PROTECTION_TIME_LEN));

		/*
		 * Due to hardware defect with DD2.0 certain errors are not handled
		 * properly. As a result, these firs are marked as checkstop for DD2 to
		 * avoid any mishandling.
		 *
		 * MCA_FIR_MAINLINE_RCD stays masked on newer platforms. ACT0 and ACT1
		 * for RCD are not touched by Hostboot, but for simplicity set those to
		 * 0 always - they are "don't care" if masked, and 0 is their reset
		 * value. Affected bits are annotated with asterisk below - whatever is
		 * mentioned below is changed to checkstop for those bits.
		 *
		 * This also affects Cumulus DD1.0, but the rest of the code is for
		 * Nimbus only so don't bother checking for it.
		 *
		 * MC01.PORT0.ECC64.SCOM.ACTION0
		 *   [13] FIR_MAINLINE_AUE =         0
		 *   [14] FIR_MAINLINE_UE =          0
		 *   [15] FIR_MAINLINE_RCD =         0
		 *   [16] FIR_MAINLINE_IAUE =        0
		 *   [17] FIR_MAINLINE_IUE =         0
		 *   [37] MCA_FIR_MAINTENANCE_IUE =  0
		 * MC01.PORT0.ECC64.SCOM.ACTION1
		 *   [13] FIR_MAINLINE_AUE =         0
		 *   [14] FIR_MAINLINE_UE =          1*
		 *   [15] FIR_MAINLINE_RCD =         0
		 *   [16] FIR_MAINLINE_IAUE =        0
		 *   [17] FIR_MAINLINE_IUE =         1
		 *   [33] MCA_FIR_MAINTENANCE_AUE =  0  // Hostboot clears AUE and IAUE without
		 *   [36] MCA_FIR_MAINTENANCE_IAUE = 0  // unmasking, with no explanation why
		 *   [37] MCA_FIR_MAINTENANCE_IUE =  1
		 * MC01.PORT0.ECC64.SCOM.MASK
		 *   [13] FIR_MAINLINE_AUE =         0  // checkstop (0,0,0)
		 *   [14] FIR_MAINLINE_UE =          0  // *recoverable_error (0,1,0)
		 *   [15] FIR_MAINLINE_RCD =         1* // *masked (X,X,1)
		 *   [16] FIR_MAINLINE_IAUE =        0  // checkstop (0,0,0)
		 *   [17] FIR_MAINLINE_IUE =         0  // recoverable_error (0,1,0)
		 *   [37] MCA_FIR_MAINTENANCE_IUE =  0  // recoverable_error (0,1,0)
		 */
		mca_and_or(chip, id, mca_i, ECC_FIR_ACTION0,
		           ~(PPC_BITMASK(13, 17) | PPC_BIT(MCA_FIR_MAINTENANCE_IUE)),
		           0);
		mca_and_or(chip, id, mca_i, ECC_FIR_ACTION1,
		           ~(PPC_BITMASK(13, 17) | PPC_BIT(ECC_FIR_MAINTENANCE_AUE) |
		             PPC_BITMASK(36, 37)),
		           (is_dd20 ? 0 : PPC_BIT(FIR_MAINLINE_UE)) |
		           PPC_BIT(FIR_MAINLINE_IUE) | PPC_BIT(MCA_FIR_MAINTENANCE_IUE));
		mca_and_or(chip, id, mca_i, ECC_FIR_MASK,
		           ~(PPC_BITMASK(13, 17) |
		           PPC_BIT(MCA_FIR_MAINTENANCE_IUE)),
		           (is_dd20 ? 0 : PPC_BIT(FIR_MAINLINE_RCD)));

		/*
		 * WARNING: checkstop is encoded differently (1,0,0). **Do not** try to
		 * make a function/macro that pretends to be universal.
		 *
		 * MC01.PORT0.SRQ.MBACALFIR_ACTION0
		 *   [13] MBACALFIR_PORT_FAIL = 0*
		 * MC01.PORT0.SRQ.MBACALFIR_ACTION1
		 *   [13] MBACALFIR_PORT_FAIL = 1*
		 * MC01.PORT0.SRQ.MBACALFIR_MASK
		 *   [13] MBACALFIR_PORT_FAIL = 0  // *recoverable_error (0,1,0)
		 */
		mca_and_or(chip, id, mca_i, MBACALFIR_ACTION0,
		           ~PPC_BIT(13),
		           (is_dd20 ? PPC_BIT(MBACALFIR_PORT_FAIL) : 0));
		mca_and_or(chip, id, mca_i, MBACALFIR_ACTION1,
		           ~PPC_BIT(MBACALFIR_PORT_FAIL),
		           (is_dd20 ? 0 : PPC_BIT(MBACALFIR_PORT_FAIL)));
		mca_and_or(chip, id, mca_i, MBACALFIR_MASK, ~PPC_BIT(MBACALFIR_PORT_FAIL), 0);

		/*
		 * Enable port fail and RCD recovery
		 * TODO: check if we can set this together with RCD protection time.
		 *
		 * MC01.PORT0.SRQ.MBA_FARB0Q
		 *   [54] MBA_FARB0Q_CFG_DISABLE_RCD_RECOVERY = 0
		 *   [57] MBA_FARB0Q_CFG_PORT_FAIL_DISABLE =    0
		 */
		mca_and_or(chip, id, mca_i, MBA_FARB0Q,
		           ~(PPC_BIT(MBA_FARB0Q_CFG_DISABLE_RCD_RECOVERY) |
		             PPC_BIT(MBA_FARB0Q_CFG_PORT_FAIL_DISABLE)), 0);
	}
}

static void set_fifo_mode(uint8_t chip, int mcs_i, int fifo)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int mca_i;
	/* Make sure fifo is either 0 or 1, nothing else. */
	fifo = !!fifo;

	/* MC01.PORT0.SRQ.MBA_RRQ0Q
	 *   [6] MBA_RRQ0Q_CFG_RRQ_FIFO_MODE = fifo
	 * MC01.PORT0.SRQ.MBA_WRQ0Q
	 *   [5] MBA_WRQ0Q_CFG_WRQ_FIFO_MODE = fifo
	 */
	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		if (!mem_data[chip].mcs[mcs_i].mca[mca_i].functional)
			continue;

		mca_and_or(chip, id, mca_i, MBA_RRQ0Q, ~PPC_BIT(MBA_RRQ0Q_CFG_RRQ_FIFO_MODE),
		           PPC_PLACE(fifo, MBA_RRQ0Q_CFG_RRQ_FIFO_MODE,
		                     MBA_RRQ0Q_CFG_RRQ_FIFO_MODE_LEN));
		mca_and_or(chip, id, mca_i, MBA_WRQ0Q, ~PPC_BIT(MBA_WRQ0Q_CFG_WRQ_FIFO_MODE),
		           PPC_PLACE(fifo, MBA_WRQ0Q_CFG_WRQ_FIFO_MODE,
		                     MBA_WRQ0Q_CFG_WRQ_FIFO_MODE_LEN));
	}
}

static void load_maint_pattern(uint8_t chip, int mcs_i, const uint64_t pat[16])
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/*
	 * Different than in Hostboot:
	 * - Hostboot writes data for second 64B line but doesn't use 128B mode so
	 *   first 64B are repeated
	 *   - Hostboot also manually sets the address for the second half even
	 *     though it would be autoincremented to proper value
	 * - Hostboot writes 4 pairs of 64b chunks of data, we write 8 uint64_t's
	 */
	int mca_i;

	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		int i;
		if (!mem_data[chip].mcs[mcs_i].mca[mca_i].functional)
			continue;

		/* MC01.PORT0.ECC64.SCOM.AACR
		 * [1-9] AACR_ADDRESS = 0b111110000 = 0x1F0
		 * [10]  AACR_AUTOINC = 1
		 * [11]  AACR_ECCGEN =  1
		 */
		mca_write(chip, id, mca_i, AACR,
		          PPC_PLACE(0x1F0, AACR_ADDRESS, AACR_ADDRESS_LEN) |
		          PPC_BIT(AACR_AUTOINC) |
		          PPC_BIT(AACR_ECCGEN));

		for (i = 0; i < 16; i++) {
			/* MC01.PORT0.ECC64.SCOM.AADR - data */
			mca_write(chip, id, mca_i, AADR, pat[i]);
			/*
			 * Although ECC is generated by hardware, we still have to write to
			 * this register to have address incremented. Comments say that
			 * the data also wouldn't be written to RMW buffer without it.
			 */
			/* MC01.PORT0.ECC64.SCOM.AAER - ECC */
			mca_write(chip, id, mca_i, AAER, 0);
		}
	}
}

static const uint64_t patterns[][16] = {
	{0},
	{0x596f75207265616c, 0x6c792073686f756c, 0x646e27742072656c, 0x79206f6e206d656d,
	 0x6f7279206265696e, 0x67207a65726f6564, 0x206279206669726d, 0x776172652e2e2e00},
	{0x4e6576657220756e, 0x646572657374696d, 0x6174652074686520, 0x62616e6477696474,
	 0x68206f6620612073, 0x746174696f6e2077, 0x61676f6e2066756c, 0x6c206f6620746170,
	 0x657320687572746c, 0x696e6720646f776e, 0x2074686520686967, 0x687761792e202d20,
	 0x416e647265772053, 0x2e2054616e656e62, 0x61756d0a00000000},
};

/*
 * Layout of start/end address registers:
 * [0-2]   unused by HW, in Hostboot:
 *     [0-1]   port select
 *     [2]     dimm select
 * [3-4]   mrank (0 to 1)
 * [5-7]   srank (0 to 2)
 * [8-25]  row (0 to 17)
 * [26-32] col (3 to 9)
 * [33-35] bank (0 to 2)
 * [36-37] bank_group (0 to 1)
 *
 * In maintenance mode MCBIST automatically skips unused bits, they can safely
 * be set to 0 for start and 1 for end addresses.
 *
 * Hostboot sets 3 ranges:
 * - 0 to end of first DIMM (aka first DIMM)
 * - 0 to end of address space (aka everything)
 * - first address on first DIMM on last port to end of address space (aka last
 *   port)
 *
 * Assuming that the documentation is correct, when spare bits are not taken
 * into account, all ranges result in [start of DIMM, end of DIMM] range. Maybe
 * they are set only for debug purposes?
 *
 * Trying to use just one range instead.
 */
/*
 * NOTE: Except for setting address ranges, Hostboot repeats all of this for
 * every subtest, even though most of the registers don't change in between.
 */
static void init_mcbist(uint8_t chip, int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	uint64_t val;
	int i;

	/* Fill address ranges */
	/* MC01.MCBIST.MBA_SCOMFIR.MCBSA0Q
	 * [0-37] MCBSA0Q_CFG_START_ADDR_0
	 */
	write_scom_for_chiplet(chip, id, MCBSA0Q, 0);
	/* MC01.MCBIST.MBA_SCOMFIR.MCBEA0Q
	 * [0-37] MCBSA0Q_CFG_END_ADDR_0
	 */
	write_scom_for_chiplet(chip, id, MCBEA0Q, PPC_BITMASK(3, 37));

	/* Hostboot stops MCBIST engine, die() if it is already started instead */
	/* TODO: check all bits (MCBIST was ever started) or just "in progress"? */
	/* MC01.MCBIST.MBA_SCOMFIR.MCB_CNTLSTATQ
	 * [0] MCB_CNTLSTATQ_MCB_IP
	 * [1] MCB_CNTLSTATQ_MCB_DONE
	 * [2] MCB_CNTLSTATQ_MCB_FAIL
	 */
	if ((val = read_scom_for_chiplet(chip, id, MCB_CNTLSTATQ)) != 0)
		die("MCBIST started already (%#16.16llx), this shouldn't happen\n", val);

	/*
	 * Clear MCBIST errors:
	 * - MCBIST Error Status Register - MC01.MCBIST.MBA_SCOMFIR.MCBSTATQ
	 * - MBS Memory Scrub/Read Error Count Register 0 - MC01.MCBIST.MBA_SCOMFIR.MBSEC0Q
	 * - MBS Memory Scrub/Read Error Count Register 1 - MC01.MCBIST.MBA_SCOMFIR.MBSEC1Q
	 * - MCBIST Fault Isolation Register - MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRQ
	 */
	write_scom_for_chiplet(chip, id, MCBSTATQ, 0);
	write_scom_for_chiplet(chip, id, MBSEC0Q, 0);
	write_scom_for_chiplet(chip, id, MBSEC1Q, 0);
	write_scom_for_chiplet(chip, id, MCBISTFIR, 0);

	/* Enable FIFO mode */
	set_fifo_mode(chip, mcs_i, 1);

	/*
	 * Hostboot clears address maps, but they are not used in maintenance
	 * address mode. Also, it sets MCBAGRAQ_CFG_MAINT_DETECT_SRANK_BOUNDARIES
	 * for scrub commands, but not for patterns. I have no idea what possible
	 * implications this has, but without 3DS DIMMs I have no way of testing it.
	 * For now I'll set this bit even for patterns so MCBAGRAQ register can be
	 * written only once instead of each subtest.
	 */
	/* MC01.MCBIST.MBA_SCOMFIR.MCBAGRAQ
	 * [all] 0
	 * [10]  MCBAGRAQ_CFG_MAINT_ADDR_MODE_EN =            1
	 * [12]  MCBAGRAQ_CFG_MAINT_DETECT_SRANK_BOUNDARIES = 1
	 */
	write_scom_for_chiplet(chip, id, MCBAGRAQ,
	                       PPC_BIT(MCBAGRAQ_CFG_MAINT_ADDR_MODE_EN) |
	                       PPC_BIT(MCBAGRAQ_CFG_MAINT_DETECT_SRANK_BOUNDARIES));

	/*
	 * Configure MCBIST
	 *
	 * Enabling MCBCFGQ_CFG_MCB_LEN64 speeds up operations on x4 devices (~70ms
	 * per pass on 16GB DIMM), but slows down x8 (~90ms per pass on 8GB DIMM).
	 * As the difference for x8 is bigger than x4, keep it disabled.
	 *
	 * MCBCFGQ_CFG_PAUSE_ON_ERROR_MODE = 0b10 sets MCBIST to pause on error
	 * after current rank finishes. This is set for scrub only, but as we don't
	 * expect to see any errors, it should be OK to set it for pattern writing
	 * as well.
	 *
	 * MCBCFGQ_CFG_ENABLE_HOST_ATTN is set in Hostboot, but we don't have
	 * interrupt handlers so keep it disabled.
	 */
	/* MC01.MCBIST.MBA_SCOMFIR.MCBCFGQ
	 * [all]   0
	 * [56]    MCBCFGQ_CFG_MCB_LEN64 =           see above
	 * [57-58] MCBCFGQ_CFG_PAUSE_ON_ERROR_MODE = 0 for patterns, 0b10 for scrub
	 * [63]    MCBCFGQ_CFG_ENABLE_HOST_ATTN =    see above
	 */
	write_scom_for_chiplet(chip, id, MCBCFGQ,
	                       PPC_PLACE(0x2, MCBCFGQ_CFG_PAUSE_ON_ERROR_MODE,
	                                 MCBCFGQ_CFG_PAUSE_ON_ERROR_MODE_LEN));

	/*
	 * This sets up memory parameters, mostly gaps between commands. For as fast
	 * as possible, gaps of 0 are configured here.
	 */
	/* MC01.MCBIST.MBA_SCOMFIR.MCBPARMQ */
	write_scom_for_chiplet(chip, id, MCBPARMQ, 0);

	/*
	 * Steps done from this point should be moved out of this function, they
	 * should be done with different patterns before each subtest. Right now
	 * only a pattern of all zeroes is used.
	 */

	/* Data pattern: 8 data registers + 1 ECC register */
	/* TODO: different patterns can be used */
	for (i = 0; i < 9; i++) {
		write_scom_for_chiplet(chip, id, MCBFD0Q + i, patterns[0][i]);
	}

	/* TODO: random seeds */

	/*
	 * Maintenance data pattern
	 *
	 * Difference between this and data pattern above is that this is used for
	 * ALTER and the one above for WRITE. ALTER can write 128 different bytes,
	 * while WRITE repeats a sequence of 64B twice. ALTER is ~3-4 times slower.
	 */
	load_maint_pattern(chip, mcs_i, patterns[0]);

	/*
	 * Load the data rotate config and seeds
	 *
	 * Patterns (fixed) used by Hostboot are self-repeating and either all ones,
	 * all zeroes or alternating bits (0x55/0xAA). Only in the last case
	 * rotating data seeds can make a difference, but it is the same as
	 * inverting.
	 */
	/* MC01.MCBIST.MBA_SCOMFIR.MCBDRCRQ */
	write_scom_for_chiplet(chip, id, MCBDRCRQ, 0);
	/* MC01.MCBIST.MBA_SCOMFIR.MCBDRSRQ */
	write_scom_for_chiplet(chip, id, MCBDRSRQ, 0);

	/*
	 * The following step may be done just once, as long as the same set of
	 * options work for both pattern writing and scrubbing, which so far seems
	 * to be the case.
	 */

	/*
	 * Load MCBIST threshold register
	 *
	 * This one has slightly different settings for patterns than for scrub, but
	 * some of those that are explicitly set for scrubbing are always implicitly
	 * enabled for nonscrub. The only meaningful difference is that some
	 * uncorrectable errors pause MCBIST on scrub, but not on pattern writes.
	 * Lets set them to pause even for pattern writes here and hope for the
	 * best.
	 */
	/* MC01.MCBIST.MBA_SCOMFIR.MBSTRQ
	 * [0-31] those are thresholds for different errors, all of them are set to
	 *        all 1's, meaning that pausing on threshold is disabled
	 * [34]   MBSTRQ_CFG_PAUSE_ON_MPE = 1 for scrub, else 0 (Mark Placed Error)
	 * [35]   MBSTRQ_CFG_PAUSE_ON_UE =  1 for scrub, else 0 (Uncorrectable Error)
	 * [37]   MBSTRQ_CFG_PAUSE_ON_AUE = 1 for scrub, else 0 (Array UE)
	 * [55]   MBSTRQ_CFG_NCE_SOFT_SYMBOL_COUNT_ENABLE  \  1 for scrub, nonscrub
	 * [56]   MBSTRQ_CFG_NCE_INTER_SYMBOL_COUNT_ENABLE  } counts all NCE
	 * [57]   MBSTRQ_CFG_NCE_HARD_SYMBOL_COUNT_ENABLE  /
	 */
	write_scom_for_chiplet(chip, id, MBSTRQ, PPC_BITMASK(0, 31) |
	                       PPC_BIT(MBSTRQ_CFG_PAUSE_ON_MPE) |
	                       PPC_BIT(MBSTRQ_CFG_PAUSE_ON_UE) |
	                       PPC_BIT(MBSTRQ_CFG_PAUSE_ON_AUE) |
	                       PPC_BIT(MBSTRQ_CFG_NCE_SOFT_SYMBOL_COUNT_ENABLE) |
	                       PPC_BIT(MBSTRQ_CFG_NCE_INTER_SYMBOL_COUNT_ENABLE) |
	                       PPC_BIT(MBSTRQ_CFG_NCE_HARD_SYMBOL_COUNT_ENABLE));
}

static void mss_memdiag(uint8_t chips)
{
	uint8_t chip;
	int mcs_i, mca_i;

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
			if (!mem_data[chip].mcs[mcs_i].functional)
				continue;

			/*
			 * FIXME: add testing for chipkill
			 *
			 * Testing touches bad DQ registers. This step also configures MC to
			 * deal with bad nibbles/DQs - see can_recover() in 13.11. It repeats,
			 * to some extent, training done in 13.12 which is TODO. Following the
			 * assumptions made in previous isteps, skip this for now.
			 */
			init_mcbist(chip, mcs_i);

			/*
			 * Add subtests.
			 *
			 * At the very minimum one pattern write is required, otherwise RAM will
			 * have random data, which most likely will throw unrecoverable errors
			 * because ECC is also random.
			 *
			 * Scrubbing may throw errors when address mapping is wrong even when
			 * maintenance pattern write can succeed for the same configuration.
			 */
			for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
				mca_data_t *mca = &mem_data[chip].mcs[mcs_i].mca[mca_i];
				int dimm;
				if (!mca->functional)
					continue;

				for (dimm = 0; dimm < DIMMS_PER_MCA; dimm++) {
					if (!mca->dimm[dimm].present)
						continue;

					add_fixed_pattern_write(chip, mcs_i, mca_i*2 + dimm);
					/*
					 * Hostboot uses separate program for scrub due to
					 * different pausing conditions. Having it in the same
					 * program seems to be working.
					 */
					if (!CONFIG(SKIP_INITIAL_ECC_SCRUB))
						add_scrub(chip, mcs_i, mca_i*2 + dimm);
				}
			}

			/*
			 * TODO: it writes whole RAM, this will take loooooong time. We can
			 * easily start second MCBIST while this is running. This would get more
			 * complicated for more patterns, but it still should be doable without
			 * interrupts reporting completion.
			 *
			 * Also, under right circumstances*, it should be possible to use
			 * broadcast mode for writing to all DIMMs simultaneously.
			 *
			 * *) Proper circumstances are:
			 *    - every port has the same number of DIMMs (or no DIMMs at all)
			 *    - every DIMM has the same:
			 *      - rank configuration
			 *      - number of row and column bits
			 *      - width (and density, but this is implied by previous
			 *        requirements)
			 *      - module family (but we don't support anything but RDIMM anyway)
			 */
			mcbist_execute(chip, mcs_i);
		}
	}

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
			if (!mem_data[chip].mcs[mcs_i].functional)
				continue;

			/*
			 * When there is no other activity on the bus, this should take roughly
			 * (total RAM size under MCS / transfer rate) * number of subtests.
			 *
			 * Not measuring time it takes individual MCBISTs to complete as they
			 * all work in parallel.
			 */
			long time = wait_us(1000*1000*60,
					    (udelay(1), mcbist_is_done(chip, mcs_i)));

			/* TODO: dump error/status registers on failure */
			if (!time) {
				die("MCBIST%d of chip %d times out (%#16.16llx)\n", mcs_i, chip,
				    read_scom_for_chiplet(chip, mcs_ids[mcs_i],
							  MCB_CNTLSTATQ));
			}

			/* Unmask mainline FIRs. */
			fir_unmask(chip, mcs_i);

			/* Turn off FIFO mode to improve performance. */
			set_fifo_mode(chip, mcs_i, 0);
		}
	}
}

void istep_14_1(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 14.1\n");
	report_istep(14,1);

	mss_memdiag(chips);

	printk(BIOS_EMERG, "ending istep 14.1\n");
}
