/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_10.h>

#include <console/console.h>
#include <cpu/power/scom.h>
#include <delay.h>
#include <stdint.h>
#include <string.h>
#include <timer.h>

#include "pci.h"

#define MAX_LANE_GROUPS_PER_PEC 4

#define NUM_PCIE_LANES 16
#define NUM_PCS_CONFIG 4

/* Enum indicating lane width (units = "number of lanes") */
enum lane_width {
	LANE_WIDTH_NC  = 0,
	LANE_WIDTH_4X  = 4,
	LANE_WIDTH_8X  = 8,
	LANE_WIDTH_16X = 16
};

enum lane_mask {
	LANE_MASK_X16     = 0xFFFF,
	LANE_MASK_X8_GRP0 = 0xFF00,
	LANE_MASK_X8_GRP1 = 0x00FF,
	LANE_MASK_X4_GRP0 = 0x00F0,
	LANE_MASK_X4_GRP1 = 0x000F,
};

/* Enumeration of PHB to PCI MAC mappings */
enum phb_to_mac {
	PHB_X16_MAC_MAP      = 0x0000,
	PHB_X8_X8_MAC_MAP    = 0x0050,
	PHB_X8_X4_X4_MAC_MAP = 0x0090,
};

/*
 * Bit position of the PHB with the largest number a given PEC can use
 * (see enum phb_active_mask for bit values).
 */
enum pec_phb_shift {
	PEC0_PHB_SHIFT = 7, // PHB0 only
	PEC1_PHB_SHIFT = 5, // PHB1 - PHB2
	PEC2_PHB_SHIFT = 2, // PHB3 - PHB5
};

/*
 * Struct for each row in PCIE IOP configuration table.
 * Used by code to compute the IOP config and PHBs active mask.
 */
struct lane_config_row {
	/*
	 * Grouping of lanes under one IOP.
	 * Value signifies width of each PCIE lane set (0, 4, 8, or 16).
	 */
	uint8_t lane_set[MAX_LANE_GROUPS_PER_PEC]; // enum lane_width

	/* IOP config value from PCIE IOP configuration table */
	uint8_t lane_config;

	/* PHB active mask (see phb_active_mask enum) */
	uint8_t phb_active;

	uint16_t phb_to_pcie_mac; // enum phb_to_mac
};

/*
 * Currently there are three PEC config tables for procs with 48 usable PCIE
 * lanes. In general, the code accumulates the current configuration of
 * the PECs from the MRW and other dynamic information (such as bifurcation)
 * then matches that config to one of the rows in the table. Once a match
 * is discovered, the PEC config value is pulled from the matching row for
 * future use.
 *
 * Each PEC can control up to 16 lanes:
 * - PEC0 can give 16 lanes to PHB0
 * - PEC1 can split 16 lanes between PHB1 & PHB2
 * - PEC2 can split 16 lanes between PHB3, PHB4 & PHB5
 */
