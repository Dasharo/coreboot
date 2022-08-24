/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <cpu/power/mvpd.h>
#include <cpu/power/proc.h>
#include <console/console.h>
#include <timer.h>

#include "istep_13_scom.h"

/*
 * 13.6 mem_startclocks: Start clocks on MBA/MCAs
 *
 * a) p9_mem_startclocks.C (proc chip)
 *    - This step is a no-op on cumulus
 *    - This step is a no-op if memory is running in synchronous mode since the
 *      MCAs are using the nest PLL, HWP detect and exits
 *    - Drop fences and tholds on MBA/MCAs to start the functional clocks
 */

static inline void p9_mem_startclocks_cplt_ctrl_action_function(uint8_t chip, chiplet_id_t id,
								uint64_t pg)
{
	// Drop partial good fences
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL1 (WO_CLEAR)
	  [all]   0
	  [3]     TC_VITL_REGION_FENCE =                  ~ATTR_PG[3]
	  [4-14]  TC_REGION{1-3}_FENCE, UNUSED_{8-14}B =  ~ATTR_PG[4-14]
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_CPLT_CTRL1_WCLEAR, ~pg & PPC_BITMASK(3, 14));

	// Reset abistclk_muxsel and syncclk_muxsel
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_CLEAR)
	  [all]   0
	  [0]     CTRL_CC_ABSTCLK_MUXSEL_DC = 1
	  [1]     TC_UNIT_SYNCCLK_MUXSEL_DC = 1
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_CPLT_CTRL0_WCLEAR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_ABSTCLK_MUXSEL_DC) |
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_TC_UNIT_SYNCCLK_MUXSEL_DC));

}

static inline void p9_sbe_common_align_chiplets(uint8_t chip, chiplet_id_t id)
{
	// Exit flush
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_OR)
	  [all]   0
	  [2]     CTRL_CC_FLUSHMODE_INH_DC =  1
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_CPLT_CTRL0_WOR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FLUSHMODE_INH_DC));

	// Enable alignement
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_OR)
	  [all]   0
	  [3]     CTRL_CC_FORCE_ALIGN_DC =    1
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_CPLT_CTRL0_WOR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FORCE_ALIGN_DC));

	// Clear chiplet is aligned
	/*
	TP.TCMC01.MCSLOW.SYNC_CONFIG
	  [7]     CLEAR_CHIPLET_IS_ALIGNED =  1
	*/
	scom_or_for_chiplet(chip, id, MCSLOW_SYNC_CONFIG,
	                    PPC_BIT(MCSLOW_SYNC_CONFIG_CLEAR_CHIPLET_IS_ALIGNED));

	// Unset Clear chiplet is aligned
	/*
	TP.TCMC01.MCSLOW.SYNC_CONFIG
	  [7]     CLEAR_CHIPLET_IS_ALIGNED =  0
	*/
	scom_and_for_chiplet(chip, id, MCSLOW_SYNC_CONFIG,
	                     ~PPC_BIT(MCSLOW_SYNC_CONFIG_CLEAR_CHIPLET_IS_ALIGNED));

	udelay(100);

	// Poll aligned bit
	/*
	timeout(10*100us):
	TP.TCMC01.MCSLOW.CPLT_STAT0
	if (([9] CC_CTRL_CHIPLET_IS_ALIGNED_DC) == 1) break
	delay(100us)
	*/
	if (!wait_us(10 * 100, read_scom_for_chiplet(chip, id, MCSLOW_CPLT_STAT0) &
	             PPC_BIT(MCSLOW_CPLT_STAT0_CC_CTRL_CHIPLET_IS_ALIGNED_DC)))
		die("Timeout while waiting for chiplet alignment\n");

	// Disable alignment
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_CLEAR)
	  [all]   0
	  [3]     CTRL_CC_FORCE_ALIGN_DC =  1
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_CPLT_CTRL0_WCLEAR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FORCE_ALIGN_DC));
}

