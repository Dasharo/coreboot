/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_18.h>
#include <timer.h>

/* TODO: this one will change if we add more than one processor */
#define MDMT (1)

/* TODO: this will be much more complicated for different topology */
static void init_tod_node(void)
{
	uint64_t error_reg;

	/* Clear the TOD error register by writing all bits to 1 */
	/*
	 * Probably documentation issue, all bits in this register are described as
	 * RW, but code treats them as if they were write-1-to-clear.
	 */
	write_scom(PERV_TOD_ERROR_REG, ~0);

	/* Assumption: node is MDMT */
	if (MDMT) {
		/* Chip TOD step checkers enable */
		write_scom(PERV_TOD_TX_TTYPE_2_REG,
		           PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));

		/* Switch local Chip TOD to 'Not Set' state */
		write_scom(PERV_TOD_LOAD_TOD_MOD_REG,
				   PPC_BIT(PERV_TOD_LOAD_TOD_MOD_REG_FSM_TRIGGER));

		/* Switch all Chip TOD in the system to 'Not Set' state */
		write_scom(PERV_TOD_TX_TTYPE_5_REG,
		           PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));

		/* Chip TOD load value (move TB to TOD) */
		write_scom(PERV_TOD_LOAD_TOD_REG,
		           PPC_PLACE(0x3FF, 0, 60) | PPC_PLACE(0xC, 60, 4));

		/* Chip TOD start_tod (switch local Chip TOD to 'Running' state) */
		write_scom(PERV_TOD_START_TOD_REG,
		           PPC_BIT(PERV_TOD_START_TOD_REG_FSM_TRIGGER));

		/* Send local Chip TOD value to all Chip TODs */
		write_scom(PERV_TOD_TX_TTYPE_4_REG,
		           PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));
	}

	/* Wait until TOD is running */
	if (!wait_us(1000,
	             read_scom(PERV_TOD_FSM_REG) & PPC_BIT(PERV_TOD_FSM_REG_IS_RUNNING))) {
		printk(BIOS_ERR, "PERV_TOD_ERROR_REG = %#16.16llx\n",
		       read_scom(PERV_TOD_ERROR_REG));
		die("Error: TOD is not running!\n");
	}

	/* Clear TTYPE#2, TTYPE#4, and TTYPE#5 status */
	write_scom(PERV_TOD_ERROR_REG,
	           PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 2) |
	           PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 4) |
	           PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 5));

	/* Check for real errors */
	error_reg = read_scom(PERV_TOD_ERROR_REG);
	if (error_reg != 0) {
		printk(BIOS_ERR, "PERV_TOD_ERROR_REG = %#16.16llx\n",
		       read_scom(PERV_TOD_ERROR_REG));
		die("Error: TOD initialization failed!\n");
	}

	/* Set error mask to runtime configuration (mask TTYPE informational bits) */
	write_scom(PERV_TOD_ERROR_MASK_REG,
	           PPC_BITMASK(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0,
	                       PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 + 5));

	/* In case of multinode system, configure child nodes recursively here */
}

void istep_18_12(void)
{
	printk(BIOS_EMERG, "starting istep 18.12\n");
	report_istep(18,12);
	init_tod_node();
	printk(BIOS_EMERG, "ending istep 18.12\n");
}
