/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <timer.h>

#include "istep_13_scom.h"

static int test_dll_calib_done(int mcs_i, int mca_i, bool *do_workaround)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	uint64_t status = mca_read(id, mca_i, DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0);
	/*
	if (IOM0.DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0
			[48]  DP_DLL_CAL_GOOD ==        1
			[49]  DP_DLL_CAL_ERROR ==       0
			[50]  DP_DLL_CAL_ERROR_FINE ==  0
			[51]  ADR_DLL_CAL_GOOD ==       1
			[52]  ADR_DLL_CAL_ERROR ==      0
			[53]  ADR_DLL_CAL_ERROR_FINE == 0) break    // success
	if (IOM0.DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0
			[49]  DP_DLL_CAL_ERROR ==       1 |
			[50]  DP_DLL_CAL_ERROR_FINE ==  1 |
			[52]  ADR_DLL_CAL_ERROR ==      1 |
			[53]  ADR_DLL_CAL_ERROR_FINE == 1) break and do the workaround
	*/
	if ((status & PPC_BITMASK(48, 53)) ==
	    (PPC_BIT(DP_DLL_CAL_GOOD) | PPC_BIT(ADR_DLL_CAL_GOOD))) {
		/* DLL calibration finished without errors */
		return 1;
	}

	if (status & (PPC_BIT(DP_DLL_CAL_ERROR) | PPC_BIT(DP_DLL_CAL_ERROR_FINE) |
	              PPC_BIT(ADR_DLL_CAL_ERROR) | PPC_BIT(ADR_DLL_CAL_ERROR_FINE))) {
		/* DLL calibration finished, but with errors */
		*do_workaround = true;
		return 1;
	}

	/* Not done yet */
	return 0;
}

static int test_bb_lock(int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	uint64_t res = PPC_BIT(BB_LOCK0) | PPC_BIT(BB_LOCK1);
	int mca_i, dp;

	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		if (!mem_data.mcs[mcs_i].mca[mca_i].functional)
			continue;

		/*
		IOM0.DDRPHY_ADR_SYSCLK_PR_VALUE_RO_P0_ADR32S{0,1}
			  [56]  BB_LOCK     &
		IOM0.DDRPHY_DP16_SYSCLK_PR_VALUE_P0_{0,1,2,3}
			  [48]  BB_LOCK0    &
			  [56]  BB_LOCK1    &
		IOM0.DDRPHY_DP16_SYSCLK_PR_VALUE_P0_4
			  [48]  BB_LOCK0          // last DP16 uses only first half
		if all bits listed above are set: success
		*/

		/* ADR_SYSCLK_PR_VALUE_RO_P0_ADR32S{0,1}, BB_LOCK0 doesn't matter */
		res &= dp_mca_read(id, 0, mca_i, ADR_SYSCLK_PR_VALUE_RO_P0_ADR32S0) |
		       PPC_BIT(BB_LOCK0);
		res &= dp_mca_read(id, 1, mca_i, ADR_SYSCLK_PR_VALUE_RO_P0_ADR32S0) |
		       PPC_BIT(BB_LOCK0);

		/* IOM0.DDRPHY_DP16_SYSCLK_PR_VALUE_P0_{0,1,2,3} */
		for (dp = 0; dp < 4; dp++) {
			res &= dp_mca_read(id, dp, mca_i, DDRPHY_DP16_SYSCLK_PR_VALUE_P0_0);
		}

		/* IOM0.DDRPHY_DP16_SYSCLK_PR_VALUE_P0_4, BB_LOCK1 doesn't matter */
		res &= dp_mca_read(id, dp, mca_i, DDRPHY_DP16_SYSCLK_PR_VALUE_P0_0) |
		       PPC_BIT(BB_LOCK1);

		/* Do we want early return here? */
	}

	return res == (PPC_BIT(BB_LOCK0) | PPC_BIT(BB_LOCK1));
}

