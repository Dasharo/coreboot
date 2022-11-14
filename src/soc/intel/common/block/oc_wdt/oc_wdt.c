/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <arch/io.h>
#include <cf9_reset.h>
#include <console/console.h>
#include <halt.h>
#include <intelblocks/oc_wdt.h>
#include <intelblocks/pmclib.h>
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
#define   PCH_OC_WDT_CTL_FAILURE_STS		BIT(23)
#define   PCH_OC_WDT_CTL_UNXP_RESET_STS		BIT(22)
#define   PCH_OC_WDT_CTL_ALLOW_UNXP_RESET_STS	BIT(21)
#define   PCH_OC_WDT_CTL_AFTER_POST		0x1F0000

struct watchdog_config {
	bool wdt_enable;
	uint16_t wdt_timeout;
} __packed;

static struct watchdog_config *wdt_config;
static int wdt_config_initted = 0;

/*
 * Checks whether SMM BIOS write protection was enabled in BIOS.
 */
static void get_watchdog_config(void)
{
#if CONFIG(DRIVERS_EFI_VARIABLE_STORE)
	struct region_device rdev;
	enum cb_err ret;
	uint8_t var;
	uint32_t size;

	const EFI_GUID dasharo_system_features_guid = {
		0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
	};


	if (smmstore_lookup_region(&rdev))
		return false;

	size = sizeof(*wdt_config);
	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "WatchdogConfig", &wdt_config, &size);
	if (ret == CB_SUCCESS) {
		wdt_config_initted = 1;
		return;
	}

#endif
	wdt_config->wdt_enable = CONFIG(SOC_INTEL_COMMON_OC_WDT_ENABLE);
	wdt_config->wdt_timeout = CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT;
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

	if (!(readback & PCH_OC_WDT_CTL_ALLOW_UNXP_RESET_STS))
		readback |= PCH_OC_WDT_CTL_UNXP_RESET_STS;

	readback &= ~PCH_OC_WDT_CTL_TOV_MASK;
	readback |= ((timeout - 1) & PCH_OC_WDT_CTL_TOV_MASK);
	readback |= PCH_OC_WDT_CTL_RLD;

	outl(readback, PCH_OC_WDT_CTL);
}

void wdt_reload_and_start(void)
{
	if (!wdt_config_initted)
		get_watchdog_config();

	if (!wdt_config->wdt_enable)
		return;

	wdt_reload(wdt_config->wdt_timeout);
}

void wdt_disable(void)
{
	uint32_t readback;

	printk(BIOS_DEBUG, "Watchdog: disabling OC watchdog timer\n");

  	readback = inl(PCH_OC_WDT_CTL);
	readback &= ~(PCH_OC_WDT_CTL_EN | PCH_OC_WDT_CTL_FORCE_ALL |
		      PCH_OC_WDT_CTL_UNXP_RESET_STS);
	outl(readback, PCH_OC_WDT_CTL);
}

bool is_wdt_failure(void)
{
	uint32_t readback;

	if (!wdt_config_initted)
		get_watchdog_config();

	if (!wdt_config->wdt_enable)
		return;

	readback = inl(PCH_OC_WDT_CTL);

	printk(BIOS_SPEW, "Watchdog: status = (%08x)\n", readback);

	if (readback & PCH_OC_WDT_CTL_FAILURE_STS) {
		printk (BIOS_ERR, "Watchdog: Failure detected\n");
		return true;
	} else {
		return false;
	}
}

/*
 * Normally, each reboot performed while watchdog runs is considered a failure.
 * This function allows platform to perform expected reboots with WDT running,
 * without being interpreted as failures.
 */
void wdt_allow_known_reset(void)
{
	uint32_t readback;

	if (!wdt_config_initted)
		get_watchdog_config();

	if (!wdt_config->wdt_enable)
		return;

	readback = inl(PCH_OC_WDT_CTL);
	readback &= ~(PCH_OC_WDT_CTL_UNXP_RESET_STS | PCH_OC_WDT_CTL_FORCE_ALL);
	readback |= PCH_OC_WDT_CTL_ALLOW_UNXP_RESET_STS;
	outl(readback, PCH_OC_WDT_CTL);
}

