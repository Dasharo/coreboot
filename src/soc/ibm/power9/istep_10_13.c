/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_10.h>

#include <console/console.h>
#include <cpu/power/scom.h>
#include <cpu/power/proc.h>

/*
 * 10.13 host_rng_bist: Trigger Built In Self Test for RNG
 *
 * a) p9_rng_init_phase1.C
 *    - Trigger the Random Number Generator Built In Self Test (BIST). Results
 *      are checked later in step 16 when RNG is secured
 */

static void host_rng_bist(uint8_t chip)
{
	/* Assume DD2.0 or newer */

	/* PU_NX_RNG_CFG
	    [44]    COND_STARTUP_TEST_FAIL
	 */
	if (read_scom_for_chiplet(chip, N0_CHIPLET_ID, 0x020110E0) & PPC_BIT(44))
		die("RNG Conditioner startup test failed\n");

	/* PU_NX_RNG_ST0
	    [0-1]   REPTEST_MATCH_TH           = 0x1 (3 repeated numbers)
	    [7-8]   ADAPTEST_SAMPLE_SIZE       = 0x2 (8b wide sample)
	    [9-11]  ADAPTEST_WINDOW_SIZE       = 0x1 (512 size)
	    [12-23] ADAPTEST_RRN_RNG0_MATCH_TH = 0x32 (50; Assuming H = 5)
	    [24-35] ADAPTEST_RRN_RNG1_MATCH_TH = 0x32 (50; Assuming H = 5)
	    [36-47] ADAPTEST_CRN_RNG0_MATCH_TH = 0x32 (50; Assuming H = 5)
	    [48-59] ADAPTEST_CRN_RNG1_MATCH_TH = 0x32 (50; Assuming H = 5)
	 */
	scom_and_or_for_chiplet(chip, N0_CHIPLET_ID, 0x020110E1,
	                        ~(PPC_BITMASK(0, 1) | PPC_BITMASK(7, 63)),
	                        PPC_PLACE(1, 0, 2) | PPC_PLACE(2, 7, 2) | PPC_PLACE(1, 9, 3)
	                        | PPC_PLACE(0x32, 12, 12) | PPC_PLACE(0x32, 24, 12)
	                        | PPC_PLACE(0x32, 36, 12) | PPC_PLACE(0x32, 48, 12));

	/* PU_NX_RNG_ST1
	    [0-6]   ADAPTEST_SOFT_FAIL_TH      =   2
	    [7-22]  ADAPTEST_1BIT_MATCH_TH_MIN = 100
	    [23-38] ADAPTEST_1BIT_MATCH_TH_MAX = 415
	 */
	scom_and_or_for_chiplet(chip, N0_CHIPLET_ID, 0x020110E2, ~PPC_BITMASK(0, 38),
	                        PPC_PLACE(2, 0, 7) | PPC_PLACE(100, 7, 16)
	                        | PPC_PLACE(415, 23, 16));

	/* PU_NX_RNG_ST3
	    [0]     SAMPTEST_RRN_ENABLE   = 1
	    [1-3]   SAMPTEST_WINDOW_SIZE  = 7 (64k -1 size)
	    [4-19]  SAMPTEST_MATCH_TH_MIN = 0x6D60 (28,000)
	    [20-35] SAMPTEST_MATCH_TH_MAX = 0x988A (39,050)
	 */
	scom_and_or_for_chiplet(chip, N0_CHIPLET_ID, 0x020110E8, ~PPC_BITMASK(0, 35),
	                        PPC_BIT(0) | PPC_PLACE(7, 1, 3) | PPC_PLACE(0x6D60, 4, 16)
	                        | PPC_PLACE(0x988A, 20, 16));

	/* PU_NX_RNG_RDELAY
	    [6]     LFSR_RESEED_EN = 1
	    [7-11]  READ_RTY_RATIO = 0x1D (1/16)
	 */
	scom_and_or_for_chiplet(chip, N0_CHIPLET_ID, 0x020110E5, ~PPC_BITMASK(6, 11),
	                        PPC_BIT(6) | PPC_PLACE(0x1D, 7, 5));

	/* PU_NX_RNG_CFG
	    [30-37] ST2_RESET_PERIOD     = 0x1B
	    [39]    MASK_TOGGLE_ENABLE   = 0
	    [40]    SAMPTEST_ENABLE      = 1
	    [41]    REPTEST_ENABLE       = 1
	    [42]    ADAPTEST_1BIT_ENABLE = 1
	    [43]    ADAPTEST_ENABLE      = 1
	    [46-61] PACE_RATE            = 0x07D0 (2000)
	    [63]    ENABLE               = 1
	 */
	scom_and_or_for_chiplet(chip, N0_CHIPLET_ID, 0x020110E0,
	                        ~(PPC_BITMASK(30, 37) | PPC_BITMASK(39, 43)
	                          | PPC_BITMASK(46, 61) | PPC_BIT(63)),
	                        PPC_PLACE(0x1B, 30, 8) | PPC_BIT(40) | PPC_BIT(41)
	                        | PPC_BIT(42) | PPC_BIT(43) | PPC_PLACE(0x07D0, 46, 16)
	                        | PPC_BIT(63));
}

void istep_10_13(uint8_t chips)
{
	uint8_t chip;

	printk(BIOS_EMERG, "starting istep 10.13\n");

	report_istep(10,13);

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			host_rng_bist(chip);
	}

	printk(BIOS_EMERG, "ending istep 10.13\n");
}
