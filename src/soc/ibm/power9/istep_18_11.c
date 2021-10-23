/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_18.h>

/* TODO: this one will change if we add more than one processor */
#define MDMT (1)

static uint32_t calculate_topology_delay(void)
{
	/*
	 * In simple topology with one proc it is enough to assign 0.
	 * With multiple processors this will get more complicated,
	 * see calculate_node_link_delay() in Hostboot
	 */

	if(MDMT)
		return MDMT_TOD_GRID_CYCLE_STAGING_DELAY;

	/* TODO: check again if this really is write, not RMW */
	write_scom(PU_PB_ELINK_RT_DELAY_CTL_REG, PPC_BITMASK(2, 3));
	uint64_t l_bus_mode_reg = read_scom(PU_PB_ELINK_DLY_0123_REG);

	uint32_t bus_delay = ((l_bus_mode_reg & BUS_DELAY_47) >> 16) +
	                     (l_bus_mode_reg & BUS_DELAY_63);

	/*
	 * FIXME: floating point wasn't fully configured, see if we can skip it.
	 * Testing requires bigger topology, i.e. more CPUs.
	 */
	return (uint32_t)(1 + ((double)(bus_delay * 8 * 1000000)
	                        / (double)(4 * FREQ_X_MHZ * TOD_GRID_PS)));
}

static void calculate_m_path(void)
{
	uint64_t dual_edge_disable =
	    (read_scom(PERV_ROOT_CTRL8_SCOM) & PPC_BIT(P9A_PERV_ROOT_CTRL8_TP_PLL_CLKIN_SEL9_DC))
	    ? PPC_BIT(PERV_TOD_M_PATH_CTRL_REG_STEP_CREATE_DUAL_EDGE_DISABLE)
	    : 0;

	if(MDMT) {
		scom_and_or(PERV_TOD_M_PATH_CTRL_REG,
		            ~(PPC_BIT(M_PATH_0_OSC_NOT_VALID) |
		              PPC_BIT(M_PATH_0_STEP_ALIGN_DISABLE) |
		              PPC_BIT(PERV_TOD_M_PATH_CTRL_REG_STEP_CREATE_DUAL_EDGE_DISABLE) |
		              PPC_PLACE(0x7, M_PATH_0_STEP_CHECK_VALIDITY_COUNT_OFFSET,
		                        M_PATH_0_STEP_CHECK_VALIDITY_COUNT_LEN) |
		              PPC_PLACE(0x7, M_PATH_SYNC_CREATE_SPS_SELECT_OFFSET,
		                        M_PATH_SYNC_CREATE_SPS_SELECT_LEN) |
		              PPC_PLACE(0xF, M_PATH_0_STEP_CHECK_CPS_DEVIATION_OFFSET,
		                        M_PATH_0_STEP_CHECK_CPS_DEVIATION_LEN) |
		              PPC_PLACE(0x3, M_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR_OFFSET,
		                        M_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR_LEN)),
		            PPC_BIT(M_PATH_1_OSC_NOT_VALID) |
		            PPC_PLACE(0x8, M_PATH_0_STEP_CHECK_CPS_DEVIATION_OFFSET,
		                      M_PATH_0_STEP_CHECK_CPS_DEVIATION_LEN) |
		            PPC_PLACE(0x3, M_PATH_0_STEP_CHECK_VALIDITY_COUNT_OFFSET,
		                      M_PATH_0_STEP_CHECK_VALIDITY_COUNT_LEN) |
		            dual_edge_disable);
	} else {
		scom_and_or(PERV_TOD_M_PATH_CTRL_REG,
		            ~PPC_BIT(PERV_TOD_M_PATH_CTRL_REG_STEP_CREATE_DUAL_EDGE_DISABLE),
		            dual_edge_disable);
	}
}