#if CONFIG(HAVE_CF9_RESET_PREPARE)
void cf9_reset_prepare(void)
{
	wdt_allow_known_reset();
}
#endif

/*
 * Returns information if WDT coverage for the duration of BIOS execution
 * was requested by an OS application
 */
bool is_wdt_required(void)
{
	if (!wdt_config_initted)
		get_watchdog_config();

	if (inl(PCH_OC_WDT_CTL) & PCH_OC_WDT_CTL_AFTER_POST) {
		printk(BIOS_DEBUG, "Watchdog: OS requested WDT coverage");
		if (!wdt_config->wdt_enable) {
			printk(BIOS_DEBUG, "but WDT not enabled\n");
			return false;
		} else {
			printk(BIOS_DEBUG, "\n");
			return true;
		}
	} else {
		printk(BIOS_DEBUG, "Watchdog: OS did not request WDT coverage\n");
		return false;
	}
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

static bool is_wake_from_s3_s4(void)
{
	/* Read power state from PMC ABASE */
	if (!(inw(ACPI_BASE_ADDRESS + PM1_STS) & WAK_STS))
		return false;

	switch (acpi_sleep_from_pm1(pmc_read_pm1_control())) {
	case ACPI_S3:
	case ACPI_S4:
		return true;
	default:
		return false;
	}
}

static void wdt_clear_allow_known_reset(void)
{
	uint32_t readback;

	readback = inl(PCH_OC_WDT_CTL);
	readback &= ~PCH_OC_WDT_CTL_ALLOW_UNXP_RESET_STS;
	readback &= ~(PCH_OC_WDT_CTL_ICCSURV_STS | PCH_OC_WDT_CTL_NO_ICCSURV_STS);

	outl(readback, PCH_OC_WDT_CTL);
}

/*
 * Check for unexpected reset.
 * If there was an unexpected reset, enforces WDT expiration.
 * Should be called before memory init.
 */
void wdt_reset_check(void)
{
	uint32_t readback;

	if (!wdt_config_initted)
		get_watchdog_config();


	wdt_clear_allow_known_reset();

	if (!wdt_config->wdt_enable) {
		wdt_disable();
		return;
	}

	readback = inl(PCH_OC_WDT_CTL);

	printk(BIOS_DEBUG, "Watchdog: OC WDT reset check\n");

	/*
	 * If there was a WDT expiration, set Failure Status and clear timeout status bits.
	 * Timeout status bits are cleared by writing '1'.
	 */
	if (readback & (PCH_OC_WDT_CTL_ICCSURV_STS | PCH_OC_WDT_CTL_NO_ICCSURV_STS)) {
		printk (BIOS_ERR, "Watchdog: timer expiration detected.\n");
		readback |= PCH_OC_WDT_CTL_FAILURE_STS;
		readback |= (PCH_OC_WDT_CTL_ICCSURV_STS | PCH_OC_WDT_CTL_NO_ICCSURV_STS);
		readback &= ~PCH_OC_WDT_CTL_UNXP_RESET_STS;
	} else {
		/*
		 * If there was unexpected reset but no WDT expiration and no resume from
		 * S3/S4, clear unexpected reset status and enforce expiration.
		 */
		if ((readback & PCH_OC_WDT_CTL_UNXP_RESET_STS) && !is_wake_from_s3_s4()) {
			printk(BIOS_ERR, "Watchdog: unexpected reset detected\n");
			printk(BIOS_ERR, "Watchdog: enforcing WDT expiration\n");
			wdt_reload(5);
			/* wait for reboot caused by WDT expiration */
			halt();
		} else {
			/* No WDT expiration and no unexpected reset - clear failure status */
			printk(BIOS_DEBUG, "Watchdog: status OK.\n");
			readback &= ~PCH_OC_WDT_CTL_FAILURE_STS;
			readback |= (PCH_OC_WDT_CTL_ICCSURV_STS |
				     PCH_OC_WDT_CTL_NO_ICCSURV_STS);
		}
	}

	outl(readback, PCH_OC_WDT_CTL);
}
