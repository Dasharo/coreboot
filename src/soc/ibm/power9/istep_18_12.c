/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_18.h>
#include <cpu/power/proc.h>
#include <timer.h>

/* See istep 18.11 for 2CPU topology topology */

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
			write_rscom(chip, PERV_TOD_ERROR_REG, ~0);
	}

	/* Configure MDMT */

	/* Chip TOD step checkers enable */
	write_rscom(mdmt, PERV_TOD_TX_TTYPE_2_REG,
			PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));

	/* Switch local Chip TOD to 'Not Set' state */
	write_rscom(mdmt, PERV_TOD_LOAD_TOD_MOD_REG,
			PPC_BIT(PERV_TOD_LOAD_TOD_MOD_REG_FSM_TRIGGER));

	/* Switch all Chip TOD in the system to 'Not Set' state */
	write_rscom(mdmt, PERV_TOD_TX_TTYPE_5_REG,
			PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));

	/* Chip TOD load value (move TB to TOD) */
	write_rscom(mdmt, PERV_TOD_LOAD_TOD_REG,
			PPC_PLACE(0x3FF, 0, 60) | PPC_PLACE(0xC, 60, 4));

	/* Chip TOD start_tod (switch local Chip TOD to 'Running' state) */
	write_rscom(mdmt, PERV_TOD_START_TOD_REG,
			PPC_BIT(PERV_TOD_START_TOD_REG_FSM_TRIGGER));

	/* Send local Chip TOD value to all Chip TODs */
	write_rscom(mdmt, PERV_TOD_TX_TTYPE_4_REG,
			PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));

	/* In case of larger topology, replace loops with a recursion */
	for (chip = 0; chip < MAX_CHIPS; chip++) {
		uint64_t error_reg;

		if (!(chips & (1 << chip)))
			continue;

		/* Wait until TOD is running */
		if (!wait_us(1000, read_rscom(chip, PERV_TOD_FSM_REG) &
				   PPC_BIT(PERV_TOD_FSM_REG_IS_RUNNING))) {
			printk(BIOS_ERR, "PERV_TOD_ERROR_REG = %#16.16llx\n",
			       read_rscom(chip, PERV_TOD_ERROR_REG));
			die("Error on chip#%d: TOD is not running!\n", chip);
		}

		/* Clear TTYPE#2, TTYPE#4, and TTYPE#5 status */
		write_rscom(chip, PERV_TOD_ERROR_REG,
			    PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 2) |
			    PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 4) |
			    PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 5));

		/* Check for real errors */
		error_reg = read_rscom(chip, PERV_TOD_ERROR_REG);
		if (error_reg != 0) {
			printk(BIOS_ERR, "PERV_TOD_ERROR_REG = %#16.16llx\n", error_reg);
			die("Error: TOD initialization failed!\n");
		}

		/* Set error mask to runtime configuration (mask TTYPE informational bits) */
		write_rscom(chip, PERV_TOD_ERROR_MASK_REG,
			    PPC_BITMASK(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0,
					PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 + 5));
	}
}

void istep_18_12(uint8_t chips, uint8_t mdmt)
{
	printk(BIOS_EMERG, "starting istep 18.12\n");
	report_istep(18,12);
	init_tod_node(chips, mdmt);
	printk(BIOS_EMERG, "ending istep 18.12\n");
}
