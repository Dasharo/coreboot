/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_INTEL_COMMON_OC_WDT_H
#define SOC_INTEL_COMMON_OC_WDT_H

#include <stdbool.h>

/* OC WDT configuration */
#define PCH_OC_WDT_CTL				(ACPI_BASE_ADDRESS + 0x54)
#define   PCH_OC_WDT_CTL_RLD			BIT(31)
#define   PCH_OC_WDT_CTL_ICCSURV_STS		BIT(25)
#define   PCH_OC_WDT_CTL_NO_ICCSURV_STS		BIT(24)
#define   PCH_OC_WDT_CTL_SCRATCH_OFFSET		0x10
#define   PCH_OC_WDT_CTL_SCRATCH_MASK		(0xFF << PCH_OC_WDT_CTL_SCRATCH_OFFSET)
#define   PCH_OC_WDT_CTL_FORCE_ALL		BIT(15)
#define   PCH_OC_WDT_CTL_EN			BIT(14)
#define   PCH_OC_WDT_CTL_ICCSURV		BIT(13)
#define   PCH_OC_WDT_CTL_LCK			BIT(12)
#define   PCH_OC_WDT_CTL_TOV_MASK		0x3FF

/* Starts and reloads the OC watchdog if enabled in Kconfig */
void setup_oc_wdt(void);

/* Reloads the OC watchdog (if enabled) preserving the current settings. */
void oc_wdt_reload(void);

/* Returns currently programmed OC watchdog timeout in seconds */
unsigned int oc_wdt_get_current_timeout(void);

#endif
