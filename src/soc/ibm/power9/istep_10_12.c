/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_10.h>

#include <console/console.h>
#include <cpu/power/scom.h>

#include "pci.h"

/* PCIe only at the moment, should also do other buses */
static void enable_ridi(void)
{
	enum {
		PERV_NET_CTRL0 = 0x000F0040,
		PERV_NET_CTRL0_WOR = 0x000F0042,
	};

	uint8_t pec = 0;

	for (pec = 0; pec < MAX_PEC_PER_PROC; ++pec) {
		chiplet_id_t chiplet = PCI0_CHIPLET_ID + pec;

		/* Getting NET_CTRL0 register value and checking its CHIPLET_ENABLE bit */
		if (read_scom_for_chiplet(chiplet, PERV_NET_CTRL0) & PPC_BIT(0)) {
			/* Enable Recievers, Drivers DI1 & DI2 */
			uint64_t val = 0;
			val |= PPC_BIT(19); // NET_CTRL0.RI_N = 1
			val |= PPC_BIT(20); // NET_CTRL0.DI1_N = 1
			val |= PPC_BIT(21); // NET_CTRL0.DI2_N = 1
			write_scom_for_chiplet(chiplet, PERV_NET_CTRL0_WOR, val);
		}
	}
}

void istep_10_12(void)
{
	printk(BIOS_EMERG, "starting istep 10.12\n");
	report_istep(10,12);

	enable_ridi();

	printk(BIOS_EMERG, "ending istep 10.12\n");
}
