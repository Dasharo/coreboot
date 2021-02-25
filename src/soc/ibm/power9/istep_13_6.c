/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <timer.h>

#include "istep_13_scom.h"

/*
 * FIXME: ATTR_PG value should come from MEMD partition, but it is empty after
 * build. Default value from talos.xml (5 for all chiplets) probably never makes
 * sense. Value read from already booted MVPD is 0xE0 (?) for both MCSs. We can
 * either add functions to read and parse MVPD or just hardcode the values. So
 * far I haven't found the code that writes to MVPD in Hostboot, other than for
 * PDI keyword (PG keyword should be used here).
 *
 * Value below comes from a log of booting Hostboot. It isn't even remotely
 * similar to values mentioned above. It touches bits marked as reserved in the
 * documentation, so we can't rely on specification to be up to date.
 *
 * As this describes whether clocks on second MCS should be started or not, this
 * definitely will be different when more DIMMs are installed.
 */
#define ATTR_PG			0xE1FC000000000000ull

static inline void p9_mem_startclocks_cplt_ctrl_action_function(chiplet_id_t id)
{
	// Drop partial good fences
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL1 (WO_CLEAR)
	  [all]   0
	  [3]     TC_VITL_REGION_FENCE =                  ~ATTR_PG[3]
	  [4-14]  TC_REGION{1-3}_FENCE, UNUSED_{8-14}B =  ~ATTR_PG[4-14]
	*/
	write_scom_for_chiplet(id, MCSLOW_CPLT_CTRL1_WCLEAR, ~ATTR_PG & PPC_BITMASK(3,14));

	// Reset abistclk_muxsel and syncclk_muxsel
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_CLEAR)
	  [all]   0
	  [0]     CTRL_CC_ABSTCLK_MUXSEL_DC = 1
	  [1]     TC_UNIT_SYNCCLK_MUXSEL_DC = 1
	*/
	write_scom_for_chiplet(id, MCSLOW_CPLT_CTRL0_WCLEAR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_ABSTCLK_MUXSEL_DC) |
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_TC_UNIT_SYNCCLK_MUXSEL_DC));

}

static inline void p9_sbe_common_align_chiplets(chiplet_id_t id)
{
	// Exit flush
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_OR)
	  [all]   0
	  [2]     CTRL_CC_FLUSHMODE_INH_DC =  1
	*/
	write_scom_for_chiplet(id, MCSLOW_CPLT_CTRL0_WOR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FLUSHMODE_INH_DC));

	// Enable alignement
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_OR)
	  [all]   0
	  [3]     CTRL_CC_FORCE_ALIGN_DC =    1
	*/
	write_scom_for_chiplet(id, MCSLOW_CPLT_CTRL0_WOR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FORCE_ALIGN_DC));

	// Clear chiplet is aligned
	/*
	TP.TCMC01.MCSLOW.SYNC_CONFIG
	  [7]     CLEAR_CHIPLET_IS_ALIGNED =  1
	*/
	scom_or_for_chiplet(id, MCSLOW_SYNC_CONFIG,
	                    PPC_BIT(MCSLOW_SYNC_CONFIG_CLEAR_CHIPLET_IS_ALIGNED));

	// Unset Clear chiplet is aligned
	/*
	TP.TCMC01.MCSLOW.SYNC_CONFIG
	  [7]     CLEAR_CHIPLET_IS_ALIGNED =  0
	*/
	scom_and_for_chiplet(id, MCSLOW_SYNC_CONFIG,
	                     ~PPC_BIT(MCSLOW_SYNC_CONFIG_CLEAR_CHIPLET_IS_ALIGNED));

	udelay(100);

	// Poll aligned bit
	/*
	timeout(10*100us):
	TP.TCMC01.MCSLOW.CPLT_STAT0
	if (([9] CC_CTRL_CHIPLET_IS_ALIGNED_DC) == 1) break
	delay(100us)
	*/
	if (!wait_us(10 * 100, read_scom_for_chiplet(id, MCSLOW_CPLT_STAT0) &
	             PPC_BIT(MCSLOW_CPLT_STAT0_CC_CTRL_CHIPLET_IS_ALIGNED_DC)))
		die("Timeout while waiting for chiplet alignment\n");

	// Disable alignment
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_CLEAR)
	  [all]   0
	  [3]     CTRL_CC_FORCE_ALIGN_DC =  1
	*/
	write_scom_for_chiplet(id, MCSLOW_CPLT_CTRL0_WCLEAR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FORCE_ALIGN_DC));
}

