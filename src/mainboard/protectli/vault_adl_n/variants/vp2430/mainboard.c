/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <console/console.h>
#include <device/device.h>
#include <pc80/i8254.h>
#include <smbios.h>
#include <soc/ramstage.h>

#include <string.h>

const char *smbios_mainboard_product_name(void)
{
	return CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME;
}

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	memset(params->PcieRpEnableCpm, 0, sizeof(params->PcieRpEnableCpm));
	memset(params->PcieRpPmSci, 0, sizeof(params->PcieRpPmSci));

	memset(params->CpuPcieRpEnableCpm, 0, sizeof(params->CpuPcieRpEnableCpm));
	memset(params->CpuPcieClockGating, 0, sizeof(params->CpuPcieClockGating));
	memset(params->CpuPciePowerGating, 0, sizeof(params->CpuPciePowerGating));
	memset(params->CpuPcieRpPmSci, 0, sizeof(params->CpuPcieRpPmSci));

	// Max payload 256B
	memset(params->PcieRpMaxPayload, 1, sizeof(params->PcieRpMaxPayload));

	// CLKREQs connected only to RP3 and RP7
	params->PcieRpEnableCpm[2] = 1;
	params->PcieRpEnableCpm[6] = 1;

	// Enable port reset message on Type-C ports
	params->PortResetMessageEnable[4] = 1;
	params->PortResetMessageEnable[5] = 1;

	// IOM USB config
	params->PchUsbOverCurrentEnable = 0;
}

static void mainboard_final(void *chip_info)
{
	if (CONFIG(BEEP_ON_BOOT))
		beep(1500, 100);
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
