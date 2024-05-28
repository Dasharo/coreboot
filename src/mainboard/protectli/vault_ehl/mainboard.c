/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <fsp/api.h>
#include <pc80/i8254.h>
#include <soc/ramstage.h>

static void mainboard_final(void *chip_info)
{
	beep(1500, 200);
}

void mainboard_silicon_init_params(FSP_S_CONFIG *silconfig)
{
	/*
	 * HWP is too aggressive in power savings and does not let using full
	 * bandwidth of Ethernet controllers without additional stressing of
	 * the CPUs (2Gb/s vs 2.35Gb/s with stressing, measured with iperf3).
	 * Let the Linux use acpi-cpufreq governor driver instead of
	 * intel_pstate by disabling HWP.
	 */
	silconfig->Hwp = 0;
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
