/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <timer.h>

#include "istep_13_scom.h"

/*
 * 13.2 mem_pll_reset: Reset PLL for MCAs in async
 *
 * a) p9_mem_pll_reset.C (proc chip)
 *    - This step is a no-op on cumulus as the centaur is already has its PLLs
 *      setup in step 11
 *    - This step is a no-op if memory is running in synchronous mode since the
 *      MCAs are using the nest PLL, HWP detect and exits
 *    - If in async mode then this HWP will put the PLL into bypass, reset mode
 *    - Disable listen_to_sync for MEM chiplet, whenever MEM is not in sync to
 *      NEST
 */
void istep_13_2(void)
{
	printk(BIOS_EMERG, "starting istep 13.2\n");
	int i;
	long time_elapsed = 0;

	report_istep(13,2);

	/* Assuming MC doesn't run in sync mode with Fabric, otherwise this is no-op */

	for (i = 0; i < MCS_PER_PROC; i++) {
		// Assert endpoint reset
		/*
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WOR)
		  [all] 0
		  [1]   PCB_EP_RESET =  1
		*/
		write_scom_for_chiplet(mcs_ids[i], PCBSLMC01_NET_CTRL0_WOR,
		                       PPC_BIT(PCBSLMC01_NET_CTRL0_PCB_EP_RESET));

		// Mask PLL unlock error in PCB slave
		/*
		TP.TPCHIP.NET.PCBSLMC01.SLAVE_CONFIG_REG
		  [12]  (part of) ERROR_MASK =  1
		*/
		scom_or_for_chiplet(mcs_ids[i], PCBSLMC01_SLAVE_CONFIG_REG,
		                    PPC_BIT(12));

		// Move MC PLL into reset state (3 separate writes, no delays between them)
		/*
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WOR)
		  [all] 0
		  [5]   PLL_BYPASS =  1
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WOR)
		  [all] 0
		  [4]   PLL_RESET =   1
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WOR)
		  [all] 0
		  [3]   PLL_TEST_EN = 1
		*/
		write_scom_for_chiplet(mcs_ids[i], PCBSLMC01_NET_CTRL0_WOR,
		                       PPC_BIT(PCBSLMC01_NET_CTRL0_PLL_BYPASS));
		write_scom_for_chiplet(mcs_ids[i], PCBSLMC01_NET_CTRL0_WOR,
		                       PPC_BIT(PCBSLMC01_NET_CTRL0_PLL_RESET));
		write_scom_for_chiplet(mcs_ids[i], PCBSLMC01_NET_CTRL0_WOR,
		                       PPC_BIT(PCBSLMC01_NET_CTRL0_PLL_TEST_EN));

		// Assert MEM PLDY and DCC bypass
		/*
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL1 (WOR)
		  [all] 0
		  [1]   CLK_DCC_BYPASS_EN =   1
		  [2]   CLK_PDLY_BYPASS_EN =  1
		*/
		write_scom_for_chiplet(mcs_ids[i], PCBSLMC01_NET_CTRL1_WOR,
		                       PPC_BIT(PCBSLMC01_NET_CTRL1_CLK_DCC_BYPASS_EN) |
		                       PPC_BIT(PCBSLMC01_NET_CTRL1_CLK_PDLY_BYPASS_EN));

		// Drop endpoint reset
		/*
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WAND)
		  [all] 1
		  [1]   PCB_EP_RESET =  0
		*/
		write_scom_for_chiplet(mcs_ids[i], PCBSLMC01_NET_CTRL0_WAND,
		                       ~PPC_BIT(PCBSLMC01_NET_CTRL0_PCB_EP_RESET));

		// Disable listen to sync pulse to MC chiplet, when MEM is not in sync to nest
		/*
		TP.TCMC01.MCSLOW.SYNC_CONFIG
		  [4] LISTEN_TO_SYNC_PULSE_DIS = 1
		*/
		scom_or_for_chiplet(mcs_ids[i], MCSLOW_SYNC_CONFIG,
		                    PPC_BIT(MCSLOW_SYNC_CONFIG_LISTEN_TO_SYNC_PULSE_DIS));

		// Initialize OPCG_ALIGN register
		/*
		TP.TCMC01.MCSLOW.OPCG_ALIGN
		  [all] 0
		  [0-3]   INOP_ALIGN =        5         // 8:1
		  [12-19] INOP_WAIT =         0
		  [47-51] SCAN_RATIO =        0         // 1:1
		  [52-63] OPCG_WAIT_CYCLES =  0x20
		*/
		write_scom_for_chiplet(mcs_ids[i], MCSLOW_OPCG_ALIGN,
		                       PPC_SHIFT(5, MCSLOW_OPCG_ALIGN_INOP_ALIGN) |
		                       PPC_SHIFT(0x20, MCSLOW_OPCG_ALIGN_OPCG_WAIT_CYCLES));

		// scan0 flush PLL boundary ring
		/*
		TP.TCMC01.MCSLOW.CLK_REGION
		  [all]   0
		  [14]    CLOCK_REGION_UNIT10 = 1
		  [48]    SEL_THOLD_SL =        1
		  [49]    SEL_THOLD_NSL =       1
		  [50]    SEL_THOLD_ARY =       1
		TP.TCMC01.MCSLOW.SCAN_REGION_TYPE
		  [all]   0
		  [14]    SCAN_REGION_UNIT10 =  1
		  [56]    SCAN_TYPE_BNDY =      1
		TP.TCMC01.MCSLOW.OPCG_REG0
		  [0]     RUNN_MODE =           0
		// Separate write, but don't have to read again
		TP.TCMC01.MCSLOW.OPCG_REG0
		  [2]     RUN_SCAN0 =           1
		*/
		write_scom_for_chiplet(mcs_ids[i], MCSLOW_CLK_REGION,
		                       PPC_BIT(MCSLOW_CLK_REGION_CLOCK_REGION_UNIT10) |
		                       PPC_BIT(MCSLOW_CLK_REGION_SEL_THOLD_SL) |
		                       PPC_BIT(MCSLOW_CLK_REGION_SEL_THOLD_NSL) |
		                       PPC_BIT(MCSLOW_CLK_REGION_SEL_THOLD_ARY));
		write_scom_for_chiplet(mcs_ids[i], MCSLOW_SCAN_REGION_TYPE,
		                       PPC_BIT(MCSLOW_SCAN_REGION_TYPE_SCAN_REGION_UNIT10) |
		                       PPC_BIT(MCSLOW_SCAN_REGION_TYPE_SCAN_TYPE_BNDY));
		scom_and_for_chiplet(mcs_ids[i], MCSLOW_OPCG_REG0,
		                     ~PPC_BIT(MCSLOW_OPCG_RUNN_MODE));
		scom_or_for_chiplet(mcs_ids[i], MCSLOW_OPCG_REG0,
		                    PPC_BIT(MCSLOW_OPCG_RUN_SCAN0));
	}

	/* Separate loop so we won't have to wait for timeout twice */
	for (i = 0; i < MCS_PER_PROC; i++) {
		/* FIXME: previous one didn't skip nonfunctional, should this one? */
		//~ if (!mem_data.mcs[i].functional)
			//~ continue;

		/*
		timeout(200 * 16us):
		  TP.TCMC01.MCSLOW.CPLT_STAT0
		  if (([8] CC_CTRL_OPCG_DONE_DC) == 1) break
		  delay(16us)
		*/
		time_elapsed = wait_us(200 * 16 - time_elapsed,
		                       read_scom_for_chiplet(mcs_ids[i], MCSLOW_CPLT_STAT0) &
		                       PPC_BIT(MCSLOW_CPLT_STAT0_CC_CTRL_OPCG_DONE_DC));

		if (!time_elapsed)
			die("Timed out while waiting for PLL boundary ring flush\n");

		// Cleanup
		/*
		TP.TCMC01.MCSLOW.CLK_REGION
		  [all]   0
		TP.TCMC01.MCSLOW.SCAN_REGION_TYPE
		  [all]   0
		*/
		write_scom_for_chiplet(mcs_ids[i], MCSLOW_CLK_REGION, 0);
		write_scom_for_chiplet(mcs_ids[i], MCSLOW_SCAN_REGION_TYPE, 0);
	}

	printk(BIOS_EMERG, "ending istep 13.2\n");
}
