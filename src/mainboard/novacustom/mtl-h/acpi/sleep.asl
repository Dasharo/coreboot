/* SPDX-License-Identifier: GPL-2.0-only */

#include <intelblocks/gpio.h>

Method (PGPM, 1, Serialized)
{
	For (Local0 = 0, Local0 < 6, Local0++)
	{
		\_SB.PCI0.CGPM (Local0, Arg0)
	}
}

/*
 * Method called from _PTS prior to system sleep state entry
 * Enables dynamic clock gating for all 5 GPIO communities
 */
Method (MPTS, 1, Serialized)
{
	PGPM (MISCCFG_GPIO_PM_CONFIG_BITS)

#if CONFIG(D3COLD_SUPPORT)
	/* Bring system out of TC cold before enter Sx */
	\_SB.PCI0.TCON ()

	/* Bring TBT group 0 and 1 out of D3 cold if it is in D3 cold */
	\_SB.PCI0.TG0N ()
	\_SB.PCI0.TG1N ()
#endif
}

/*
 * Method called from _WAK prior to system sleep state wakeup
 * Disables dynamic clock gating for all 5 GPIO communities
 */
Method (MWAK, 1, Serialized)
{
	PGPM (0)

	If (CondRefOf (\_SB.PCI0.TXHC)) {
		\_SB.TCWK (Arg0)
	}
}

/*
 * S0ix Entry/Exit Notifications
 * Called from \_SB.PEPD._DSM
 */
Method (MS0X, 1, Serialized)
{
	If (Arg0 == 1) {
		/* S0ix Entry */
		PGPM (MISCCFG_GPIO_PM_CONFIG_BITS)
	} Else {
		/* S0ix Exit */
		PGPM (0)
#if CONFIG(D3COLD_SUPPORT)
		/* Bring system out of TC cold  */
		\_SB.PCI0.TCON ()

		/* Bring TBT group 0 and 1 out of D3 cold if it is in D3 cold */
		\_SB.PCI0.TG0N ()
		\_SB.PCI0.TG1N ()
#endif
	}
}

/*
 * Display Notifications
 * Called from \_SB.PEPD._DSM
 */
Method (MSDX, 1, Serialized)
{
	If (CondRefOf (\_SB.PCI0.IMNG))
	{
		\_SB.PCI0.IMNG (Arg0)
	}
}
