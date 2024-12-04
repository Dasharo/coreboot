/* SPDX-License-Identifier: GPL-2.0-only */

#include <intelblocks/cfg.h>
#include <intelblocks/lpc_lib.h>
#include <intelpch/lockdown.h>
#include <soc/lockdown.h>
#include <soc/pm.h>

void soc_lockdown_config(int chipset_lockdown)
{
	if (chipset_lockdown == CHIPSET_LOCKDOWN_FSP)
		return;

	if (!CONFIG(SOC_INTEL_COMMON_SPI_LOCKDOWN_SMM))
		/* LPC/eSPI lock down configuration */
		lpc_lockdown_config(chipset_lockdown);

	pmc_lockdown_config();
	sata_lockdown_config(chipset_lockdown);
	spi_lockdown_config(chipset_lockdown);
}