static void p9_sbe_common_clock_start_stop(chiplet_id_t id)
{
	// Chiplet exit flush
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_OR)
	  [all]   0
	  [2]     CTRL_CC_FLUSHMODE_INH_DC =  1
	*/
	write_scom_for_chiplet(id, MCSLOW_CPLT_CTRL0_WOR,
	                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FLUSHMODE_INH_DC));

	// Clear Scan region type register
	/*
	TP.TCMC01.MCSLOW.SCAN_REGION_TYPE
	  [all]   0
	*/
	write_scom_for_chiplet(id, MCSLOW_SCAN_REGION_TYPE, 0);

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
	scom_and_or_for_chiplet(id, MCSLOW_CLK_REGION,
	                        ~(PPC_BITMASK(0,14) | PPC_BITMASK(48, 50)),
	                        PPC_SHIFT(1, MCSLOW_CLK_REGION_CLOCK_CMD) |
	                        PPC_BIT(MCSLOW_CLK_REGION_SEL_THOLD_SL) |
	                        PPC_BIT(MCSLOW_CLK_REGION_SEL_THOLD_NSL) |
	                        PPC_BIT(MCSLOW_CLK_REGION_SEL_THOLD_ARY) |
	                        (~ATTR_PG & PPC_BITMASK(4, 13)));

	// Poll OPCG done bit to check for completeness
	/*
	timeout(10*100us):
	TP.TCMC01.MCSLOW.CPLT_STAT0
	if (([8] CC_CTRL_OPCG_DONE_DC) == 1) break
	delay(100us)
	*/
	if (!wait_us(10 * 100, read_scom_for_chiplet(id, MCSLOW_CPLT_STAT0) &
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
	uint64_t mask = ATTR_PG & PPC_BITMASK(4, 13);
	if ((read_scom_for_chiplet(id, MCSLOW_CLOCK_STAT_SL) & PPC_BITMASK(4, 13)) != mask ||
	    (read_scom_for_chiplet(id, MCSLOW_CLOCK_STAT_NSL) & PPC_BITMASK(4, 13)) != mask ||
	    (read_scom_for_chiplet(id, MCSLOW_CLOCK_STAT_ARY) & PPC_BITMASK(4, 13)) != mask)
		die("Unexpected clock status\n");
}

static inline void p9_mem_startclocks_fence_setup_function(chiplet_id_t id)
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
	write_scom_for_chiplet(id, PCBSLMC01_NET_CTRL0_WAND,
	                       ~PPC_BIT(PCBSLMC01_NET_CTRL0_FENCE_EN));

	/* }*/
}

static void p9_sbe_common_configure_chiplet_FIR(chiplet_id_t id)
{
	// reset pervasive FIR
	/*
	TP.TCMC01.MCSLOW.LOCAL_FIR
	  [all]   0
	*/
	write_scom_for_chiplet(id, MCSLOW_LOCAL_FIR, 0);

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
	write_scom_for_chiplet(id, MCSLOW_LOCAL_FIR_ACTION0, 0);
	write_scom_for_chiplet(id, MCSLOW_LOCAL_FIR_ACTION1, PPC_BITMASK(0,3));
	write_scom_for_chiplet(id, MCSLOW_LOCAL_FIR_MASK, PPC_BITMASK(4,41));

	// reset XFIR
	/*
	TP.TCMC01.MCSLOW.XFIR
	  [all]   0
	*/
	write_scom_for_chiplet(id, MCSLOW_XFIR, 0);

	// configure XFIR mask
	/*
	TP.TCMC01.MCSLOW.FIR_MASK
	  [all]   0
	*/
	write_scom_for_chiplet(id, MCSLOW_FIR_MASK, 0);
}

/*
 * 13.6 mem_startclocks: Start clocks on MBA/MCAs
 *
 * a) p9_mem_startclocks.C (proc chip)
 *    - This step is a no-op on cumulus
 *    - This step is a no-op if memory is running in synchronous mode since the
 *      MCAs are using the nest PLL, HWP detect and exits
 *    - Drop fences and tholds on MBA/MCAs to start the functional clocks
 */
void istep_13_6(void)
{
	printk(BIOS_EMERG, "starting istep 13.6\n");
	int i;

	report_istep(13,6);

	/* Assuming MC doesn't run in sync mode with Fabric, otherwise this is no-op */

	for (i = 0; i < MCS_PER_PROC; i++) {
		/* According to logs, Hostboot does it also for the second MCS */
		//~ if (!mem_data.mcs[i].functional)
			//~ continue;

		// Call p9_mem_startclocks_cplt_ctrl_action_function for Mc chiplets
		p9_mem_startclocks_cplt_ctrl_action_function(mcs_ids[i]);

		// Call module align chiplets for Mc chiplets
		p9_sbe_common_align_chiplets(mcs_ids[i]);

		// Call module clock start stop for MC01, MC23
		p9_sbe_common_clock_start_stop(mcs_ids[i]);

		// Call p9_mem_startclocks_fence_setup_function for Mc chiplets
		p9_mem_startclocks_fence_setup_function(mcs_ids[i]);

		// Clear flush_inhibit to go in to flush mode
		/*
		TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_CLEAR)
		  [all]   0
		  [2]     CTRL_CC_FLUSHMODE_INH_DC =  1
		*/
		write_scom_for_chiplet(mcs_ids[i], MCSLOW_CPLT_CTRL0_WCLEAR,
		                       PPC_BIT(MCSLOW_CPLT_CTRL0_CTRL_CC_FLUSHMODE_INH_DC));

		// Call p9_sbe_common_configure_chiplet_FIR for MC chiplets
		p9_sbe_common_configure_chiplet_FIR(mcs_ids[i]);

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
		scom_and_for_chiplet(mcs_ids[i], MCSLOW_CPLT_CONF0,
		                     ~(PPC_BITMASK(48,54) | PPC_BITMASK(56,60)));

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
		if ((read_scom_for_chiplet(mcs_ids[i], PCBSLMC01_MULTICAST_GROUP_1) & PPC_BITMASK(3,5))
		    == PPC_BITMASK(3,5))
			scom_and_or_for_chiplet(mcs_ids[i], PCBSLMC01_MULTICAST_GROUP_1,
			                        ~(PPC_BITMASK(3,5) | PPC_BITMASK(16,23)),
			                        PPC_BITMASK(19,21));

		if ((read_scom_for_chiplet(mcs_ids[i], PCBSLMC01_MULTICAST_GROUP_2) & PPC_BITMASK(3,5))
		    == PPC_BITMASK(3,5))
			scom_and_or_for_chiplet(mcs_ids[i], PCBSLMC01_MULTICAST_GROUP_2,
			                        ~(PPC_BITMASK(3,5) | PPC_BITMASK(16,23)),
			                        PPC_BIT(4) | PPC_BITMASK(19,21));
	}

	printk(BIOS_EMERG, "ending istep 13.6\n");
}
