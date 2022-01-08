/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_9.h>

#include <console/console.h>
#include <cpu/power/scom.h>

#include "xbus.h"

static void smp_link_layer(uint8_t chip)
{
	enum {
		/* dl_control_addr */
		XBUS_LL1_IOEL_CONTROL = 0x0000000006011C0B,

		XBUS_LL0_IOEL_CONTROL_LINK0_STARTUP = 1,
		XBUS_LL0_IOEL_CONTROL_LINK1_STARTUP = 33,
	};

	/* Hostboot uses PUTSCOMMASK operation of SBE IO. Assuming that it's
	 * equivalent to a RMW sequence. */
	or_scom(chip, XBUS_LL1_IOEL_CONTROL,
		PPC_BIT(XBUS_LL0_IOEL_CONTROL_LINK0_STARTUP) |
		PPC_BIT(XBUS_LL0_IOEL_CONTROL_LINK1_STARTUP));
}

void istep_9_6(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 9.6\n");
	report_istep(9,6);

	if (chips != 0x01) {
		smp_link_layer(/*chip=*/0);
		smp_link_layer(/*chip=*/1);
	}

	printk(BIOS_EMERG, "ending istep 9.6\n");
}
