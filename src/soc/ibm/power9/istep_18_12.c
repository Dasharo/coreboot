/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_18.h>

#include <console/console.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>
#include <timer.h>

#define PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 38

#define PERV_TOD_ERROR_REG_RX_TTYPE_0 38

#define PERV_TOD_FSM_REG_IS_RUNNING 4
#define PERV_TOD_LOAD_TOD_MOD_REG_FSM_TRIGGER 0
#define PERV_TOD_START_TOD_REG_FSM_TRIGGER 0
#define PERV_TOD_TX_TTYPE_REG_TRIGGER 0

// TOD: TX TTYPE
// TX TType triggering register.
// [0]    TX_TTYPE_2_TRIGGER: TX TTYPE trigger.
#define PERV_TOD_TX_TTYPE_2_REG 0x00040013
#define PERV_TOD_TX_TTYPE_4_REG 0x00040015
#define PERV_TOD_TX_TTYPE_5_REG 0x00040016

// TOD: Load
// TOD-mod triggering register. This register sets the FSM in NOT_SET state.
// [0]    FSM_LOAD_TOD_MOD_TRIGGER: FSM: LOAD_TOD_MOD trigger.
#define PERV_TOD_LOAD_TOD_MOD_REG 0x00040018
// TOD: Load Register TOD Incrementer: 60
// Bit TOD and 4-bit WOF on read: Returns all 0s when the TOD is not running.
// On write: go to wait for sync state when data bit 6) = '0' (load TOD).
// Otherwise, go to stopped state (load TOD data63).
// [0-59]  LOAD_TOD_VALUE: Internal path: load TOD value.
// [60-63] WOF: who's-on-first (WOF) incrementer.
#define PERV_TOD_LOAD_TOD_REG 0x00040021
// TOD: Start TOD Triggering Register
// Goes to running state when data bit [02] = '0'.
// Otherwise, go to wait for sync state.
// [0]    FSM_START_TOD_TRIGGER: FSM: Start TOD trigger.
#define PERV_TOD_START_TOD_REG 0x00040022
// TOD: FSM Register
// [0:3]  RWX I_PATH_FSM_STATE: Internal path.
//        TOD FSM state (TOD is running in the following states:
//        x'02', x'0A', x'0E'). 0000 = Error.
// [4]    ROX TOD_IS_RUNNING: TOD running indicator.
#define PERV_TOD_FSM_REG 0x00040024
// TOD: Error and Interrupt Register
// [38] RWX RX_TTYPE_0: Status: received TType-0.
// [39] RWX RX_TTYPE_1: Status: received TType-1.
// [40] RWX RX_TTYPE_2: Status: received TType-2.
// [41] RWX RX_TTYPE_3: Status: received TType-3.
// [42] RWX RX_TTYPE_4: Status: received TType-4.
// [43] RWX RX_TTYPE_5: Status: received TType-5 when FSM is in running state.
#define PERV_TOD_ERROR_REG 0x00040030
// TOD: Error Mask Register Mask: Error Reporting Component (C_ERR_RPT)
// TOD: Error mask register mask of the error reporting component (c_err_rpt)
// This register holds masks for the same bits
// as in the previous (PERV_TOD_ERROR_REG) register
#define PERV_TOD_ERROR_MASK_REG 0x00040032

/* See istep 18.11 for 2 CPU topology diagram */

static void init_tod_node(uint8_t chips, uint8_t mdmt)
{
	uint8_t chip;

	/* Clear the TOD error register by writing all bits to 1 */
	/*
	 * Probably documentation issue, all bits in this register are described as
	 * RW, but code treats them as if they were write-1-to-clear.
	 */
	for (chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			write_scom(chip, PERV_TOD_ERROR_REG, ~0);
	}

	/* Configure MDMT */

	/* Chip TOD step checkers enable */
	write_scom(mdmt, PERV_TOD_TX_TTYPE_2_REG,
		   PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));

	/* Switch local Chip TOD to 'Not Set' state */
	write_scom(mdmt, PERV_TOD_LOAD_TOD_MOD_REG,
		   PPC_BIT(PERV_TOD_LOAD_TOD_MOD_REG_FSM_TRIGGER));

	/* Switch all Chip TOD in the system to 'Not Set' state */
	write_scom(mdmt, PERV_TOD_TX_TTYPE_5_REG,
		   PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));

	/* Chip TOD load value (move TB to TOD) */
	write_scom(mdmt, PERV_TOD_LOAD_TOD_REG,
		   PPC_PLACE(0x3FF, 0, 60) | PPC_PLACE(0xC, 60, 4));

	/* Chip TOD start_tod (switch local Chip TOD to 'Running' state) */
	write_scom(mdmt, PERV_TOD_START_TOD_REG,
		   PPC_BIT(PERV_TOD_START_TOD_REG_FSM_TRIGGER));

	/* Send local Chip TOD value to all Chip TODs */
	write_scom(mdmt, PERV_TOD_TX_TTYPE_4_REG,
		   PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));

	/* In case of larger topology, replace loops with a recursion */
	for (chip = 0; chip < MAX_CHIPS; chip++) {
		uint64_t error_reg;

		if (!(chips & (1 << chip)))
			continue;

		/* Wait until TOD is running */
		if (!wait_us(1000, read_scom(chip, PERV_TOD_FSM_REG) &
				   PPC_BIT(PERV_TOD_FSM_REG_IS_RUNNING))) {
			printk(BIOS_ERR, "PERV_TOD_ERROR_REG = %#16.16llx\n",
			       read_scom(chip, PERV_TOD_ERROR_REG));
			die("Error on chip#%d: TOD is not running!\n", chip);
		}

		/* Clear TTYPE#2, TTYPE#4, and TTYPE#5 status */
		write_scom(chip, PERV_TOD_ERROR_REG,
			   PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 2) |
			   PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 4) |
			   PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 5));

		/* Check for real errors */
		error_reg = read_scom(chip, PERV_TOD_ERROR_REG);
		if (error_reg != 0) {
			printk(BIOS_ERR, "PERV_TOD_ERROR_REG = %#16.16llx\n", error_reg);
			die("Error: TOD initialization failed!\n");
		}

		/* Set error mask to runtime configuration (mask TTYPE informational bits) */
		write_scom(chip, PERV_TOD_ERROR_MASK_REG,
			   PPC_BITMASK(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0,
				       PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 + 5));
	}
}

void istep_18_12(uint8_t chips, uint8_t mdmt)
{
	report_istep(18, 12);
	init_tod_node(chips, mdmt);
}
