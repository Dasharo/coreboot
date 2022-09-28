
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/cpu.h>
#include <device/device.h>
#include <pc80/i8254.h>
#include <soc/gpio.h>
#include <soc/ramstage.h>
#include <smbios.h>
#include <stdlib.h>
#include <string.h>

#include "gpio.h"

const char *smbios_mainboard_product_name(void)
{
	u32 tmp[13];
	const char *str = "Unknown Processor Name";

	if (cpu_have_cpuid()) {
		int i;
		struct cpuid_result res = cpuid(0x80000000);
		if (res.eax >= 0x80000004) {
			int j = 0;
			for (i = 0; i < 3; i++) {
				res = cpuid(0x80000002 + i);
				tmp[j++] = res.eax;
				tmp[j++] = res.ebx;
				tmp[j++] = res.ecx;
				tmp[j++] = res.edx;
			}
			tmp[12] = 0;
			str = (const char *)tmp;
		}
	}

	if (strstr(str, "i3-10110U") != NULL)
		return "VP4630";
	else if (strstr(str, "i5-10210U") != NULL)
		return "VP4650";
	else if (strstr(str, "i7-10810U") != NULL)
		return "VP4670";
	else
		return CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME;
}

static void mainboard_final(void *unused)
{
	if (CONFIG(BEEP_ON_BOOT))
		beep(1500, 100);
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
