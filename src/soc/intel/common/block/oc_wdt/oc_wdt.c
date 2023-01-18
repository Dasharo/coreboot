/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <arch/io.h>
#include <cf9_reset.h>
#include <console/console.h>
#include <drivers/efi/efivars.h>
#include <halt.h>
#include <intelblocks/oc_wdt.h>
#include <intelblocks/pmclib.h>
#include <smmstore.h>
#include <soc/iomap.h>
#include <soc/pm.h>

/* OC WDT configuration */
#define PCH_OC_WDT_CTL				(ACPI_BASE_ADDRESS + 0x54)
#define   PCH_OC_WDT_CTL_RLD			BIT(31)
#define   PCH_OC_WDT_CTL_ICCSURV_STS		BIT(25)
#define   PCH_OC_WDT_CTL_NO_ICCSURV_STS		BIT(24)
#define   PCH_OC_WDT_CTL_FORCE_ALL		BIT(15)
#define   PCH_OC_WDT_CTL_EN			BIT(14)
#define   PCH_OC_WDT_CTL_ICCSURV		BIT(13)
#define   PCH_OC_WDT_CTL_LCK			BIT(12)
#define   PCH_OC_WDT_CTL_TOV_MASK		0x3FF

struct watchdog_config {
	bool wdt_enable;
	uint16_t wdt_timeout;
} __packed;

static struct watchdog_config wdt_config;
static int wdt_config_initted = 0;

/*
 * Checks whether SMM BIOS write protection was enabled in BIOS.
 */
static void get_watchdog_config(void)
{
#if CONFIG(DRIVERS_EFI_VARIABLE_STORE)
	struct region_device rdev;
	enum cb_err ret;
	uint32_t size;
	bool wdt_available;

	const EFI_GUID dasharo_system_features_guid = {
		0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
	};


	if (smmstore_lookup_region(&rdev)) {
		wdt_config.wdt_enable = CONFIG(SOC_INTEL_COMMON_OC_WDT_ENABLE);
		wdt_config.wdt_timeout = CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT;
		wdt_config_initted = 1;
		return;
	}

	size = sizeof(wdt_available);
	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "WatchdogAvailable",
				&wdt_available, &size);
	/*
	 * Update the watchdog state to indicate whether we want to expose 
	 * its configuration in the setup menu. Needed to handle the case when
	 * variable storage would be corrupted.
	 */
	if (ret == CB_SUCCESS) {
		if (wdt_available != CONFIG(SOC_INTEL_COMMON_OC_WDT_ENABLE)) {
			wdt_available = CONFIG(SOC_INTEL_COMMON_OC_WDT_ENABLE);
			efi_fv_set_option(&rdev, &dasharo_system_features_guid,
					  "WatchdogAvailable", &wdt_available, size);

		}
	}

	size = sizeof(wdt_config);
	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "WatchdogConfig",
				&wdt_config, &size);
	if (ret == CB_SUCCESS) {
		wdt_config_initted = 1;
		return;
	}
#endif
	wdt_config.wdt_enable = CONFIG(SOC_INTEL_COMMON_OC_WDT_ENABLE);
	wdt_config.wdt_timeout = CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT;
	wdt_config_initted = 1;
}


static void wdt_reload(uint16_t timeout)
{
	uint32_t readback;

	printk(BIOS_SPEW, "Watchdog: reload and start timer (timeout %ds)\n", timeout);

	if ((timeout - 1) > PCH_OC_WDT_CTL_TOV_MASK || timeout == 0) {
		printk(BIOS_ERR, "Watchdog: invalid timeout value\n");
		return;
	}

	readback = inl(PCH_OC_WDT_CTL);
	readback |= (PCH_OC_WDT_CTL_EN | PCH_OC_WDT_CTL_FORCE_ALL | PCH_OC_WDT_CTL_ICCSURV);

	readback &= ~PCH_OC_WDT_CTL_TOV_MASK;
	readback |= ((timeout - 1) & PCH_OC_WDT_CTL_TOV_MASK);
	readback |= PCH_OC_WDT_CTL_RLD;

	outl(readback, PCH_OC_WDT_CTL);
}

void wdt_reload_and_start(void)
{
	if (!wdt_config_initted)
		get_watchdog_config();

	if (!wdt_config.wdt_enable)
		return;

	wdt_reload(wdt_config.wdt_timeout);
}

void wdt_disable(void)
{
	uint32_t readback;

	printk(BIOS_DEBUG, "Watchdog: disabling OC watchdog timer\n");

  	readback = inl(PCH_OC_WDT_CTL);
	readback &= ~(PCH_OC_WDT_CTL_EN | PCH_OC_WDT_CTL_FORCE_ALL);
	outl(readback, PCH_OC_WDT_CTL);
}

bool is_wdt_enabled(void)
{
	if (inl(PCH_OC_WDT_CTL) & PCH_OC_WDT_CTL_EN) {
		printk(BIOS_DEBUG, "Watchdog: WDT enabled\n");
		return true;
	} else {
		printk(BIOS_DEBUG, "Watchdog: WDT disabled\n");
		return false;
	}
}

uint16_t wdt_get_current_timeout(void)
{
	return (inl(PCH_OC_WDT_CTL) & PCH_OC_WDT_CTL_TOV_MASK) + 1;
}