static const struct lane_config_row pec0_lane_cfg[] = {
	{
		{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB_MASK_NA,
		PHB_X16_MAC_MAP
	},
	{
		{ LANE_WIDTH_16X, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB0_MASK,
		PHB_X16_MAC_MAP
	},
};
static const struct lane_config_row pec1_lane_cfg[] = {
	{
		{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB_MASK_NA,
		PHB_X8_X8_MAC_MAP
	},
	{
		{ LANE_WIDTH_8X, LANE_WIDTH_NC, LANE_WIDTH_8X, LANE_WIDTH_NC },
		0x00,
		PHB1_MASK | PHB2_MASK,
		PHB_X8_X8_MAC_MAP
	},
	{
		{ LANE_WIDTH_8X, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB1_MASK,
		PHB_X8_X8_MAC_MAP
	},
	{
		{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_8X, LANE_WIDTH_NC },
		0x00,
		PHB2_MASK,
		PHB_X8_X8_MAC_MAP
	},
};
static const struct lane_config_row pec2_lane_cfg[] = {
	{
		{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB_MASK_NA,
		PHB_X16_MAC_MAP
	},
	{
		{ LANE_WIDTH_16X, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB3_MASK,
		PHB_X16_MAC_MAP
	},
	{
		{ LANE_WIDTH_8X, LANE_WIDTH_NC, LANE_WIDTH_8X, LANE_WIDTH_NC },
		0x10,
		PHB3_MASK | PHB4_MASK,
		PHB_X8_X8_MAC_MAP
	},
	{
		{ LANE_WIDTH_8X, LANE_WIDTH_NC, LANE_WIDTH_4X, LANE_WIDTH_4X },
		0x20,
		PHB3_MASK | PHB4_MASK | PHB5_MASK,
		PHB_X8_X4_X4_MAC_MAP
	},
};

static const struct lane_config_row *pec_lane_cfgs[] = {
	pec0_lane_cfg,
	pec1_lane_cfg,
	pec2_lane_cfg
};
static const size_t pec_lane_cfg_sizes[] = {
	ARRAY_SIZE(pec0_lane_cfg),
	ARRAY_SIZE(pec1_lane_cfg),
	ARRAY_SIZE(pec2_lane_cfg)
};

/*
 * PEC_PCIE_LANE_MASK_NON_BIFURCATED in processed talos.xml for the first
 * processor chip.  Values correspond to lane_width enumeration.
 */
static uint16_t lane_masks[MAX_PEC_PER_PROC][MAX_LANE_GROUPS_PER_PEC] = {
	{ LANE_MASK_X16,     0x0,               0x0,               0x0 },
	{ LANE_MASK_X8_GRP0, 0x0, LANE_MASK_X8_GRP1,               0x0 },
	{ LANE_MASK_X8_GRP0, 0x0, LANE_MASK_X4_GRP0, LANE_MASK_X4_GRP1 },
};

static const uint64_t RX_VGA_CTRL3_REGISTER[NUM_PCIE_LANES] = {
	0x8000008D0D010C3F,
	0x800000CD0D010C3F,
	0x8000018D0D010C3F,
	0x800001CD0D010C3F,
	0x8000028D0D010C3F,
	0x800002CD0D010C3F,
	0x8000038D0D010C3F,
	0x800003CD0D010C3F,
	0x8000088D0D010C3F,
	0x800008CD0D010C3F,
	0x8000098D0D010C3F,
	0x800009CD0D010C3F,
	0x80000A8D0D010C3F,
	0x80000ACD0D010C3F,
	0x80000B8D0D010C3F,
	0x80000BCD0D010C3F,
};

static const uint64_t RX_LOFF_CNTL_REGISTER[NUM_PCIE_LANES] = {
	0x800000A60D010C3F,
	0x800000E60D010C3F,
	0x800001A60D010C3F,
	0x800001E60D010C3F,
	0x800002A60D010C3F,
	0x800002E60D010C3F,
	0x800003A60D010C3F,
	0x800003E60D010C3F,
	0x800008A60D010C3F,
	0x800008E60D010C3F,
	0x800009A60D010C3F,
	0x800009E60D010C3F,
	0x80000AA60D010C3F,
	0x80000AE60D010C3F,
	0x80000BA60D010C3F,
	0x80000BE60D010C3F,
};

static enum lane_width lane_mask_to_width(uint16_t mask)
{
	enum lane_width width = LANE_WIDTH_NC;

	if (mask == LANE_MASK_X16)
		width = LANE_WIDTH_16X;
	else if (mask == LANE_MASK_X8_GRP0 || mask == LANE_MASK_X8_GRP1)
		width = LANE_WIDTH_8X;
	else if (mask == LANE_MASK_X4_GRP0 || mask == LANE_MASK_X4_GRP1)
		width = LANE_WIDTH_4X;

	return width;
}

static void determine_lane_configs(uint8_t *phb_active_mask,
				   const struct lane_config_row **pec_cfgs)
{
	uint8_t pec = 0;

	*phb_active_mask = 0;

	for (pec = 0; pec < MAX_PEC_PER_PROC; ++pec) {
		uint8_t i;
		uint8_t lane_group;

		struct lane_config_row config = {
			{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
			0x00,
			PHB_MASK_NA,
			PHB_X16_MAC_MAP,
		};

		/* Transform effective config to match lane config table format */
		for (lane_group = 0; lane_group < MAX_LANE_GROUPS_PER_PEC; ++lane_group)
			config.lane_set[lane_group] = lane_mask_to_width(lane_masks[pec][lane_group]);

		for (i = 0; i < pec_lane_cfg_sizes[pec]; ++i) {
			if (memcmp(pec_lane_cfgs[pec][i].lane_set, &config.lane_set,
				   sizeof(config.lane_set)) == 0)
				break;
		}

		if (i == pec_lane_cfg_sizes[pec])
			die("Failed to find PCIE IOP configuration for PEC%d\n", pec);

		*phb_active_mask |= pec_lane_cfgs[pec][i].phb_active;

		pec_cfgs[pec] = &pec_lane_cfgs[pec][i];

		/*
		 * In the rest of the PCIe-related code the following PEC attributes have these
		 * values:
		 *  - PEC[ATTR_PROC_PCIE_IOP_CONFIG]      := pec_cfgs[pec]->lane_config
		 *  - PEC[ATTR_PROC_PCIE_REFCLOCK_ENABLE] := 1
		 *  - PEC[ATTR_PROC_PCIE_PCS_SYSTEM_CNTL] := pec_cfgs[pec]->phb_to_pcie_mac
		 */
	}
}

static uint64_t pec_val(int pec_id, uint8_t in,
			int pec0_s, int pec0_c,
			int pec1_s, int pec1_c,
			int pec2_s, int pec2_c)
{
	uint64_t out = 0;

	switch (pec_id) {
		case 0:
			out = PPC_PLACE(in, pec0_s, pec0_c);
			break;
		case 1:
			out = PPC_PLACE(in, pec1_s, pec1_c);
			break;
		case 2:
			out = PPC_PLACE(in, pec2_s, pec2_c);
			break;
		default:
			die("Unknown PEC ID: %d\n", pec_id);
	}

	return out;
}

static void phase1(const struct lane_config_row **pec_cfgs,
		   const uint8_t *iovalid_enable)
{
	enum {
		PEC_CPLT_CONF1_OR = 0x0D000019,
		PEC_CPLT_CTRL0_OR = 0x0D000010,
		PEC_CPLT_CONF1_CLEAR = 0x0D000029,

		PEC_PCS_RX_ROT_CNTL_REG = 0x800004820D010C3F,
		PEC_PCS_RX_CONFIG_MODE_REG = 0x800004800D010C3F,
		PEC_PCS_RX_CDR_GAIN_REG = 0x800004B30D010C3F,
		PEC_PCS_RX_SIGDET_CONTROL_REG = 0x800004A70D010C3F,

		PCI_IOP_FIR_ACTION0_REG = 0x0000000000000000ULL,
		PCI_IOP_FIR_ACTION1_REG = 0xE000000000000000ULL,
		PCI_IOP_FIR_MASK_REG    = 0x1FFFFFFFF8000000ULL,

		PEC_FIR_ACTION0_REG = 0x0D010C06,
		PEC_FIR_ACTION1_REG = 0x0D010C07,
		PEC_FIR_MASK_REG = 0x0D010C03,

		PEC0_IOP_CONFIG_START_BIT = 13,
		PEC1_IOP_CONFIG_START_BIT = 14,
		PEC2_IOP_CONFIG_START_BIT = 10,
		PEC0_IOP_BIT_COUNT = 1,
		PEC1_IOP_BIT_COUNT = 2,
		PEC2_IOP_BIT_COUNT = 3,
		PEC0_IOP_SWAP_START_BIT = 12,
		PEC1_IOP_SWAP_START_BIT = 12,
		PEC2_IOP_SWAP_START_BIT = 7,
		PEC0_IOP_IOVALID_ENABLE_START_BIT = 4,
		PEC1_IOP_IOVALID_ENABLE_START_BIT = 4,
		PEC2_IOP_IOVALID_ENABLE_START_BIT = 4,
		PEC_IOP_IOVALID_ENABLE_STACK0_BIT = 4,
		PEC_IOP_IOVALID_ENABLE_STACK1_BIT = 5,
		PEC_IOP_IOVALID_ENABLE_STACK2_BIT = 6,
		PEC_IOP_REFCLOCK_ENABLE_START_BIT = 32,
		PEC_IOP_PMA_RESET_START_BIT = 29,
		PEC_IOP_PIPE_RESET_START_BIT = 28,

		PEC_PCS_PCLCK_CNTL_PLLA_REG = 0x8000050F0D010C3F,
		PEC_PCS_PCLCK_CNTL_PLLB_REG = 0x8000054F0D010C3F,
		PEC_PCS_TX_DCLCK_ROTATOR_REG = 0x800004450D010C3F,
		PEC_PCS_TX_PCIE_REC_DETECT_CNTL1_REG = 0x8000046C0D010C3F,
		PEC_PCS_TX_PCIE_REC_DETECT_CNTL2_REG = 0x8000046D0D010C3F,
		PEC_PCS_TX_POWER_SEQ_ENABLE_REG = 0x800004700D010C3F,

		PEC_SCOM0X0B_EDMOD = 52,

		PEC_PCS_RX_VGA_CONTROL1_REG = 0x8000048B0D010C3F,
		PEC_PCS_RX_VGA_CONTROL2_REG = 0x8000048C0D010C3F,
		PEC_IOP_RX_DFE_FUNC_REGISTER1 = 0x8000049F0D010C3F,
		PEC_PCS_SYS_CONTROL_REG = 0x80000C000D010C3F,

		PEC_PCS_M1_CONTROL_REG =  0x80000C010D010C3F,
		PEC_PCS_M2_CONTROL_REG =  0x80000C020D010C3F,
		PEC_PCS_M3_CONTROL_REG =  0x80000C030D010C3F,
		PEC_PCS_M4_CONTROL_REG =  0x80000C040D010C3F,
	};

	uint8_t pec = 0;

	for (pec = 0; pec < MAX_PEC_PER_PROC; ++pec) {
		long time;
		uint8_t i;
		uint64_t val;
		uint8_t proc_pcie_iop_swap;

		chiplet_id_t chiplet = PCI0_CHIPLET_ID + pec;

		/* ATTR_PROC_PCIE_PCS_RX_CDR_GAIN, from talos.xml */
		uint8_t pcs_cdr_gain[] = { 0x56, 0x47, 0x47, 0x47 };
		/* ATTR_PROC_PCIE_PCS_RX_INIT_GAIN, all zeroes by default */
		uint8_t pcs_init_gain = 0;
		/* ATTR_PROC_PCIE_PCS_RX_PK_INIT, all zeroes by default */
		uint8_t pcs_pk_init = 0;
		/* ATTR_PROC_PCIE_PCS_RX_SIGDET_LVL, defaults and talos.xml */
		uint8_t pcs_sigdet_lvl = 0x0B;

		uint32_t pcs_config_mode[NUM_PCS_CONFIG] = { 0xA006, 0xA805, 0xB071, 0xB870 };

		/* Phase1 init step 1 (get VPD, no operation here) */

		/* Phase1 init step 2a */
		val = pec_val(pec, pec_cfgs[pec]->lane_config,
			      PEC0_IOP_CONFIG_START_BIT, PEC0_IOP_BIT_COUNT * 2,
			      PEC1_IOP_CONFIG_START_BIT, PEC1_IOP_BIT_COUNT * 2,
			      PEC2_IOP_CONFIG_START_BIT, PEC2_IOP_BIT_COUNT * 2);
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_OR, val);

		/* Phase1 init step 2b */

		/* ATTR_PROC_PCIE_IOP_SWAP from processed talos.xml for first proc */
		proc_pcie_iop_swap = (pec == 0);

		val = pec_val(pec, proc_pcie_iop_swap,
			      PEC0_IOP_SWAP_START_BIT, PEC0_IOP_BIT_COUNT,
			      PEC1_IOP_SWAP_START_BIT, PEC1_IOP_BIT_COUNT,
			      PEC2_IOP_SWAP_START_BIT, PEC2_IOP_BIT_COUNT);
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_OR, val);

		/* Phase1 init step 3a */

		val = pec_val(pec, iovalid_enable[pec],
			      PEC0_IOP_IOVALID_ENABLE_START_BIT, PEC0_IOP_BIT_COUNT,
			      PEC1_IOP_IOVALID_ENABLE_START_BIT, PEC1_IOP_BIT_COUNT,
			      PEC2_IOP_IOVALID_ENABLE_START_BIT, PEC2_IOP_BIT_COUNT);

		/* Set IOVALID for base PHB if PHB2, or PHB4, or PHB5 are set (SW417485) */
		if ((val & PPC_BIT(PEC_IOP_IOVALID_ENABLE_STACK1_BIT)) ||
		    (val & PPC_BIT(PEC_IOP_IOVALID_ENABLE_STACK2_BIT))) {
			val |= PPC_BIT(PEC_IOP_IOVALID_ENABLE_STACK0_BIT);
			val |= PPC_BIT(PEC_IOP_IOVALID_ENABLE_STACK1_BIT);
		}

		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_OR, val);

		/* Phase1 init step 3b (enable clock) */
		/* ATTR_PROC_PCIE_REFCLOCK_ENABLE, all PECs are enabled. */
		write_scom_for_chiplet(chiplet, PEC_CPLT_CTRL0_OR,
				       PPC_BIT(PEC_IOP_REFCLOCK_ENABLE_START_BIT));

		/* Phase1 init step 4 (PMA reset) */

		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_CLEAR,
				       PPC_BIT(PEC_IOP_PMA_RESET_START_BIT));
		udelay(1); /* at least 400ns */
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_OR,
				       PPC_BIT(PEC_IOP_PMA_RESET_START_BIT));
		udelay(1); /* at least 400ns */
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_CLEAR,
				       PPC_BIT(PEC_IOP_PMA_RESET_START_BIT));

		/*
		 * Poll for PRTREADY status on PLLA and PLLB:
		 * PEC_IOP_PLLA_VCO_COURSE_CAL_REGISTER1 = 0x800005010D010C3F
		 * PEC_IOP_PLLB_VCO_COURSE_CAL_REGISTER1 = 0x800005410D010C3F
		 * PEC_IOP_HSS_PORT_READY_START_BIT = 58
		 */
		time = wait_us(40,
				(read_scom_for_chiplet(chiplet, 0x800005010D010C3F) & PPC_BIT(58)) ||
				(read_scom_for_chiplet(chiplet, 0x800005410D010C3F) & PPC_BIT(58)));
		if (!time)
			die("IOP HSS Port Ready status is not set!");

		/* Phase1 init step 5 (Set IOP FIR action0) */
		write_scom_for_chiplet(chiplet, PEC_FIR_ACTION0_REG, PCI_IOP_FIR_ACTION0_REG);

		/* Phase1 init step 6 (Set IOP FIR action1) */
		write_scom_for_chiplet(chiplet, PEC_FIR_ACTION1_REG, PCI_IOP_FIR_ACTION1_REG);

		/* Phase1 init step 7 (Set IOP FIR mask) */
		write_scom_for_chiplet(chiplet, PEC_FIR_MASK_REG, PCI_IOP_FIR_MASK_REG);

		/* Phase1 init step 8-11 (Config 0 - 3) */

		for (i = 0; i < NUM_PCS_CONFIG; ++i) {
			uint8_t lane;

			/* RX Config Mode */
			write_scom_for_chiplet(chiplet, PEC_PCS_RX_CONFIG_MODE_REG,
					       pcs_config_mode[i]);

			/* RX CDR GAIN */
			scom_and_or_for_chiplet(chiplet, PEC_PCS_RX_CDR_GAIN_REG,
						~PPC_BITMASK(56, 63),
						pcs_cdr_gain[i]);

			for (lane = 0; lane < NUM_PCIE_LANES; ++lane) {
				/* RX INITGAIN */
				scom_and_or_for_chiplet(chiplet, RX_VGA_CTRL3_REGISTER[lane],
							~PPC_BITMASK(48, 52),
							PPC_PLACE(pcs_init_gain, 48, 5));

				/* RX PKINIT */
				scom_and_or_for_chiplet(chiplet, RX_LOFF_CNTL_REGISTER[lane],
							~PPC_BITMASK(58, 63),
							pcs_pk_init);
			}

			/* RX SIGDET LVL */
			scom_and_or_for_chiplet(chiplet, PEC_PCS_RX_SIGDET_CONTROL_REG,
						~PPC_BITMASK(59, 63),
						pcs_sigdet_lvl);
		}

		/*
		 * Phase1 init step 12 (RX Rot Cntl CDR Lookahead Disabled, SSC Disabled)
                 *
		 * All these attributes are zero for Nimbus:
		 *  - ATTR_PROC_PCIE_PCS_RX_ROT_CDR_LOOKAHEAD (55)
		 *  - ATTR_PROC_PCIE_PCS_RX_ROT_CDR_SSC (63)
		 *  - ATTR_PROC_PCIE_PCS_RX_ROT_EXTEL (59)
		 *  - ATTR_PROC_PCIE_PCS_RX_ROT_RST_FW (62)
		 */
		scom_and_for_chiplet(chiplet, PEC_PCS_RX_ROT_CNTL_REG,
				     ~(PPC_BIT(55) | PPC_BIT(63) | PPC_BIT(59) | PPC_BIT(62)));

		/* Phase1 init step 13 (RX Config Mode Enable External Config Control) */
		write_scom_for_chiplet(chiplet, PEC_PCS_RX_CONFIG_MODE_REG, 0x8600);

		/* Phase1 init step 14 (PCLCK Control Register - PLLA) */
		/* ATTR_PROC_PCIE_PCS_PCLCK_CNTL_PLLA = 0xF8 */
		scom_and_or_for_chiplet(chiplet, PEC_PCS_PCLCK_CNTL_PLLA_REG,
					~PPC_BITMASK(56, 63),
					0xF8);

		/* Phase1 init step 15 (PCLCK Control Register - PLLB) */
		/* ATTR_PROC_PCIE_PCS_PCLCK_CNTL_PLLB = 0xF8 */
		scom_and_or_for_chiplet(chiplet, PEC_PCS_PCLCK_CNTL_PLLB_REG,
					~PPC_BITMASK(56, 63),
					0xF8);

		/* Phase1 init step 16 (TX DCLCK Rotator Override) */
		/* ATTR_PROC_PCIE_PCS_TX_DCLCK_ROT = 0x0022 */
		write_scom_for_chiplet(chiplet, PEC_PCS_TX_DCLCK_ROTATOR_REG, 0x0022);

		/* Phase1 init step 17 (TX PCIe Receiver Detect Control Register 1) */
		/* ATTR_PROC_PCIE_PCS_TX_PCIE_RECV_DETECT_CNTL_REG1 = 0xAA7A */
		write_scom_for_chiplet(chiplet, PEC_PCS_TX_PCIE_REC_DETECT_CNTL1_REG, 0xaa7a);

		/* Phase1 init step 18 (TX PCIe Receiver Detect Control Register 2) */
		/* ATTR_PROC_PCIE_PCS_TX_PCIE_RECV_DETECT_CNTL_REG2 = 0x2000 */
		write_scom_for_chiplet(chiplet, PEC_PCS_TX_PCIE_REC_DETECT_CNTL2_REG, 0x2000);

		/* Phase1 init step 19 (TX Power Sequence Enable) */
		/* ATTR_PROC_PCIE_PCS_TX_POWER_SEQ_ENABLE = 0xFF, but field is 7 bits */
		scom_and_or_for_chiplet(chiplet, PEC_PCS_TX_POWER_SEQ_ENABLE_REG,
					~PPC_BITMASK(56, 62),
					PPC_PLACE(0x7F, 56, 7));

		/* Phase1 init step 20 (RX VGA Control Register 1) */

		/* ATTR_PROC_PCIE_PCS_RX_VGA_CNTL_REG1 = 0 */
		val = 0;

		/* ATTR_CHIP_EC_FEATURE_HW414759 = 0, so not setting PEC_SCOM0X0B_EDMOD */

		write_scom_for_chiplet(chiplet, PEC_PCS_RX_VGA_CONTROL1_REG, val);

		/* Phase1 init step 21 (RX VGA Control Register 2) */
		/* ATTR_PROC_PCIE_PCS_RX_VGA_CNTL_REG2 = 0 */
		write_scom_for_chiplet(chiplet, PEC_PCS_RX_VGA_CONTROL2_REG, 0);

		/* Phase1 init step 22 (RX DFE Func Control Register 1) */
		/* ATTR_PROC_PCIE_PCS_RX_DFE_FDDC = 1 */
		scom_or_for_chiplet(chiplet, PEC_IOP_RX_DFE_FUNC_REGISTER1, PPC_BIT(50));

		/* Phase1 init step 23 (PCS System Control) */
		/* ATTR_PROC_PCIE_PCS_SYSTEM_CNTL computed above */
		scom_and_or_for_chiplet(chiplet, PEC_PCS_SYS_CONTROL_REG,
					~PPC_BITMASK(55, 63),
					pec_cfgs[pec]->phb_to_pcie_mac);

		/*
		 * All values in ATTR_PROC_PCIE_PCS_M_CNTL are 0.
		 * Hostboot has bugs here in that it updates PEC_PCS_M1_CONTROL_REG
		 * 4 times instead of updating 4 different registers (M1-M4).
		 */

		/* Phase1 init step 24 (PCS M1 Control) */
		scom_and_for_chiplet(chiplet, PEC_PCS_M1_CONTROL_REG, ~PPC_BITMASK(55, 63));
		/* Phase1 init step 25 (PCS M2 Control) */
		scom_and_for_chiplet(chiplet, PEC_PCS_M2_CONTROL_REG, ~PPC_BITMASK(55, 63));
		/* Phase1 init step 26 (PCS M3 Control) */
		scom_and_for_chiplet(chiplet, PEC_PCS_M3_CONTROL_REG, ~PPC_BITMASK(55, 63));
		/* Phase1 init step 27 (PCS M4 Control) */
		scom_and_for_chiplet(chiplet, PEC_PCS_M4_CONTROL_REG, ~PPC_BITMASK(55, 63));

		/* Delay a minimum of 200ns to allow prior SCOM programming to take effect */
		udelay(1);

		/* Phase1 init step 28 */
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_CLEAR,
				       PPC_BIT(PEC_IOP_PIPE_RESET_START_BIT));

		/*
		 * Delay a minimum of 300ns for reset to complete.
		 * Inherent delay before deasserting PCS PIPE Reset is enough here.
		 */
	}
}

void istep_10_10(uint8_t *phb_active_mask, uint8_t *iovalid_enable)
{
	const struct lane_config_row *pec_cfgs[MAX_PEC_PER_PROC] = { NULL };

	printk(BIOS_EMERG, "starting istep 10.10\n");
	report_istep(10,10);

	determine_lane_configs(phb_active_mask, pec_cfgs);

	/*
	 * Mask of functional PHBs for each PEC, ATTR_PROC_PCIE_IOVALID_ENABLE in Hostboot.
	 * LSB is the PHB with the highest number for the given PEC.
	 */
	iovalid_enable[0] = pec_cfgs[0]->phb_active >> PEC0_PHB_SHIFT;
	iovalid_enable[1] = pec_cfgs[1]->phb_active >> PEC1_PHB_SHIFT;
	iovalid_enable[2] = pec_cfgs[2]->phb_active >> PEC2_PHB_SHIFT;

	phase1(pec_cfgs, iovalid_enable);

	printk(BIOS_EMERG, "ending istep 10.10\n");
}
