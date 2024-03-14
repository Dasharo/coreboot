## SPDX-License-Identifier: GPL-2.0-only

all-$(CONFIG_SOC_INTEL_COMMON_BLOCK_OC_WDT) += oc_wdt.c

ramstage-$(CONFIG_SOC_INTEL_COMMON_OC_WDT_WDAT) += acpi.c

smm-$(CONFIG_SOC_INTEL_COMMON_BLOCK_OC_WDT) += oc_wdt.c
