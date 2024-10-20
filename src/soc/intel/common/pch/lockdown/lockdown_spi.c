/* SPDX-License-Identifier: GPL-2.0-only */

#include <intelblocks/cfg.h>
#include <intelblocks/fast_spi.h>
#include <intelpch/lockdown.h>

void fast_spi_lockdown_bios(int chipset_lockdown)
{
	if (!CONFIG(SOC_INTEL_COMMON_BLOCK_FAST_SPI))
		return;

	/* Discrete Lock Flash PR registers */
	fast_spi_pr_dlock();

	/* Lock FAST_SPIBAR */
	fast_spi_lock_bar();

	/* Set BIOS Interface Lock, BIOS Lock */
	if (chipset_lockdown == CHIPSET_LOCKDOWN_COREBOOT) {
		/* BIOS Interface Lock */
		fast_spi_set_bios_interface_lock_down();

		/* Only allow writes in SMM */
		if (CONFIG(BOOTMEDIA_SMM_BWP)) {
			fast_spi_set_eiss();
			fast_spi_enable_wp();
		}

		/* BIOS Lock */
		fast_spi_set_lock_enable();

		/* EXT BIOS Lock */
		fast_spi_set_ext_bios_lock_enable();
	}
}
