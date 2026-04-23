/* SPDX-License-Identifier: GPL-2.0-only */

#include <intelblocks/gpio.h>

/*
 * Enable/disable dynamic clock gating for all GPIO communities.
 * Arg0 - MISCCFG_GPIO_PM_CONFIG_BITS to enable, 0 to disable.
 */
Method (PGPM, 1, Serialized)
{
	For (Local0 = 0, Local0 < 6, Local0++)
	{
		\_SB.PCI0.CGPM (Local0, Arg0)
	}
}

/* Method called from _PTS prior to enter sleep state */
Method (MPTS, 1, Serialized) {
	PGPM (MISCCFG_GPIO_PM_CONFIG_BITS)
	\_SB.SIO.PTS()
}

/* Method called from _WAK prior to wakeup */
Method (MWAK, 1, Serialized) {
	\_SB.SIO.WAK()
	PGPM (0)
	If (CondRefOf (\_SB.PCI0.TXHC)) {
		\_SB.TCWK (Arg0)
	}
}