static void fix_bad_voltage_settings(int mcs_i)
{
	die("fix_bad_voltage_settings() required for MCS%d, but not implemented yet\n", mcs_i);

	/* TODO: implement if needed */
/*
  for each functional MCA
	// Each MCA has 10 DLLs: ADR DLL0, DP0-4 DLL0, DP0-3 DLL1. Each of those can fail. For each DLL there are 5 registers
	// used in this workaround, those are (see src/import/chips/p9/procedures/hwp/memory/lib/workarounds/dll_workaround.C):
	// - l_CNTRL:         DP16 or ADR CNTRL register
	// - l_COARSE_SAME:   VREG_COARSE register for same DLL as CNTRL reg
	// - l_COARSE_NEIGH:  VREG_COARSE register for DLL neighbor for this workaround
	// - l_DAC_LOWER:     DLL DAC Lower register
	// - l_DAC_UPPER:     DLL DAC Upper register
	// Warning: the last two have their descriptions swapped in dll_workaround.H
	// It seems that the code excepts that DLL neighbor is always good, what if it isn't?
	//
	// General flow, stripped from C++ bloating and repeated loops:
	for each DLL          // list in workarounds/dll_workaround.C
	  1. check if this DLL failed, if not - skip to the next one
			(l_CNTRL[62 | 63] | l_COARSE_SAME[56-62] == 1) -> failed
	  2. set reset bit, set skip VREG bit, clear the error bits
			l_CNTRL[48] =     1
			l_CNTRL[50-51] =  2     // REGS_RXDLL_CAL_SKIP, 2 - skip VREG calib., do coarse delay calib. only
			l_CNTRL[62-63] =  0
	  3. clear DLL FIR (see "Do FIRry things" at the end of 13.8)  // this was actually done for non-failed DLLs too, why?
			IOM0.IOM_PHY0_DDRPHY_FIR_REG =      // 0x07011000         // maybe use SCOM1 (AND) 0x07011001
				  [56]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2 = 0   // calibration errors
				  [58]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_4 = 0   // DLL errors
	  4. write the VREG DAC value found in neighbor (good) to the failing DLL VREG DAC
			l_COARSE_SAME[56-62] = l_COARSE_NEIGH[56-62]
	  5. reset the upper and lower fine calibration bits back to defaults
			l_DAC_LOWER[56-63] =  0x0x8000    // Hard coded default values per Steve Wyatt for this workaround
			l_DAC_UPPER[56-63] =  0x0xFFE0
	  6. run DLL Calibration again on failed DLLs
			l_CNTRL[48] = 0
	// Wait for calibration to finish
	delay(37,382 memclocks)     // again, we could do better than this

	// Check if calibration succeeded (same tests as in 1 above, for all DLLs)
	for each DLL
	  if (l_CNTRL[62 | 63] | l_COARSE_SAME[56-62] == 1): failed, assert and die?
*/
}

static void check_during_phy_reset(int mcs_i)
{
	/*
	 * Mostly FFDC, which to my current knowledge is just the error logging. If
	 * it does anything else, this whole function needs rechecking.
	 */
	chiplet_id_t id = mcs_ids[mcs_i];
	int mca_i;
	uint64_t val;

	/* If any of these bits is set, report error. Clear them unconditionally. */
	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		if (mca_i != 0 && !mem_data.mcs[mcs_i].mca[mca_i].functional)
			continue;

		/* MC01.PORT0.SRQ.MBACALFIR
			  [0]   MBACALFIR_MBA_RECOVERABLE_ERROR
			  [1]   MBACALFIR_MBA_NONRECOVERABLE_ERROR
			  [10]  MBACALFIR_SM_1HOT_ERR
		*/
		val = mca_read(id, mca_i, MBACALFIR);
		if (val & (PPC_BIT(MBACALFIR_MBA_RECOVERABLE_ERROR) |
		           PPC_BIT(MBACALFIR_MBA_NONRECOVERABLE_ERROR) |
		           PPC_BIT(MBACALFIR_SM_1HOT_ERR))) {
			/* No idea how severe that error is... */
			printk(BIOS_ERR, "Error detected in PORT%d.SRQ.MBACALFIR: %#llx\n",
			       mca_i, val);
		}

		mca_and_or(id, mca_i, MBACALFIR,
		           ~(PPC_BIT(MBACALFIR_MBA_RECOVERABLE_ERROR) |
		             PPC_BIT(MBACALFIR_MBA_NONRECOVERABLE_ERROR) |
		             PPC_BIT(MBACALFIR_SM_1HOT_ERR)),
		           0);

		/* IOM0.IOM_PHY0_DDRPHY_FIR_REG
			  [54]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_0
			  [55]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_1
			  [56]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2
			  [57]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_3
			  [58]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_4
			  [59]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_5
			  [60]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_6
			  [61]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_7
		*/
		val = mca_read(id, mca_i, IOM_PHY0_DDRPHY_FIR_REG);
		if (val & PPC_BITMASK(54, 61)) {
			/* No idea how severe that error is... */
			printk(BIOS_ERR, "Error detected in IOM_PHY%d_DDRPHY_FIR_REG: %#llx\n",
				   mca_i , val);
		}

		mca_and_or(id, mca_i, IOM_PHY0_DDRPHY_FIR_REG, ~(PPC_BITMASK(54, 61)), 0);
	}
}

