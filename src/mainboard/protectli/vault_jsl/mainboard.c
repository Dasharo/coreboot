/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/mmio.h>
#include <device/device.h>
#include <pc80/i8254.h>
#include <soc/iomap.h>
#include <soc/ramstage.h>

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	params->UsbPdoProgramming = 0;

	params->PcieRpAcsEnabled[0] = 1;
	params->PcieRpAcsEnabled[1] = 1;
	params->PcieRpAcsEnabled[3] = 1;
	params->PcieRpAcsEnabled[4] = 1;
	params->PcieRpAcsEnabled[5] = 1;
	params->PcieRpAcsEnabled[6] = 1;

	params->PcieRpAdvancedErrorReporting[0] = 1;
	params->PcieRpAdvancedErrorReporting[1] = 1;
	params->PcieRpAdvancedErrorReporting[3] = 1;
	params->PcieRpAdvancedErrorReporting[4] = 0; // Atheros WiFi cards don't like AER
	params->PcieRpAdvancedErrorReporting[5] = 1;
	params->PcieRpAdvancedErrorReporting[6] = 1;

	params->PcieRpLtrEnable[0] = 1;
	params->PcieRpLtrEnable[1] = 1;
	params->PcieRpLtrEnable[3] = 1;
	params->PcieRpLtrEnable[4] = 1;
	params->PcieRpLtrEnable[5] = 1;
	params->PcieRpLtrEnable[6] = 1;

	/* Set Max Payload to 256B */
	params->PcieRpMaxPayload[0] = 1;
	params->PcieRpMaxPayload[1] = 1;
	params->PcieRpMaxPayload[3] = 1;
	params->PcieRpMaxPayload[4] = 1;
	params->PcieRpMaxPayload[5] = 1;
	params->PcieRpMaxPayload[6] = 1;

	params->PcieRpSlotImplemented[3] = 1;
	params->PcieRpSlotImplemented[4] = 1;

	params->PcieRpDetectTimeoutMs[1] = 200;

	/*
	 * HWP is too aggressive in power savings and does not let using full
	 * bandwidth of Ethernet controllers without additional stressing of
	 * the CPUs (2Gb/s vs 2.35Gb/s with stressing, measured with iperf3).
	 * Let the Linux use acpi-cpufreq governor driver instead of
	 * intel_pstate by disabling HWP.
	 */
	params->Hwp = 0;
}

static void mainboard_final(void *unused)
{
	if (CONFIG(BEEP_ON_BOOT))
		beep(1500, 100);
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
