/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_8.h>

#include <console/console.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>

#include "xbus.h"

static void xbus_enable_ridi(uint8_t chip)
{
	enum {
		PERV_NET_CTRL0 = 0x060F0040,
		PERV_NET_CTRL0_WOR = 0x060F0042,
	};

	/* Getting NET_CTRL0 register value and checking its CHIPLET_ENABLE bit */
	if (read_scom(chip, xbus_addr(PERV_NET_CTRL0)) & PPC_BIT(0)) {
		/* Enable Receivers, Drivers DI1 & DI2 */
		uint64_t val = 0;
		val |= PPC_BIT(19); // NET_CTRL0.RI_N = 1
		val |= PPC_BIT(20); // NET_CTRL0.DI1_N = 1
		val |= PPC_BIT(21); // NET_CTRL0.DI2_N = 1
		write_scom(chip, xbus_addr(PERV_NET_CTRL0_WOR), val);
	}
}

void istep_8_11(uint8_t chips)
{
	uint8_t chip;

	report_istep(8,11);

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			xbus_enable_ridi(chip);
	}
}