static void fir_unmask(int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int mca_i;

	/*
	 * MASK must be always written last, otherwise we may get unintended
	 * actions. No need for explicit barrier here, SCOM functions do eieio.
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRACT0
		[2]   MCBISTFIRQ_INTERNAL_FSM_ERROR =       0
		[13]  MCBISTFIRQ_SCOM_RECOVERABLE_REG_PE =  0
		[14]  MCBISTFIRQ_SCOM_FATAL_REG_PE =        0
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRACT1
		[2]   MCBISTFIRQ_INTERNAL_FSM_ERROR =       0
		[13]  MCBISTFIRQ_SCOM_RECOVERABLE_REG_PE =  1
		[14]  MCBISTFIRQ_SCOM_FATAL_REG_PE =        0
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRMASK
		[2]   MCBISTFIRQ_INTERNAL_FSM_ERROR =       0   // checkstop (0,0,0)
		[13]  MCBISTFIRQ_SCOM_RECOVERABLE_REG_PE =  0   // recoverable_error (0,1,0)
		[14]  MCBISTFIRQ_SCOM_FATAL_REG_PE =        0   // checkstop (0,0,0)
	*/
	scom_and_or_for_chiplet(id, MCBISTFIRACT0,
	                        ~(PPC_BIT(MCBISTFIRQ_INTERNAL_FSM_ERROR) |
	                          PPC_BIT(MCBISTFIRQ_SCOM_RECOVERABLE_REG_PE) |
	                          PPC_BIT(MCBISTFIRQ_SCOM_FATAL_REG_PE)),
	                        0);
	scom_and_or_for_chiplet(id, MCBISTFIRACT1,
	                        ~(PPC_BIT(MCBISTFIRQ_INTERNAL_FSM_ERROR) |
	                          PPC_BIT(MCBISTFIRQ_SCOM_RECOVERABLE_REG_PE) |
	                          PPC_BIT(MCBISTFIRQ_SCOM_FATAL_REG_PE)),
	                        PPC_BIT(MCBISTFIRQ_SCOM_RECOVERABLE_REG_PE));
	scom_and_or_for_chiplet(id, MCBISTFIRMASK,
	                        ~(PPC_BIT(MCBISTFIRQ_INTERNAL_FSM_ERROR) |
	                          PPC_BIT(MCBISTFIRQ_SCOM_RECOVERABLE_REG_PE) |
	                          PPC_BIT(MCBISTFIRQ_SCOM_FATAL_REG_PE)),
	                        0);

	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		if (!mem_data.mcs[mcs_i].mca[mca_i].functional)
			continue;

		/*
		MC01.PORT0.SRQ.MBACALFIR_ACTION0
			  [0]   MBACALFIR_MBA_RECOVERABLE_ERROR =    0
			  [1]   MBACALFIR_MBA_NONRECOVERABLE_ERROR = 0
			  [4]   MBACALFIR_RCD_PARITY_ERROR =         0
			  [10]  MBACALFIR_SM_1HOT_ERR =              0
		MC01.PORT0.SRQ.MBACALFIR_ACTION1
			  [0]   MBACALFIR_MBA_RECOVERABLE_ERROR =    1
			  [1]   MBACALFIR_MBA_NONRECOVERABLE_ERROR = 0
			  [4]   MBACALFIR_RCD_PARITY_ERROR =         1
			  [10]  MBACALFIR_SM_1HOT_ERR =              0
		MC01.PORT0.SRQ.MBACALFIR_MASK
			  [0]   MBACALFIR_MBA_RECOVERABLE_ERROR =    0   // recoverable_error (0,1,0)
			  [1]   MBACALFIR_MBA_NONRECOVERABLE_ERROR = 0   // checkstop (0,0,0)
			  [4]   MBACALFIR_RCD_PARITY_ERROR =         0   // recoverable_error (0,1,0)
			  [10]  MBACALFIR_SM_1HOT_ERR =              0   // checkstop (0,0,0)
		*/
		mca_and_or(id, mca_i, MBACALFIR_ACTION0,
		           ~(PPC_BIT(MBACALFIR_MBA_RECOVERABLE_ERROR) |
		             PPC_BIT(MBACALFIR_MBA_NONRECOVERABLE_ERROR) |
		             PPC_BIT(MBACALFIR_RCD_PARITY_ERROR) |
		             PPC_BIT(MBACALFIR_SM_1HOT_ERR)),
		           0);
		mca_and_or(id, mca_i, MBACALFIR_ACTION1,
		           ~(PPC_BIT(MBACALFIR_MBA_RECOVERABLE_ERROR) |
		             PPC_BIT(MBACALFIR_MBA_NONRECOVERABLE_ERROR) |
		             PPC_BIT(MBACALFIR_RCD_PARITY_ERROR) |
		             PPC_BIT(MBACALFIR_SM_1HOT_ERR)),
		           PPC_BIT(MBACALFIR_MBA_RECOVERABLE_ERROR) |
		           PPC_BIT(MBACALFIR_RCD_PARITY_ERROR));
		mca_and_or(id, mca_i, MBACALFIR_MASK,
		           ~(PPC_BIT(MBACALFIR_MBA_RECOVERABLE_ERROR) |
		             PPC_BIT(MBACALFIR_MBA_NONRECOVERABLE_ERROR) |
		             PPC_BIT(MBACALFIR_RCD_PARITY_ERROR) |
		             PPC_BIT(MBACALFIR_SM_1HOT_ERR)),
		           0);

		/*
		IOM0.IOM_PHY0_DDRPHY_FIR_ACTION0_REG
			  [54]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_0 = 0
			  [55]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_1 = 0
			  [57]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_3 = 0   // no ERROR_2!
			  [58]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_4 = 0
			  [59]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_5 = 0
			  [60]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_6 = 0
			  [61]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_7 = 0
		IOM0.IOM_PHY0_DDRPHY_FIR_ACTION1_REG
			  [54]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_0 = 1
			  [55]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_1 = 1
			  [57]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_3 = 1
			  [58]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_4 = 1
			  [59]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_5 = 1
			  [60]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_6 = 1
			  [61]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_7 = 1
		IOM0.IOM_PHY0_DDRPHY_FIR_MASK_REG
			  [54]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_0 = 0   // recoverable_error (0,1,0)
			  [55]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_1 = 0   // recoverable_error (0,1,0)
			  [57]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_3 = 0   // recoverable_error (0,1,0)
			  [58]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_4 = 0   // recoverable_error (0,1,0)
			  [59]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_5 = 0   // recoverable_error (0,1,0)
			  [60]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_6 = 0   // recoverable_error (0,1,0)
			  [61]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_7 = 0   // recoverable_error (0,1,0)
		*/
		mca_and_or(id, mca_i, IOM_PHY0_DDRPHY_FIR_ACTION0_REG,
		           ~(PPC_BITMASK(54, 55) | PPC_BITMASK(57, 61)),
		           0);
		mca_and_or(id, mca_i, IOM_PHY0_DDRPHY_FIR_ACTION1_REG,
		           ~(PPC_BITMASK(54, 55) | PPC_BITMASK(57, 61)),
		           PPC_BITMASK(54, 55) | PPC_BITMASK(57, 61));
		mca_and_or(id, mca_i, IOM_PHY0_DDRPHY_FIR_MASK_REG,
		           ~(PPC_BITMASK(54, 55) | PPC_BITMASK(57, 61)),
		           0);
	}
}