void istep_18_11(void)
{
	printk(BIOS_EMERG, "starting istep 18.11\n");
	report_istep(18,11);

	/* Clear previous primary topology */
	write_scom(PERV_TOD_PRI_PORT_0_CTRL_REG, 0);
	write_scom(PERV_TOD_SEC_PORT_0_CTRL_REG, 0);

	/* Workaround for HW480181: Init remote sync checker tolerance to maximum
	 *   [26-27] REMOTE_SYNC_CHECK_CPS_DEVIATION_FACTOR = 0x3 (factor 8)
	 *   [28-31] REMOTE_SYNC_CHECK_CPS_DEVIATION        = 0xF (93.75%)
	 */
	scom_or(PERV_TOD_S_PATH_CTRL_REG, PPC_PLACE(0x3, 26, 2) | PPC_PLACE(0xF, 28, 4));

	/*
	 * Set PSS_MSS_CTRL_REG for primary configuration, assumptions:
	 * - MDMT = 1
	 * - valid oscillator is attached to master path-0, but not path-1
	 *   [0] PRI_M_PATH_SELECT     = 0 (path-0 selected)
	 *   [1] PRI_M_S_TOD_SELECT    = 1 (TOD is master)
	 *   [2] PRI_M_S_DRAWER_SELECT = 1 (drawer is master)
	 */
	scom_and_or(PERV_TOD_PSS_MSS_CTRL_REG, ~PPC_BIT(0),
	            PPC_BIT(1) | PPC_BIT(2));

	/* Configure PORT_CTRL_REGs (primary) */
	/*
	 * TODO: this touches XBUS/OBUS, but Hostboot doesn't modify registers so
	 * skip it for now and fix if needed.
	 */
	//scom_and_or(PERV_TOD_PRI_PORT_0_CTRL_REG, ???, ???);
	//scom_and_or(PERV_TOD_SEC_PORT_0_CTRL_REG, ???, ???);

	/* Configure M_PATH_CTRL_REG */
	/*
	 * TODO: check this again. Value is correct, not sure whether fields are
	 * correctly cleared. Also comment the values written.
	 */
	calculate_m_path();

	/* Configure I_PATH_CTRL_REG (primary) */
	/* PERV_TOD_PRI_PORT_0_CTRL_REG:
	 *   [32-39] PRI_I_PATH_DELAY_VALUE = calculate
	 * PERV_TOD_I_PATH_CTRL_REG:
	 *   [0]     I_PATH_DELAY_DISABLE                   = 0
	 *   [1]     I_PATH_DELAY_ADJUST_DISABLE            = 0
	 *   [6-7]   I_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR = 0 (factor = 1)
	 *   [8-11]  I_PATH_STEP_CHECK_CPS_DEVIATION        = 0xF (93.75%)
	 *   [13-15] I_PATH_STEP_CHECK_VALIDITY_COUNT       = 0x3 (count = 8)
	 */
	scom_and_or(PERV_TOD_PRI_PORT_0_CTRL_REG, ~PPC_BITMASK(32, 39),
	            PPC_PLACE(calculate_topology_delay(), 32, 8));
	scom_and_or(PERV_TOD_I_PATH_CTRL_REG,
	            ~(PPC_BIT(0) | PPC_BIT(1) | PPC_BITMASK(6, 11) | PPC_BITMASK(13, 15)),
	            PPC_PLACE(0xF, 8, 4) | PPC_PLACE(0x3, 13, 3));

	/* Configure INIT_CHIP_CTRL_REG (primary) */
	/*   [1-3]   I_PATH_CORE_SYNC_PERIOD_SELECT   = 0 (core sync period is 8us)
	 *   [4]     I_PATH_SYNC_CHECK_DISABLE        = 0 (enable internal path sync check)
	 *   [7]     MOVE_TOD_TO_TB_ON_2X_SYNC_ENABLE = 0 (1x sync boundaries)
	 *   [8]     USE_TB_SYNC_MECHANISM            = 0
	 *   [9]     USE_TB_STEP_SYNC                 = 0 (use TB step sync from internal path)
	 *   [10-15] LOW_ORDER_STEP_VALUE             = 0x3F (4-bit WOF counter is incremented with each 200MHz clock cycle)
	 *   [30]    XSTOP_GATE                       = 0 (stop TOD on checkstop)
	 */
	scom_and_or(PERV_TOD_CHIP_CTRL_REG,
	            ~(PPC_BITMASK(1, 4) | PPC_BITMASK(7, 15) | PPC_BIT(30)),
	            PPC_PLACE(0x3F, 10, 6));


	/* TODO: test if we can skip repeated writes (M_PATH, I_PATH, CHIP) */

	/* Clear previous secondary topology */
	/* NOTE: order is swapped wrt primary, does it matter? */
	write_scom(PERV_TOD_SEC_PORT_1_CTRL_REG, 0);
	write_scom(PERV_TOD_PRI_PORT_1_CTRL_REG, 0);

	/*
	 * Set PSS_MSS_CTRL_REG for secondary configuration, assumptions as before
	 *   [8]  SEC_M_PATH_SELECT     = 0 (path-0 selected)
	 *   [9]  SEC_M_S_TOD_SELECT    = 1 (TOD is master)
	 *   [10] SEC_M_S_DRAWER_SELECT = 1 (drawer is master)
	 */
	scom_and_or(PERV_TOD_PSS_MSS_CTRL_REG, ~PPC_BIT(8),
	            PPC_BIT(9) | PPC_BIT(10));

	/* Configure PORT_CTRL_REGs (secondary) */
	//scom_and_or(PERV_TOD_SEC_PORT_1_CTRL_REG, ???, ???);
	//scom_and_or(PERV_TOD_PRI_PORT_1_CTRL_REG, ???, ???);

	/* Configure M_PATH_CTRL_REG */
	calculate_m_path();

	/* Configure I_PATH_CTRL_REG (secondary) */
	/* PERV_TOD_SEC_PORT_0_CTRL_REG:
	 *   [32-39] SEC_I_PATH_DELAY_VALUE = calculate
	 * PERV_TOD_I_PATH_CTRL_REG:
	 *   [0]     I_PATH_DELAY_DISABLE                   = 0
	 *   [1]     I_PATH_DELAY_ADJUST_DISABLE            = 0
	 *   [6-7]   I_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR = 0 (factor = 1)
	 *   [8-11]  I_PATH_STEP_CHECK_CPS_DEVIATION        = 0xF (93.75%)
	 *   [13-15] I_PATH_STEP_CHECK_VALIDITY_COUNT       = 0x3 (count = 8)
	 */
	scom_and_or(PERV_TOD_SEC_PORT_0_CTRL_REG, ~PPC_BITMASK(32, 39),
	            PPC_PLACE(calculate_topology_delay(), 32, 8));
	scom_and_or(PERV_TOD_I_PATH_CTRL_REG,
	            ~(PPC_BIT(0) | PPC_BIT(1) | PPC_BITMASK(6, 11) | PPC_BITMASK(13, 15)),
	            PPC_PLACE(0xF, 8, 4) | PPC_PLACE(0x3, 13, 3));

	/* Configure INIT_CHIP_CTRL_REG (secondary) */
	/*   [1-3]   I_PATH_CORE_SYNC_PERIOD_SELECT   = 0 (core sync period is 8us)
	 *   [4]     I_PATH_SYNC_CHECK_DISABLE        = 0 (enable internal path sync check)
	 *   [7]     MOVE_TOD_TO_TB_ON_2X_SYNC_ENABLE = 0 (1x sync boundaries)
	 *   [8]     USE_TB_SYNC_MECHANISM            = 0
	 *   [9]     USE_TB_STEP_SYNC                 = 0 (use TB step sync from internal path)
	 *   [10-15] LOW_ORDER_STEP_VALUE             = 0x3F (4-bit WOF counter is incremented with each 200MHz clock cycle)
	 *   [30]    XSTOP_GATE                       = 0 (stop TOD on checkstop)
	 */
	scom_and_or(PERV_TOD_CHIP_CTRL_REG,
	            ~(PPC_BITMASK(1, 4) | PPC_BITMASK(7, 15) | PPC_BIT(30)),
	            PPC_PLACE(0x3F, 10, 6));

	printk(BIOS_EMERG, "ending istep 18.11\n");
}
