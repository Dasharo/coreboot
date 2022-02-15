/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_18.h>
#include <cpu/power/proc.h>

/*
 * 2 CPU topology
 *
 * Primary (CHIP0 is MDMT):
 *
 *     CHIP0 --TX--> XBUS1 --> XBUS1 --RX--> CHIP1
 *
 * Secondary (CHIP1 is MDMT):
 *
 *     CHIP0 <--RX-- XBUS1 <-- XBUS1 <--TX-- CHIP1
 */

static uint32_t calculate_topology_delay(uint8_t chip, uint8_t chips, uint8_t mdmt)
{
	enum { XBUS_LINK_FACTOR = 8 };

	uint32_t delay;

	/*
	 * In simple topology with one proc it is enough to assign node delay to 0.
	 * With multiple processors this will get more complicated,
	 * see calculate_node_link_delay() in Hostboot
	 */
	if (chips == 0x01)
		return MDMT_TOD_GRID_CYCLE_STAGING_DELAY;

	/* The only non-trivial topology supported is a two-node one */
	if (chips != 0x03)
		die("%s() supports only two-chip configuration\n", __func__);

	/* There are no nodes connected to non-MDMT chip, hence no delay */
	if (chip != mdmt)
		return 0;

	/*
	 * Find the most-delayed path in the topology; this is the MDMT's delay.
	 *
	 * A more advanced topologies would require building a tree and finding
	 * the longest/slowest path from top node to one of the leaves, which in
	 * our special case equals to delay to the next chip.
	 */

	/* For some reason this is a write, not RMW */
	write_rscom(chip, PU_PB_ELINK_RT_DELAY_CTL_REG, PPC_BITMASK(2, 3));
	uint64_t bus_mode_reg = read_rscom(chip, PU_PB_ELINK_DLY_0123_REG);

	uint32_t bus_delay = (((bus_mode_reg & BUS_DELAY_47) >> 16) +
	                      (bus_mode_reg & BUS_DELAY_63)) / 2;


	/*
	 * By default, the TOD grid runs at 400ps; TOD counts its delay based on this.
	 *
	 * Example: Bus round trip delay is 35 cycles and the bus is running at 4800MHz
	 *   - Divide by 2 to get one-way delay time
	 *   - Divide by 4800 * 10^6 to get delay in seconds
	 *   - Multiply by 10^12 to get delay in picoseconds
	 *   - Divide by 400ps to get TOD-grid-cycles
	 *   - (To avoid including math.h) Add 1 and cast to uint32_t to round up to nearest TOD-grid-cycle
	 *   - (To avoid including math.h) 10^12/10^6=1000000
	 *   - (uint32_t)(( 35 / 2 / (4800 * 10^6) * 10^12 / 400 ) + 1) = 10 TOD-grid-cycles
	 */
	/*
	 * FIXME: floating point wasn't fully configured, see if we can skip it.
	 * Testing requires bigger topology, i.e. more CPUs.
	 */
	delay = (uint32_t)(1 + ((double)(bus_delay * 1000000)
	                      / (double)(2 * XBUS_LINK_FACTOR * FREQ_X_MHZ * TOD_GRID_PS)));

	/* The MDMT delay must include additional TOD-grid-cycles to account for
	 * staging latches in slaves */
	if (chip == mdmt)
		delay += MDMT_TOD_GRID_CYCLE_STAGING_DELAY;

	return delay;
}

