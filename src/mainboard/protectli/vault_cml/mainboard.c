
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/cpu.h>
#include <chip.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <pc80/i8254.h>
#include <soc/gpio.h>
#include <soc/pci_devs.h>
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

static void mainboard_enable(struct device *dev)
{
	struct soc_power_limits_config *soc_conf;
	config_t *conf = config_of_soc();
	uint16_t sa_devid = pci_read_config16(pcidev_on_root(0, 0), PCI_DEVICE_ID);

	soc_conf = &conf->power_limits_config;

	if (sa_devid == PCI_DID_INTEL_CML_ULT_2_2) {
		soc_conf->tdp_pl2_override = 35;
		soc_conf->tdp_pl4 = 51;
	} else if (sa_devid == PCI_DID_INTEL_CML_ULT ||
		   sa_devid == PCI_DID_INTEL_CML_ULT_6_2) {
		soc_conf->tdp_pl2_override = 64;
		soc_conf->tdp_pl4 = 90;
	}
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};

void mainboard_silicon_init_params(FSPS_UPD *supd)
{
	supd->FspsTestConfig.C1StateAutoDemotion = 0;
	supd->FspsTestConfig.C1StateUnDemotion = 0;
	supd->FspsTestConfig.C3StateAutoDemotion = 0;
	supd->FspsTestConfig.C3StateUnDemotion = 0;
}