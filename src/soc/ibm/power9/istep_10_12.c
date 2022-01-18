/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_10.h>

#include <console/console.h>
#include <cpu/power/scom.h>

#include "homer.h"
#include "pci.h"

/* PCIe only at the moment, Hostboot also updates MC and OBus chiplets too */
static void enable_ridi(uint8_t chip)
{
	enum {
		PERV_NET_CTRL0 = 0x000F0040,
		PERV_NET_CTRL0_WOR = 0x000F0042,
	};

	uint8_t pec = 0;

	for (pec = 0; pec < MAX_PEC_PER_PROC; ++pec) {
		chiplet_id_t chiplet = PCI0_CHIPLET_ID + pec;

		/* Getting NET_CTRL0 register value and checking its CHIPLET_ENABLE bit */
		if (read_rscom_for_chiplet(chip, chiplet, PERV_NET_CTRL0) & PPC_BIT(0)) {
			/* Enable Receivers, Drivers DI1 & DI2 */
			uint64_t val = 0;
			val |= PPC_BIT(19); // NET_CTRL0.RI_N = 1
			val |= PPC_BIT(20); // NET_CTRL0.DI1_N = 1
			val |= PPC_BIT(21); // NET_CTRL0.DI2_N = 1
			write_rscom_for_chiplet(chip, chiplet, PERV_NET_CTRL0_WOR, val);
		}
	}
}

void istep_10_12(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 10.12\n");
	report_istep(10,12);

	for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			enable_ridi(chip);
	}

	printk(BIOS_EMERG, "ending istep 10.12\n");
}
