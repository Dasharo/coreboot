/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_9.h>

#include <console/console.h>
#include <cpu/power/scom.h>
#include <delay.h>
#include <stdbool.h>

#include "xbus.h"

static void p9_fab_iovalid_link_validate(uint8_t chip)
{
	enum {
		XBUS_LL1_IOEL_FIR_REG = 0x06011C00,
		DL_FIR_LINK0_TRAINED_BIT = 0,
		DL_FIR_LINK1_TRAINED_BIT = 1,
	};

	int i;

	for (i = 0; i < 100; ++i) {
		/* Only OBus seems to be retrained, so this XBus-only code is
		 * much simpler compared to corresponding code in Hostboot */

		uint64_t dl_fir_reg = read_scom(chip, xbus_addr(XBUS_LL1_IOEL_FIR_REG));

		bool dl_trained = (dl_fir_reg & PPC_BIT(DL_FIR_LINK0_TRAINED_BIT))
			       && (dl_fir_reg & PPC_BIT(DL_FIR_LINK1_TRAINED_BIT));
		if (dl_trained)
			break;

		mdelay(1);
	}

	if (i == 100)
		die("XBus link DL training failed\n");
}

static void p9_fab_iovalid(uint8_t chip)
{
	enum {
		PERV_XB_CPLT_CONF1_OR = 0x06000019,
		PERV_CPLT_CONF1_IOVALID_6D = 6,

		PU_PB_CENT_SM0_PB_CENT_FIR_REG = 0x05011C00,
		PU_PB_CENT_SM0_PB_CENT_FIR_MASK_REG_SPARE_13 = 13,

		PU_PB_CENT_SM1_EXTFIR_ACTION0_REG = 0x05011C34,
		PU_PB_CENT_SM1_EXTFIR_ACTION1_REG = 0x05011C35,

		PU_PB_CENT_SM1_EXTFIR_MASK_REG_AND = 0x05011C32,
	};

	uint64_t fbc_cent_fir_data;

	p9_fab_iovalid_link_validate(chip);

	/* Clear RAS FIR mask for link if not already set up by SBE */
	fbc_cent_fir_data = read_scom(chip, PU_PB_CENT_SM0_PB_CENT_FIR_REG);
	if (!(fbc_cent_fir_data & PPC_BIT(PU_PB_CENT_SM0_PB_CENT_FIR_MASK_REG_SPARE_13))) {
		scom_and(chip, PU_PB_CENT_SM1_EXTFIR_ACTION0_REG,
			 ~PPC_BIT(PERV_CPLT_CONF1_IOVALID_6D));
		scom_and(chip, PU_PB_CENT_SM1_EXTFIR_ACTION1_REG,
			 ~PPC_BIT(PERV_CPLT_CONF1_IOVALID_6D));
		write_scom(chip, PU_PB_CENT_SM1_EXTFIR_MASK_REG_AND,
			   ~PPC_BIT(PERV_CPLT_CONF1_IOVALID_6D));
	}

	/*
	 * Use AND/OR mask registers to atomically update link specific fields
	 * in iovalid control register.
	 */
	write_scom(chip, xbus_addr(PERV_XB_CPLT_CONF1_OR),
		   PPC_BIT(PERV_CPLT_CONF1_IOVALID_6D) |
		   PPC_BIT(PERV_CPLT_CONF1_IOVALID_6D + 1));
}

void istep_9_7(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 9.7\n");
	report_istep(9,7);

	if (chips != 0x01) {
		/*
		 * Add delay for DD1.1+ procedure to compensate for lack of lane
		 * lock polls.
		 *
		 * HB does this inside p9_fab_iovalid(), which doubles the
		 * delay, which is probably unnecessary.
		 */
		mdelay(100);

		p9_fab_iovalid(/*chip=*/0);
		p9_fab_iovalid(/*chip=*/1);
	}

	printk(BIOS_EMERG, "ending istep 9.7\n");
}
