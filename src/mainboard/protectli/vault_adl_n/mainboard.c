/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/x86/name.h>
#include <device/device.h>
#include <pc80/i8254.h>
#include <smbios.h>
#include <soc/ramstage.h>
#include <string.h>

const char *smbios_mainboard_product_name(void)
{
	char processor_name[49];

	fill_processor_name(processor_name);

	if (strstr(processor_name, "N100") != NULL)
		return "VP3210";
	else if (strstr(processor_name, "N305") != NULL)
		return "VP3230";
	else
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

	/* Max payload 256B */
	memset(params->PcieRpMaxPayload, 1, sizeof(params->PcieRpMaxPayload));

	/*
	 * CLKREQs connected only to RP3 and RP7, but other CLKREQs are
	 * pulled to GND, So it should be fine to enable CPM on all RPs.
	 */
	params->PcieRpEnableCpm[0] = 1;
	params->PcieRpEnableCpm[2] = 1;
	params->PcieRpEnableCpm[4] = 1;
	params->PcieRpEnableCpm[6] = 1;
	params->PcieRpEnableCpm[9] = 1;

	/* Enable port reset message on Type-C ports */
	params->PortResetMessageEnable[4] = 1;
	params->PortResetMessageEnable[5] = 1;

	/*
	 * Configure AUX bias pads in FPS-S, because coreboot would do it too
	 * late and cause the Type-C displays to not work.
	 */
	params->IomTypeCPortPadCfg[0] = 0x09020016; /* GPP_A22 */
	params->IomTypeCPortPadCfg[1] = 0x09020015; /* GPP_A21 */
	params->IomTypeCPortPadCfg[2] = 0x0902000F; /* GPP_A15 */
	params->IomTypeCPortPadCfg[3] = 0x0902000E; /* GPP_A14 */

	/* PMC-PD controller */
	params->PmcPdEnable = 1;
	/* IOM USB config */
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