static void p9_sbe_common_clock_start_stop(uint8_t chip, chiplet_id_t id, uint64_t pg)
{
	// Chiplet exit flush
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_OR)
	  [all]   0
	  [2]     CTRL_CC_FLUSHMODE_INH_DC =  1
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_CPLT_CTRL0_WOR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FLUSHMODE_INH_DC));

	// Clear Scan region type register
	/*
	TP.TCMC01.MCSLOW.SCAN_REGION_TYPE
	  [all]   0
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_SCAN_REGION_TYPE, 0);

	// Setup all Clock Domains and Clock Types
	/*
	TP.TCMC01.MCSLOW.CLK_REGION
	  [0-1]   CLOCK_CMD =       1     // start
	  [2]     SLAVE_MODE =      0
	  [3]     MASTER_MODE =     0
	  [4-14]  CLOCK_REGION_* =  (((~ATTR_PG[4-14]) >> 1) & 0x07FE) << 1 =
	                            ~ATTR_PG[4-14] & 0x0FFC =
	                            ~ATTR_PG[4-13]       // Hostboot tends to complicate
	  [48]    SEL_THOLD_SL =    1
	  [49]    SEL_THOLD_NSL =   1
	  [50]    SEL_THOLD_ARY =   1
	*/
	scom_and_or_for_chiplet(chip, id, MCSLOW_CLK_REGION,
	                        ~(PPC_BITMASK(0, 14) | PPC_BITMASK(48, 50)),
	                        PPC_PLACE(1, MCSLOW_CLK_REGION_CLOCK_CMD,
	                                  MCSLOW_CLK_REGION_CLOCK_CMD_LEN) |
	                        PPC_BIT(MCSLOW_CLK_REGION_SEL_THOLD_SL) |
	                        PPC_BIT(MCSLOW_CLK_REGION_SEL_THOLD_NSL) |
	                        PPC_BIT(MCSLOW_CLK_REGION_SEL_THOLD_ARY) |
	                        (~pg & PPC_BITMASK(4, 13)));

	// Poll OPCG done bit to check for completeness
	/*
	timeout(10*100us):
	TP.TCMC01.MCSLOW.CPLT_STAT0
	if (([8] CC_CTRL_OPCG_DONE_DC) == 1) break
	delay(100us)
	*/
	if (!wait_us(10 * 100, read_scom_for_chiplet(chip, id, MCSLOW_CPLT_STAT0) &
	             PPC_BIT(MCSLOW_CPLT_STAT0_CC_CTRL_OPCG_DONE_DC)))
		die("Timeout while waiting for OPCG done bit\n");

	/*
	 * Here Hostboot calculates what is expected clock status, based on previous
	 * values and requested command. It is done by generic functions, but
	 * because we know exactly which clocks were to be started, we can test just
	 * for those.
	 */
	/*
	TP.TCMC01.MCSLOW.CLOCK_STAT_SL
	TP.TCMC01.MCSLOW.CLOCK_STAT_NSL
	TP.TCMC01.MCSLOW.CLOCK_STAT_ARY
	  assert(([4-14] & ATTR_PG[4-14]) == ATTR_PG[4-14])
	*/
	uint64_t mask = PPC_BITMASK(4, 13);
	uint64_t expected = pg & mask;
	if ((read_scom_for_chiplet(chip, id, MCSLOW_CLOCK_STAT_SL) & mask) != expected ||
	    (read_scom_for_chiplet(chip, id, MCSLOW_CLOCK_STAT_NSL) & mask) != expected ||
	    (read_scom_for_chiplet(chip, id, MCSLOW_CLOCK_STAT_ARY) & mask) != expected)
		die("Unexpected clock status\n");
}

static inline void p9_mem_startclocks_fence_setup_function(uint8_t chip, chiplet_id_t id)
{
	/*
	 * Hostboot does it based on pg_vector. It seems to check for Nest IDs to
	 * which MCs are connected, but I'm not sure if this is the case. I also
	 * don't know if it is possible to have a functional MCBIST for which we
	 * don't want to drop the fence (functional MCBIST with nonfunctional NEST?)
	 *
	 * Most likely this will need to be fixed for populated second MCS.
	 */

	/*
	 * if ((MC.ATTR_CHIP_UNIT_POS == 0x07 && pg_vector[5]) ||
	 *    (MC.ATTR_CHIP_UNIT_POS == 0x08 && pg_vector[3]))
	 *{
	 */

	// Drop chiplet fence
	/*
	TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WAND)
	  [all] 1
	  [18]  FENCE_EN =  0
	*/
	write_scom_for_chiplet(chip, id, PCBSLMC01_NET_CTRL0_WAND,
	                       ~PPC_BIT(PCBSLMC01_NET_CTRL0_FENCE_EN));

	/* }*/
}

static void p9_sbe_common_configure_chiplet_FIR(uint8_t chip, chiplet_id_t id)
{
	// reset pervasive FIR
	/*
	TP.TCMC01.MCSLOW.LOCAL_FIR
	  [all]   0
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_LOCAL_FIR, 0);

	// configure pervasive FIR action/mask
	/*
	TP.TCMC01.MCSLOW.LOCAL_FIR_ACTION0
	  [all]   0
	TP.TCMC01.MCSLOW.LOCAL_FIR_ACTION1
	  [all]   0
	  [0-3]   0xF
	TP.TCMC01.MCSLOW.LOCAL_FIR_MASK
	  [all]   0
	  [4-41]  0x3FFFFFFFFF (every bit set)
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_LOCAL_FIR_ACTION0, 0);
	write_scom_for_chiplet(chip, id, MCSLOW_LOCAL_FIR_ACTION1, PPC_BITMASK(0, 3));
	write_scom_for_chiplet(chip, id, MCSLOW_LOCAL_FIR_MASK, PPC_BITMASK(4, 41));

	// reset XFIR
	/*
	TP.TCMC01.MCSLOW.XFIR
	  [all]   0
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_XFIR, 0);

	// configure XFIR mask
	/*
	TP.TCMC01.MCSLOW.FIR_MASK
	  [all]   0
	*/
	write_scom_for_chiplet(chip, id, MCSLOW_FIR_MASK, 0);
}

