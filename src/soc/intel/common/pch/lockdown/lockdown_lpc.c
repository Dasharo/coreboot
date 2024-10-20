/* SPDX-License-Identifier: GPL-2.0-only */

#include <intelblocks/cfg.h>
#include <intelblocks/lpc_lib.h>
#include <intelpch/lockdown.h>

void lpc_lockdown_config(int chipset_lockdown)
{
	/* Set BIOS Interface Lock, BIOS Lock */
	if (chipset_lockdown == CHIPSET_LOCKDOWN_COREBOOT) {
		/* BIOS Interface Lock */
		lpc_set_bios_interface_lock_down();

		/* Only allow writes in SMM */
		if (CONFIG(BOOTMEDIA_SMM_BWP)) {
			lpc_set_eiss();
			lpc_enable_wp();
		}

		/* BIOS Lock */
		lpc_set_lock_enable();
	}
}
