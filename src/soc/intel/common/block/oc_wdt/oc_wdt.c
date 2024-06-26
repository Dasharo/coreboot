/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <console/console.h>
#include <dasharo/options.h>
#include <intelblocks/oc_wdt.h>
#include <soc/iomap.h>
#include <types.h>

static struct watchdog_config wdt_config;
/*
 * Starts and reloads the OC watchdog with given timeout.
 *
 * timeout - Time in seconds before OC watchdog times out. Supported range = 70 - 1024
 */
static void oc_wdt_start(unsigned int timeout)
{
	uint32_t oc_wdt_ctrl;
	bool bootstatus;

	if ((timeout < 70) || (timeout > (PCH_OC_WDT_CTL_TOV_MASK + 1))) {
		timeout = CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT_SECONDS;
		printk(BIOS_WARNING, "OC Watchdog: invalid timeout value,"
				     " using config default: %ds\n", timeout);
	}

	printk(BIOS_SPEW, "OC Watchdog: start and reload timer (timeout %ds)\n", timeout);

	oc_wdt_ctrl = inl(PCH_OC_WDT_CTL);

	bootstatus = !!(oc_wdt_ctrl & (PCH_OC_WDT_CTL_ICCSURV_STS | PCH_OC_WDT_CTL_NO_ICCSURV_STS));
	if (bootstatus)
		printk(BIOS_ALERT, "OC Watchdog: Last reset caused by timeout!\n");

	/* Store bootstatus in scratch register to be consumed by WDAT */
	oc_wdt_ctrl &= ~PCH_OC_WDT_CTL_SCRATCH_MASK;
	oc_wdt_ctrl |= bootstatus << PCH_OC_WDT_CTL_SCRATCH_OFFSET;

	oc_wdt_ctrl |= (PCH_OC_WDT_CTL_EN | PCH_OC_WDT_CTL_FORCE_ALL | PCH_OC_WDT_CTL_ICCSURV);

	oc_wdt_ctrl &= ~PCH_OC_WDT_CTL_TOV_MASK;
	oc_wdt_ctrl |= (timeout - 1);
	oc_wdt_ctrl |= PCH_OC_WDT_CTL_RLD;

	outl(oc_wdt_ctrl, PCH_OC_WDT_CTL);
}

/* Checks if OC WDT is enabled and returns true if so, otherwise false. */
static bool is_oc_wdt_enabled(void)
{
	return (inl(PCH_OC_WDT_CTL) & PCH_OC_WDT_CTL_EN) ? true : false;
}

/* Reloads the OC watchdog (if enabled) preserving the current settings. */
void oc_wdt_reload(void)
{
	uint32_t oc_wdt_ctrl;

	/* Reload only works if OC WDT enable bit is set */
	if (!is_oc_wdt_enabled())
		return;

	oc_wdt_ctrl = inl(PCH_OC_WDT_CTL);
	/* Unset write-1-to-clear bits and preserve other settings */
	oc_wdt_ctrl &= ~(PCH_OC_WDT_CTL_ICCSURV_STS | PCH_OC_WDT_CTL_NO_ICCSURV_STS);
	oc_wdt_ctrl |= PCH_OC_WDT_CTL_RLD;
	outl(oc_wdt_ctrl, PCH_OC_WDT_CTL);
}

/* Disables the OC WDT. */
static void oc_wdt_disable(void)
{
	uint32_t oc_wdt_ctrl;

	printk(BIOS_INFO, "OC Watchdog: disabling watchdog timer\n");

	oc_wdt_ctrl = inl(PCH_OC_WDT_CTL);
	oc_wdt_ctrl &= ~(PCH_OC_WDT_CTL_EN | PCH_OC_WDT_CTL_FORCE_ALL);
	outl(oc_wdt_ctrl, PCH_OC_WDT_CTL);
}

/* Returns currently programmed OC watchdog timeout in seconds */
unsigned int oc_wdt_get_current_timeout(void)
{
	return (inl(PCH_OC_WDT_CTL) & PCH_OC_WDT_CTL_TOV_MASK) + 1;
}

/* Starts and reloads the OC watchdog if enabled in Kconfig */
void setup_oc_wdt(void)
{
	get_watchdog_config(&wdt_config);

	if (wdt_config.wdt_enable) {
		oc_wdt_start(wdt_config.wdt_timeout);
		if (is_oc_wdt_enabled())
			printk(BIOS_DEBUG, "OC Watchdog enabled\n");
		else
			printk(BIOS_ERR, "Failed to enable OC watchdog\n");
	} else {
		oc_wdt_disable();
	}
}