static void mem_startclocks(uint8_t chip)
{
	int i;
	uint16_t pg[MCS_PER_PROC];

	mvpd_get_mcs_pg(chip, pg);

	for (i = 0; i < MCS_PER_PROC; i++) {
		const uint64_t mcs_pg = PPC_PLACE(pg[i], 0, 16);

		/* According to logs, Hostboot does it also for the second MCS */
		//~ if (!mem_data.mcs[i].functional)
			//~ continue;

		// Call p9_mem_startclocks_cplt_ctrl_action_function for Mc chiplets
		p9_mem_startclocks_cplt_ctrl_action_function(chip, mcs_ids[i], mcs_pg);

		// Call module align chiplets for Mc chiplets
		p9_sbe_common_align_chiplets(chip, mcs_ids[i]);

		// Call module clock start stop for MC01, MC23
		p9_sbe_common_clock_start_stop(chip, mcs_ids[i], mcs_pg);

		// Call p9_mem_startclocks_fence_setup_function for Mc chiplets
		p9_mem_startclocks_fence_setup_function(chip, mcs_ids[i]);

		// Clear flush_inhibit to go in to flush mode
		/*
		TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_CLEAR)
		  [all]   0
		  [2]     CTRL_CC_FLUSHMODE_INH_DC =  1
		*/
		write_scom_for_chiplet(chip, mcs_ids[i], MCSLOW_CPLT_CTRL0_WCLEAR,
		                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FLUSHMODE_INH_DC));

		// Call p9_sbe_common_configure_chiplet_FIR for MC chiplets
		p9_sbe_common_configure_chiplet_FIR(chip, mcs_ids[i]);

		// Reset FBC chiplet configuration
		/*
		TP.TCMC01.MCSLOW.CPLT_CONF0
		  [48-51] TC_UNIT_GROUP_ID_DC = ATTR_PROC_FABRIC_GROUP_ID   // Where do these come from?
		  [52-54] TC_UNIT_CHIP_ID_DC =  ATTR_PROC_FABRIC_CHIP_ID
		  [56-60] TC_UNIT_SYS_ID_DC =   ATTR_PROC_FABRIC_SYSTEM_ID  // 0 in talos.xml
		*/
		/*
		 * Take 0 for all values - assuming ATTR_PROC_FABRIC_GROUP_ID is
		 * ATTR_FABRIC_GROUP_ID of parent PROC (same for CHIP_ID). Only
		 * SYSTEM_ID is present in talos.xml with full name.
		 */
		scom_and_or_for_chiplet(chip, mcs_ids[i], MCSLOW_CPLT_CONF0,
		                        ~(PPC_BITMASK(48, 54) | PPC_BITMASK(56, 60)),
		                        PPC_PLACE(chip, 48, 4));

		// Add to Multicast Group
		/* Avoid setting if register is already set, i.e. [3-5] != 7 */
		/*
		TP.TPCHIP.NET.PCBSLMC01.MULTICAST_GROUP_1
		  [3-5]   MULTICAST1_GROUP: if 7 then set to 0
		  [16-23] (not described):  if [3-5] == 7 then set to 0x1C    // No clue why Hostboot modifies these bits
		TP.TPCHIP.NET.PCBSLMC01.MULTICAST_GROUP_2
		  [3-5]   MULTICAST1_GROUP: if 7 then set to 2
		  [16-23] (not described):  if [3-5] == 7 then set to 0x1C
		*/
		if ((read_scom_for_chiplet(chip, mcs_ids[i], PCBSLMC01_MULTICAST_GROUP_1) &
		     PPC_BITMASK(3, 5)) == PPC_BITMASK(3, 5))
			scom_and_or_for_chiplet(chip, mcs_ids[i], PCBSLMC01_MULTICAST_GROUP_1,
			                        ~(PPC_BITMASK(3, 5) | PPC_BITMASK(16, 23)),
			                        PPC_BITMASK(19, 21));

		if ((read_scom_for_chiplet(chip, mcs_ids[i], PCBSLMC01_MULTICAST_GROUP_2) &
		     PPC_BITMASK(3, 5)) == PPC_BITMASK(3, 5))
			scom_and_or_for_chiplet(chip, mcs_ids[i], PCBSLMC01_MULTICAST_GROUP_2,
			                        ~(PPC_BITMASK(3, 5) | PPC_BITMASK(16, 23)),
			                        PPC_BIT(4) | PPC_BITMASK(19, 21));
	}

}

void istep_13_6(uint8_t chips)
{
	uint8_t chip;

	report_istep(13, 6);

	/* Assuming MC doesn't run in sync mode with Fabric, otherwise this is no-op */

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			mem_startclocks(chip);
	}
}
