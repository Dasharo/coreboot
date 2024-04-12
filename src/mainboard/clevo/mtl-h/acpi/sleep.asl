/* SPDX-License-Identifier: GPL-2.0-only */

#include <intelblocks/gpio.h>

/*
 * Method called from _PTS prior to system sleep state entry
 */
Method (MPTS, 1, Serialized)
{
	/* Bring system out of TC cold before enter Sx */
	\_SB.PCI0.TCON ()

	/* Bring TBT group 0 and 1 out of D3 cold if it is in D3 cold */
	\_SB.PCI0.TG0N ()
	\_SB.PCI0.TG1N ()
}

/*
 * Method called from _WAK prior to system sleep state wakeup
 */
Method (MWAK, 1, Serialized)
{

}

/*
 * S0ix Entry/Exit Notifications
 * Called from \_SB.PEPD._DSM
 */
Method (MS0X, 1, Serialized)
{
	If (Arg0 == 1) {

	} Else {
		/* Bring system out of TC cold  */
		\_SB.PCI0.TCON ()

		/* Bring TBT group 0 and 1 out of D3 cold if it is in D3 cold */
		\_SB.PCI0.TG0N ()
		\_SB.PCI0.TG1N ()
	}
}