/*
 * Can't protect with do..while, this macro is supposed to exit 'for' loop in
 * which it is invoked. As a side effect, it is used without semicolon.
 *
 * "I want to break free" - Freddie Mercury
 */
#define TEST_VREF(dp, scom) \
if ((dp_mca_read(mcs_ids[mcs_i], dp, mca_i, scom) & PPC_BITMASK(56, 62)) == \
             PPC_SHIFT(1,62)) { \
	need_dll_workaround = true; \
	break; \
}

/*
 * 13.9 mss_ddr_phy_reset: Soft reset of DDR PHY macros
 *
 * - Lock DDR DLLs
 *   - Already configured DDR DLL in scaninit
 * - Sends Soft DDR Phy reset
 * - Kick off internal ZQ Cal
 * - Perform any config that wasn't scanned in (TBD)
 *   - Nothing known here
 */
void istep_13_9(void)
{
	printk(BIOS_EMERG, "starting istep 13.9\n");
	int mcs_i, mca_i, dp;
	long time;
	bool need_dll_workaround;

	report_istep(13,9);

	/*
	 * Most of this istep consists of:
	 * 1. asserting reset bit or starting calibration
	 * 2. delay
	 * 3. deasserting reset bit or checking the result of calibration
	 *
	 * These are done for each (functional and/or magic) MCA. Because the delay
	 * is required between points 1 and 3 for a given MCA, those delays are done
	 * outside of 'for each MCA' loops. They are still inside 'for each MCS'
	 * loop, unclear if we can break it into pieces too.
	 */
	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (mca_i != 0 && !mca->functional)
				continue;

			/* MC01.PORT0.SRQ.MBA_FARB5Q =
				[8]     MBA_FARB5Q_CFG_FORCE_MCLK_LOW_N = 0
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, MBA_FARB5Q,
			           ~PPC_BIT(MBA_FARB5Q_CFG_FORCE_MCLK_LOW_N), 0);

			/* Drive all control signals to their inactive/idle state, or
			 * inactive value
			IOM0.DDRPHY_DP16_SYSCLK_PR0_P0_{0,1,2,3,4} =
			IOM0.DDRPHY_DP16_SYSCLK_PR1_P0_{0,1,2,3,4} =
				[all]   0
				[48]    reserved = 1            // MCA_DDRPHY_DP16_SYSCLK_PR0_P0_0_01_ENABLE
			*/
			for (dp = 0; dp < 5; dp++) {
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, DDRPHY_DP16_SYSCLK_PR0_P0_0,
				              0, PPC_BIT(48));
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, DDRPHY_DP16_SYSCLK_PR1_P0_0,
				              0, PPC_BIT(48));
			}

			/* Assert reset to PHY for 32 memory clocks
			MC01.PORT0.SRQ.MBA_CAL0Q =
				[57]    MBA_CAL0Q_RESET_RECOVER = 1
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, MBA_CAL0Q, ~0,
			           PPC_BIT(MBA_CAL0Q_RESET_RECOVER));
		}

		delay_nck(32);

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (mca_i != 0 && !mca->functional)
				continue;

			/* Deassert reset_n
			MC01.PORT0.SRQ.MBA_CAL0Q =
					[57]    MBA_CAL0Q_RESET_RECOVER = 0
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, MBA_CAL0Q,
			           ~PPC_BIT(MBA_CAL0Q_RESET_RECOVER), 0);

			/* Flush output drivers
			IOM0.DDRPHY_ADR_OUTPUT_FORCE_ATEST_CNTL_P0_ADR32S{0,1} =
					[all]   0
					[48]    FLUSH =   1
					[50]    INIT_IO = 1
			*/
			/* Has the same stride as DP16 */
			dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i,
			              DDRPHY_ADR_OUTPUT_FORCE_ATEST_CNTL_P0_ADR32S0, 0,
			              PPC_BIT(FLUSH) | PPC_BIT(INIT_IO));
			dp_mca_and_or(mcs_ids[mcs_i], 1, mca_i,
			              DDRPHY_ADR_OUTPUT_FORCE_ATEST_CNTL_P0_ADR32S0, 0,
			              PPC_BIT(FLUSH) | PPC_BIT(INIT_IO));

			/* IOM0.DDRPHY_DP16_CONFIG0_P0_{0,1,2,3,4} =
					[all]   0
					[51]    FLUSH =                 1
					[54]    INIT_IO =               1
					[55]    ADVANCE_PING_PONG =     1
					[58]    DELAY_PING_PONG_HALF =  1
			*/
			for (dp = 0; dp < 5; dp++) {
				dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i, DDRPHY_DP16_CONFIG0_P0_0, 0,
				              PPC_BIT(DP16_CONFIG0_FLUSH) |
				              PPC_BIT(DP16_CONFIG0_INIT_IO) |
				              PPC_BIT(DP16_CONFIG0_ADVANCE_PING_PONG) |
				              PPC_BIT(DP16_CONFIG0_DELAY_PING_PONG_HALF));
			}
		}

		delay_nck(32);

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (mca_i != 0 && !mca->functional)
				continue;

			/* IOM0.DDRPHY_ADR_OUTPUT_FORCE_ATEST_CNTL_P0_ADR32S{0,1} =
					[all]   0
					[48]    FLUSH =   0
					[50]    INIT_IO = 0
			*/
			/* Has the same stride as DP16 */
			dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i,
			              DDRPHY_ADR_OUTPUT_FORCE_ATEST_CNTL_P0_ADR32S0, 0, 0);
			dp_mca_and_or(mcs_ids[mcs_i], 1, mca_i,
			              DDRPHY_ADR_OUTPUT_FORCE_ATEST_CNTL_P0_ADR32S0, 0, 0);

			/* IOM0.DDRPHY_DP16_CONFIG0_P0_{0,1,2,3,4} =
					[all]   0
					[51]    FLUSH =                 0
					[54]    INIT_IO =               0
					[55]    ADVANCE_PING_PONG =     1
					[58]    DELAY_PING_PONG_HALF =  1
			*/
			for (dp = 0; dp < 5; dp++) {
				dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i, DDRPHY_DP16_CONFIG0_P0_0, 0,
				              PPC_BIT(DP16_CONFIG0_ADVANCE_PING_PONG) |
				              PPC_BIT(DP16_CONFIG0_DELAY_PING_PONG_HALF));
			}
		}

		/* ZCTL Enable */
		/*
		 * In Hostboot this is 'for each magic MCA'. We know there is only one
		 * magic, and it has always the same index.
		IOM0.DDRPHY_PC_RESETS_P0 =
			// Yet another documentation error: all bits in this register are marked as read-only
			[51]    ENABLE_ZCAL = 1
		 */
		mca_and_or(mcs_ids[mcs_i], 0, DDRPHY_PC_RESETS_P0, ~0, PPC_BIT(ENABLE_ZCAL));

		/* Maybe it would be better to add another 1us later instead of this. */
		delay_nck(1024);

		/* for each magic MCA */
		/* 50*10ns, but we don't have such precision. */
		time = wait_us(1, mca_read(mcs_ids[mcs_i], 0,
		               DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0) & PPC_BIT(ZCAL_DONE));

		if (!time)
			die("ZQ calibration timeout\n");

		/* DLL calibration */
		/*
		 * Here was an early return if no functional MCAs were found. Wouldn't
		 * that make whole MCBIST non-functional?
		 */
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			/* IOM0.DDRPHY_ADR_DLL_CNTL_P0_ADR32S{0,1} =
				[48]    INIT_RXDLL_CAL_RESET = 0
			*/
			/* Has the same stride as DP16 */
			dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i, DDRPHY_ADR_DLL_CNTL_P0_ADR32S0,
			              ~PPC_BIT(INIT_RXDLL_CAL_RESET), 0);
			dp_mca_and_or(mcs_ids[mcs_i], 1, mca_i, DDRPHY_ADR_DLL_CNTL_P0_ADR32S0,
			              ~PPC_BIT(INIT_RXDLL_CAL_RESET), 0);

			for (dp = 0; dp < 4; dp++) {
				/* IOM0.DDRPHY_DP16_DLL_CNTL{0,1}_P0_{0,1,2,3} =
					[48]    INIT_RXDLL_CAL_RESET = 0
				*/
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, DDRPHY_DP16_DLL_CNTL0_P0_0,
				              ~PPC_BIT(INIT_RXDLL_CAL_RESET), 0);
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, DDRPHY_DP16_DLL_CNTL1_P0_0,
				              ~PPC_BIT(INIT_RXDLL_CAL_RESET), 0);
			}
			/* Last DP16 is different
			IOM0.DDRPHY_DP16_DLL_CNTL0_P0_4
				[48]    INIT_RXDLL_CAL_RESET = 0
			IOM0.DDRPHY_DP16_DLL_CNTL1_P0_4
				[48]    INIT_RXDLL_CAL_RESET = 1
			*/
			dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, DDRPHY_DP16_DLL_CNTL0_P0_0,
				              ~PPC_BIT(INIT_RXDLL_CAL_RESET), 0);
			dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, DDRPHY_DP16_DLL_CNTL1_P0_0,
				              ~0, PPC_BIT(INIT_RXDLL_CAL_RESET));
		}

		/* From Hostboot's comments:
		 * 32,772 dphy_nclk cycles from Reset=0 to VREG Calibration to exhaust all values
		 * 37,382 dphy_nclk cycles for full calibration to start and fail ("worst case")
		 *
		 * Why assume worst case instead of making the next timeout bigger?
		 */
		delay_nck(37382);

		/*
		 * The comment before poll says:
		 * > To keep things simple, we'll poll for the change in one of the ports.
		 * > Once that's completed, we'll check the others. If any one has failed,
		 * > or isn't notifying complete, we'll pop out an error
		 *
		 * The issue is that it only tests the first of the functional ports.
		 * Other ports may or may not have failed. Even if this times out, the
		 * rest of the function continues normally, without throwing any error...
		 *
		 * For now, leave it as it was done in Hostboot.
		 */
		/* timeout(50*10ns):
		  if (IOM0.DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0
					[48]  DP_DLL_CAL_GOOD ==        1
					[49]  DP_DLL_CAL_ERROR ==       0
					[50]  DP_DLL_CAL_ERROR_FINE ==  0
					[51]  ADR_DLL_CAL_GOOD ==       1
					[52]  ADR_DLL_CAL_ERROR ==      0
					[53]  ADR_DLL_CAL_ERROR_FINE == 0) break    // success
		  if (IOM0.DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0
					[49]  DP_DLL_CAL_ERROR ==       1 |
					[50]  DP_DLL_CAL_ERROR_FINE ==  1 |
					[52]  ADR_DLL_CAL_ERROR ==      1 |
					[53]  ADR_DLL_CAL_ERROR_FINE == 1) break and do the workaround
		*/
		need_dll_workaround = false;
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			if (mem_data.mcs[mcs_i].mca[mca_i].functional)
				break;
		}
		/* 50*10ns, but we don't have such precision. */
		time = wait_us(1, test_dll_calib_done(mcs_i, mca_i, &need_dll_workaround));
		if (!time)
			die("DLL calibration timeout\n");

		/*
		 * Workaround is also required if any of coarse VREG has value 1 after
		 * calibration. Test from poll above is repeated here - this time for every
		 * MCA, but it doesn't wait until DLL gets calibrated if that is still in
		 * progress. The registers below (also used in the workaround) __must not__
		 * be written to while hardware calibration is in progress.
		 */
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (need_dll_workaround)
				break;

			if (!mca->functional)
				continue;

			/*
			 * This assumes that by the time the first functional MCA completed
			 * successfully, all MCAs completed (with or without errors). If the
			 * first MCA failed then we won't even get here, we would bail earlier
			 * because need_dll_workaround == true in that case.
			 *
			 * This is not safe if DLL calibration takes more time for other MCAs,
			 * but this is the way Hostboot does it.
			 */
			test_dll_calib_done(mcs_i, mca_i, &need_dll_workaround);

			/*
			if (IOM0.DDRPHY_ADR_DLL_VREG_COARSE_P0_ADR32S0        |
			  IOM0.DDRPHY_DP16_DLL_VREG_COARSE0_P0_{0,1,2,3,4}  |
			  IOM0.DDRPHY_DP16_DLL_VREG_COARSE1_P0_{0,1,2,3}    |
					[56-62] REGS_RXDLL_VREG_DAC_COARSE = 1)    // The same offset for ADR and DP16
				  do the workaround
			*/
			TEST_VREF(0, DDRPHY_ADR_DLL_VREG_COARSE_P0_ADR32S0)
			TEST_VREF(4, DDRPHY_DP16_DLL_VREG_COARSE0_P0_0)
			for (dp = 0; dp < 4; dp++) {
				TEST_VREF(dp, DDRPHY_DP16_DLL_VREG_COARSE0_P0_0)
				TEST_VREF(dp, DDRPHY_DP16_DLL_VREG_COARSE1_P0_0)
			}
		}

		if (need_dll_workaround)
			fix_bad_voltage_settings(mcs_i);

		/* Start bang-bang-lock */
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			/* Take dphy_nclk/SysClk alignment circuits out of reset and put into
			 * continuous update mode
			IOM0.DDRPHY_ADR_SYSCLK_CNTL_PR_P0_ADR32S{0,1} =
			IOM0.DDRPHY_DP16_SYSCLK_PR0_P0_{0,1,2,3,4} =
			IOM0.DDRPHY_DP16_SYSCLK_PR1_P0_{0,1,2,3} =
					[all]   0
					[48-63] 0x8024      // From the DDR PHY workbook
			*/
			/* Has the same stride as DP16 */
			dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i,
			              ADR_SYSCLK_CNTRL_PR_P0_ADR32S0,
			              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8024, 63));
			dp_mca_and_or(mcs_ids[mcs_i], 1, mca_i,
			              ADR_SYSCLK_CNTRL_PR_P0_ADR32S0,
			              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8024, 63));

			for (dp = 0; dp < 4; dp++) {
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i,
				              DDRPHY_DP16_SYSCLK_PR0_P0_0,
				              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8024, 63));
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i,
				              DDRPHY_DP16_SYSCLK_PR1_P0_0,
				              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8024, 63));
			}
			dp_mca_and_or(mcs_ids[mcs_i], 4, mca_i,
						  DDRPHY_DP16_SYSCLK_PR0_P0_0,
						  ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8024, 63));
		}

		/*
		 * Wait at least 5932 dphy_nclk clock cycles to allow the dphy_nclk/SysClk
		 * alignment circuit to perform initial alignment.
		 */
		delay_nck(5932);

		/* Check for LOCK in {DP16,ADR}_SYSCLK_PR_VALUE */
		/* 50*10ns, but we don't have such precision. */
		/*
		 * FIXME: Hostboot uses the timeout mentioned above for each of
		 * the registers separately. It also checks them separately,
		 * meaning that they don't have to be locked at the same time.
		 * I am not sure if this is why the call below times out or if
		 * there is another reason. Can these locks be lost or should
		 * they hold until reset?
		 *
		 * Increasing the timeout helps (maybe that's just luck), but
		 * this probably isn't a proper way to do this.
		 */
		time = wait_ms(1000, test_bb_lock(mcs_i));
		if (!time)
			die("BB lock timeout\n");

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			/* De-assert the SYSCLK_RESET
			IOM0.DDRPHY_PC_RESETS_P0 =
				  [49]  SYSCLK_RESET = 0
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, DDRPHY_PC_RESETS_P0,
			           ~PPC_BIT(SYSCLK_RESET), 0);

			/* Reset the windage registers */
			/*
			 * According to the PHY team, resetting the read delay offset must be
			 * done after SYSCLK_RESET.
			 *
			 * ATTR_MSS_VPD_MT_WINDAGE_RD_CTR holds (signed) value of offset in
			 * picoseconds. It must be converted to phase rotator ticks. There are
			 * 128 ticks per clock, and clock period depends on memory frequency.
			 *
			 * Result is rounded away from zero, so we have to add _or subtract_
			 * half of tick.
			 *
			 * In some cases we can skip this (40 register writes per port), from
			 * documentation:
			 *
			 * "This register must not be set to a nonzero value unless detailed
			 * timing analysis shows that, for a particular configuration, the
			 * read-centering algorithm places the sampling point off from the eye
			 * center."
			 *
			 * ATTR_MSS_VPD_MT_WINDAGE_RD_CTR is outside of defined values for VPD
			 * for Talos, it is by default set to 0. Skipping this for now, but it
			 * may be needed for generalized code.
			 *
			           // 0x80000{0,1,2,3}0C0701103F, +0x0400_0000_0000
			IOM0.DDRPHY_DP16_READ_DELAY_OFFSET0_RANK_PAIR{0,1,2,3}_P0_{0,1,2,3,4} =
			           // 0x80000{0,1,2,3}0D0701103F, +0x0400_0000_0000
			IOM0.DDRPHY_DP16_READ_DELAY_OFFSET1_RANK_PAIR{0,1,2,3}_P0_{0,1,2,3,4} =
				  [all]   0
				  [49-55] OFFSET0 = offset_in_ticks_rounded
				  [57-63] OFFSET1 = offset_in_ticks_rounded
			*/

			/*
			 * Take the dphy_nclk/SysClk alignment circuit out of the Continuous
			 * Update mode
			IOM0.DDRPHY_ADR_SYSCLK_CNTL_PR_P0_ADR32S{0,1} =           // 0x800080320701103F, +0x0400_0000_0000
			IOM0.DDRPHY_DP16_SYSCLK_PR0_P0_{0,1,2,3,4} =              // 0x800000070701103F, +0x0400_0000_0000
			IOM0.DDRPHY_DP16_SYSCLK_PR1_P0_{0,1,2,3} =                // 0x8000007F0701103F, +0x0400_0000_0000
				  [all]   0
				  [48-63] 0x8020      // From the DDR PHY workbook
			*/
			/* Has the same stride as DP16 */
			dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i,
			              ADR_SYSCLK_CNTRL_PR_P0_ADR32S0,
			              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8020, 63));
			dp_mca_and_or(mcs_ids[mcs_i], 1, mca_i,
			              ADR_SYSCLK_CNTRL_PR_P0_ADR32S0,
			              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8020, 63));

			for (dp = 0; dp < 4; dp++) {
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i,
				              DDRPHY_DP16_SYSCLK_PR0_P0_0,
				              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8020, 63));
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i,
				              DDRPHY_DP16_SYSCLK_PR1_P0_0,
				              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8020, 63));
			}
			dp_mca_and_or(mcs_ids[mcs_i], 4, mca_i,
						  DDRPHY_DP16_SYSCLK_PR0_P0_0,
						  ~PPC_BITMASK(48, 63), PPC_SHIFT(0x8020, 63));
		}

		/* Wait at least 32 dphy_nclk clock cycles */
		delay_nck(32);
		/* Done bang-bang-lock */

		/* Per J. Bialas, force_mclk_low can be dasserted */
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			/* MC01.PORT0.SRQ.MBA_FARB5Q =
					[8]     MBA_FARB5Q_CFG_FORCE_MCLK_LOW_N = 1
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, MBA_FARB5Q, ~0,
			           PPC_BIT(MBA_FARB5Q_CFG_FORCE_MCLK_LOW_N));

		}

		/* Workarounds */
		/*
		 * Does not apply to DD2, but even then reads and writes back some
		 * registers without modifications.
		 */
		// mss::workarounds::dp16::after_phy_reset();

		/*
		 * Comments from Hostboot:
		 *
		 * "New for Nimbus - perform duty cycle clock distortion calibration
		 * (DCD cal).
		 * Per PHY team's characterization, the DCD cal needs to be run after DLL
		 * calibration."
		 *
		 * However, it can be skipped based on ATTR_MSS_RUN_DCD_CALIBRATION,
		 * and by default it is skipped.
		 */
		// mss::adr32s::duty_cycle_distortion_calibration();

		/* FIR */
		check_during_phy_reset(mcs_i);
		fir_unmask(mcs_i);
	}

	printk(BIOS_EMERG, "ending istep 13.9\n");
}