static void calculate_m_path(uint8_t chip, uint8_t mdmt)
{
	uint64_t dual_edge_disable =
	    (read_rscom(chip, PERV_ROOT_CTRL8_SCOM) &
	     PPC_BIT(P9A_PERV_ROOT_CTRL8_TP_PLL_CLKIN_SEL9_DC))
	    ? PPC_BIT(PERV_TOD_M_PATH_CTRL_REG_STEP_CREATE_DUAL_EDGE_DISABLE)
	    : 0;

	if (chip == mdmt) {
		rscom_and_or(chip, PERV_TOD_M_PATH_CTRL_REG,
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
		rscom_and_or(chip, PERV_TOD_M_PATH_CTRL_REG,
		             ~PPC_BIT(PERV_TOD_M_PATH_CTRL_REG_STEP_CREATE_DUAL_EDGE_DISABLE),
		             dual_edge_disable);
	}
}

static void configure_tod(uint8_t chip, uint8_t chips, uint8_t pri_mdmt, uint8_t sec_mdmt)
{
	uint32_t topology_delay = calculate_topology_delay(chip, chips, pri_mdmt);

	/* Clear previous primary topology */
	write_rscom(chip, PERV_TOD_PRI_PORT_0_CTRL_REG, 0);
	write_rscom(chip, PERV_TOD_SEC_PORT_0_CTRL_REG, 0);

	/* Workaround for HW480181: Init remote sync checker tolerance to maximum
	 *   [26-27] REMOTE_SYNC_CHECK_CPS_DEVIATION_FACTOR = 0x3 (factor 8)
	 *   [28-31] REMOTE_SYNC_CHECK_CPS_DEVIATION        = 0xF (93.75%)
	 */
	rscom_or(chip, PERV_TOD_S_PATH_CTRL_REG, PPC_PLACE(0x3, 26, 2) | PPC_PLACE(0xF, 28, 4));

	/*
	 * Set PSS_MSS_CTRL_REG for primary configuration, assumption:
	 * - valid oscillator is attached to master path-0, but not path-1
	 *   [0] PRI_M_PATH_SELECT     = 0 (path-0 selected)
	 *   [1] PRI_M_S_TOD_SELECT    = 1 (TOD is master)
	 *   [2] PRI_M_S_DRAWER_SELECT = 1 (drawer is master)
	 */
	if (chip == pri_mdmt) {
		rscom_and_or(chip, PERV_TOD_PSS_MSS_CTRL_REG,
		             ~PPC_BIT(0), PPC_BIT(1) | PPC_BIT(2));
	} else {
		rscom_and(chip, PERV_TOD_PSS_MSS_CTRL_REG, ~PPC_BIT(1));
	}

	/*
	 * Set CPS deviation to 75% (CPS deviation bits = 0xC, factor=1),
	 * 8 valid steps to enable step check.
	 *
	 * [0]     S_PATH_CTRL_REG_PRI_SELECT                             = 0 (slave path 0)
	 * [6-7]   S_PATH_CTRL_REG_STEP_CHECK_CPS_DEVIATION_FACTOR        = 0 (factor = 1)
	 * [8-11]  S_PATH_CTRL_REG_0_STEP_CHECK_CPS_DEVIATION             = 0xC (75%)
	 * [13-15] S_PATH_CTRL_REG_0_STEP_CHECK_VALIDITY_COUNT            = 3 (8 valid steps)
	 * [26-27] S_PATH_CTRL_REG_REMOTE_SYNC_CHECK_CPS_DEVIATION_FACTOR = 0 (factor = 1)
	 * [28-31] S_PATH_CTRL_REG_REMOTE_SYNC_CHECK_CPS_DEVIATION        = 0xC (75%)
	 * [32-39] TOD_S_PATH_CTRL_REG_REMOTE_SYNC_MISS_COUNT_2           = 0x5
	 */
	if (chip != pri_mdmt) {
		rscom_and_or(chip, PERV_TOD_S_PATH_CTRL_REG,
		             ~(PPC_BIT(0) | PPC_BITMASK(6, 11) | PPC_BITMASK(13, 15) |
		               PPC_BITMASK(26, 39)),
		             PPC_PLACE(0xC, 8, 4) | PPC_PLACE(0x3, 13, 3) |
		             PPC_PLACE(0xC, 28, 4) | PPC_PLACE(0x5, 32, 8));
	}

	/*
	 * Configure PORT_CTRL_REGs (primary), assumptions:
	 *  - XBUS1 link is used on both chips
	 *  - oscillator connected to OSC0 and not to OSC1
	 *
	 * [0-2] PRI_PORT_0_CTRL_REG_RX_SELECT = 1 (link number)
	 * [6-7] TOD_PORT_CTRL_REG_TX_X1_SEL   = TX sel
	 * [21]  TOD_PORT_CTRL_REG_TX_X1_EN    = enable
	 */
	if (chips != 0x01) {
		/* MDMT has no RX */
		int rx_sel = (chip == pri_mdmt ? 0 : 1);
		/* Only MDMT has TX */
		int tx_sel = (chip == pri_mdmt ? 2 /*m_path_0*/ : 0);
		uint64_t tx_en = (chip == pri_mdmt ? PPC_BIT(21) : 0);

		rscom_and_or(chip, PERV_TOD_PRI_PORT_0_CTRL_REG,
		             ~(PPC_BITMASK(0, 2) | PPC_BITMASK(6, 7) | PPC_BIT(21)),
		             PPC_PLACE(rx_sel, 0, 3) | PPC_PLACE(tx_sel, 6, 2) | tx_en);
		rscom_and_or(chip, PERV_TOD_SEC_PORT_0_CTRL_REG,
		             ~(PPC_BITMASK(0, 2) | PPC_BITMASK(6, 7) | PPC_BIT(21)),
		             PPC_PLACE(rx_sel, 0, 3) | PPC_PLACE(tx_sel, 6, 2) | tx_en);
	}

	/* Configure M_PATH_CTRL_REG (primary) */
	/*
	 * TODO: check this again. Value is correct, not sure whether fields are
	 * correctly cleared. Also comment the values written.
	 */
	calculate_m_path(chip, pri_mdmt);

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
	rscom_and_or(chip, PERV_TOD_PRI_PORT_0_CTRL_REG,
	             ~PPC_BITMASK(32, 39), PPC_PLACE(topology_delay, 32, 8));
	rscom_and_or(chip, PERV_TOD_I_PATH_CTRL_REG,
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
	rscom_and_or(chip, PERV_TOD_CHIP_CTRL_REG,
	             ~(PPC_BITMASK(1, 4) | PPC_BITMASK(7, 15) | PPC_BIT(30)),
	             PPC_PLACE(0x3F, 10, 6));


	/* TODO: test if we can skip repeated writes (M_PATH, I_PATH, CHIP) */

	topology_delay = calculate_topology_delay(chip, chips, sec_mdmt);

	/* Clear previous secondary topology */
	/* NOTE: order is swapped wrt primary, does it matter? */
	write_rscom(chip, PERV_TOD_SEC_PORT_1_CTRL_REG, 0);
	write_rscom(chip, PERV_TOD_PRI_PORT_1_CTRL_REG, 0);

	/*
	 * Set PSS_MSS_CTRL_REG for secondary configuration, assumptions as before
	 *   [0]  PRI_M_PATH_SELECT     = 0 (path-0 selected) (SW440224)
	 *   [8]  SEC_M_PATH_SELECT     = 0 (path-0 selected)
	 *   [9]  SEC_M_S_TOD_SELECT    = 1 (TOD is master)
	 *   [10] SEC_M_S_DRAWER_SELECT = 1 (drawer is master)
	 */
	if (chip == sec_mdmt) {
		rscom_and_or(chip, PERV_TOD_PSS_MSS_CTRL_REG, ~(PPC_BIT(0) | PPC_BIT(8)),
		             PPC_BIT(9) | PPC_BIT(10));
	} else {
		rscom_and(chip, PERV_TOD_PSS_MSS_CTRL_REG, ~PPC_BIT(9));
	}

	/*
	 * Set CPS deviation to 75% (CPS deviation bits = 0xC, factor=1),
	 * 8 valid steps to enable step check.
	 *
	 * [0]     S_PATH_CTRL_REG_PRI_SELECT                             = 0 (slave path 0)
	 * [4]     S_PATH_CTRL_REG_SEC_SELECT                             = 1 (slave path 1)
	 * [6-7]   S_PATH_CTRL_REG_STEP_CHECK_CPS_DEVIATION_FACTOR        = 0 (factor = 1)
	 * [16-19] S_PATH_CTRL_REG_1_STEP_CHECK_CPS_DEVIATION             = 0xC (75%)
	 * [21-23] PERV_TOD_S_PATH_CTRL_REG_1_STEP_CHECK_VALIDITY_COUNT   = 3 (8 valid steps)
	 * [32-39] TOD_S_PATH_CTRL_REG_REMOTE_SYNC_MISS_COUNT_2           = 0x5
	 */
	if (chip != sec_mdmt) {
		rscom_and_or(chip, PERV_TOD_S_PATH_CTRL_REG,
		             ~(PPC_BIT(0) | PPC_BITMASK(6, 7) | PPC_BITMASK(16, 19) |
		               PPC_BITMASK(21, 23) | PPC_BITMASK(32, 39)),
		             PPC_BIT(4) | PPC_PLACE(0xC, 16, 4) | PPC_PLACE(0x3, 21, 3) |
		             PPC_PLACE(0x5, 32, 8));
	}

	/* Configure PORT_CTRL_REGs (secondary), assumptions as above.
	 *
	 * [0-2] PRI_PORT_0_CTRL_REG_RX_SELECT = 1 (link number)
	 * [6-7] TOD_PORT_CTRL_REG_TX_X1_SEL   = TX sel
	 * [21]  TOD_PORT_CTRL_REG_TX_X1_EN    = enable
	 */
	if (chips != 0x01) {
		/* MDMT has no RX */
		int rx_sel = (chip == sec_mdmt ? 0 : 1);
		/* Only MDMT has TX */
		int tx_sel = (chip == sec_mdmt ? 2 /*m_path_0*/ : 0);
		uint64_t tx_en = (chip == sec_mdmt ? PPC_BIT(21) : 0);

		rscom_and_or(chip, PERV_TOD_PRI_PORT_1_CTRL_REG,
		             ~(PPC_BITMASK(0, 2) | PPC_BITMASK(6, 7) | PPC_BIT(21)),
		             PPC_PLACE(rx_sel, 0, 3) | PPC_PLACE(tx_sel, 6, 2) | tx_en);
		rscom_and_or(chip, PERV_TOD_SEC_PORT_1_CTRL_REG,
		             ~(PPC_BITMASK(0, 2) | PPC_BITMASK(6, 7) | PPC_BIT(21)),
		             PPC_PLACE(rx_sel, 0, 3) | PPC_PLACE(tx_sel, 6, 2) | tx_en);
	}

	/* Configure M_PATH_CTRL_REG (secondary) */
	calculate_m_path(chip, sec_mdmt);

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
	rscom_and_or(chip, PERV_TOD_SEC_PORT_0_CTRL_REG,
	             ~PPC_BITMASK(32, 39), PPC_PLACE(topology_delay, 32, 8));
	rscom_and_or(chip, PERV_TOD_I_PATH_CTRL_REG,
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
	rscom_and_or(chip, PERV_TOD_CHIP_CTRL_REG,
	             ~(PPC_BITMASK(1, 4) | PPC_BITMASK(7, 15) | PPC_BIT(30)),
	             PPC_PLACE(0x3F, 10, 6));
}

static int core_count(uint8_t chip)
{
	uint64_t cores = read_rscom(chip, 0x0006C090);
	return __builtin_popcount((uint32_t)cores) + __builtin_popcount(cores >> 32);
}

void istep_18_11(uint8_t chips)
{
	uint8_t pri_mdmt;
	uint8_t sec_mdmt;
	uint8_t chip;

	printk(BIOS_EMERG, "starting istep 18.11\n");
	report_istep(18,11);

	if (chips != 0x01 && chips != 0x03)
		die("Unsupported number of chips for TOD: 0x%02x\n", chips);

	pri_mdmt = 0;
	sec_mdmt = 0;

	if (chips == 0x3) {
		uint64_t cores[2] = { core_count(0), core_count(1) };
		/* CPU with max amount of cores is primary MDMT */
		pri_mdmt = (cores[0] <= cores[1] ? 0 : 1);
		/* The other one will be secondary MDMT */
		sec_mdmt = (cores[0] <= cores[1] ? 1 : 0);
	}

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			configure_tod(chip, chips, pri_mdmt, sec_mdmt);
	}

	printk(BIOS_EMERG, "ending istep 18.11\n");
}
