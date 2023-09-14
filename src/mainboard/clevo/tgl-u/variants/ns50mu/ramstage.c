/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/ramstage.h>
#include <variant/ramstage.h>

void variant_configure_fsps(FSP_S_CONFIG *params)
{
	/*
	 * Disable AER for the SSD slot to support S0ix with SSDs running
	 * buggy firmware
	 */
	params->CpuPcieRpAdvancedErrorReporting[0] = 0;
	params->CpuPcieRpLtrEnable[0] = 1;
	params->CpuPcieRpSlotImplemented[0] = 1;
}
