/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_8.h>

#include <console/console.h>
#include <cpu/power/scom.h>

static void xbus_enable_ridi(uint8_t chip)
{
	enum {
		PERV_NET_CTRL0 = 0x060F0040,
		PERV_NET_CTRL0_WOR = 0x060F0042,
	};

	/* Getting NET_CTRL0 register value and checking its CHIPLET_ENABLE bit */
	if (get_scom(chip, PERV_NET_CTRL0) & PPC_BIT(0)) {
		/* Enable Recievers, Drivers DI1 & DI2 */
		uint64_t val = 0;
		val |= PPC_BIT(19); // NET_CTRL0.RI_N = 1
		val |= PPC_BIT(20); // NET_CTRL0.DI1_N = 1
		val |= PPC_BIT(21); // NET_CTRL0.DI2_N = 1
		put_scom(chip, PERV_NET_CTRL0_WOR, val);
	}
}

void istep_8_11(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 8.11\n");
	report_istep(8,11);

	if (chips != 0x01) {
		xbus_enable_ridi(/*chip=*/0);
		xbus_enable_ridi(/*chip=*/1);
	}

	printk(BIOS_EMERG, "ending istep 8.11\n");
}
