/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <arch/io.h>
#include <device/mmio.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include "ec.h"
#include <ec/google/chromeec/ec.h>
#include <elog.h>
#include <soc/nvs.h>
#include <soc/pm.h>
#include <soc/gpio.h>

#include <variant/onboard.h>

/* The wake gpio is SUS_GPIO[0]. */
#define WAKE_GPIO_EN SUS_GPIO_EN0
#define GPIO_SUS7_WAKE_MASK     (1 << 12)
#define GPIO_SUS1_WAKE_MASK     (1 << 13)

int mainboard_io_trap_handler(int smif)
{
	switch (smif) {
	case 0x99:
		printk(BIOS_DEBUG, "Sample\n");
		gnvs->smif = 0;
		break;
	default:
		return 0;
	}

	/*
	 * On success, the IO Trap Handler returns 0
	 * On failure, the IO Trap Handler returns a value != 0
	 *
	 * For now, we force the return value to 0 and log all traps to
	 * see what's going on.
	 */
	//gnvs->smif = 0;
	return 1;
}

static uint8_t mainboard_smi_ec(void)
{
	uint8_t cmd = google_chromeec_get_event();
	uint16_t pmbase = get_pmbase();
	uint32_t pm1_cnt;

	/* Log this event */
	if (cmd)
		elog_gsmi_add_event_byte(ELOG_TYPE_EC_EVENT, cmd);

	switch (cmd) {
	case EC_HOST_EVENT_LID_CLOSED:
		printk(BIOS_DEBUG, "LID CLOSED, SHUTDOWN\n");

		/* Go to S5 */
		pm1_cnt = inl(pmbase + PM1_CNT);
		pm1_cnt |= SLP_EN | (SLP_TYP_S5 << SLP_TYP_SHIFT);
		outl(pm1_cnt, pmbase + PM1_CNT);
		break;
	}

	return cmd;
}

/*
 * The entire 32-bit ALT_GPIO_SMI register is passed as a parameter. Note, that
 * this includes the enable bits in the lower 16 bits.
 */
void mainboard_smi_gpi(uint32_t alt_gpio_smi)
{
	if (alt_gpio_smi & (1 << EC_SMI_GPI)) {
		/* Process all pending events */
		while (mainboard_smi_ec() != 0)
			;
	}
}

void mainboard_smi_sleep(uint8_t slp_typ)
{
	void		*addr;
	uint32_t	mask;

	/* Disable USB charging if required */
	switch (slp_typ) {
	case ACPI_S3:
		if (gnvs->s3u0 == 0)
			google_chromeec_set_usb_charge_mode(
				0, USB_CHARGE_MODE_DISABLED);
		if (gnvs->s3u1 == 0)
			google_chromeec_set_usb_charge_mode(
				1, USB_CHARGE_MODE_DISABLED);

		/* Enable wake events */
		google_chromeec_set_wake_mask(MAINBOARD_EC_S3_WAKE_EVENTS);
		/* Enable wake pin in GPE block. */
		enable_gpe(WAKE_GPIO_EN);
		break;
	case ACPI_S4:
	case ACPI_S5:
		if (gnvs->s5u0 == 0)
			google_chromeec_set_usb_charge_mode(
				0, USB_CHARGE_MODE_DISABLED);
		if (gnvs->s5u1 == 0)
			google_chromeec_set_usb_charge_mode(
				1, USB_CHARGE_MODE_DISABLED);

		/* Enable wake events */
		google_chromeec_set_wake_mask(MAINBOARD_EC_S5_WAKE_EVENTS);

		/* Disabling wake from SUS_GPIO1 (TOUCH INT) and
		 * SUS_GPIO7 (TRACKPAD INT) in North bank as they are not
		 * valid S5 wake sources
		 */
		addr = (void *)(IO_BASE_ADDRESS + COMMUNITY_OFFSET_GPNORTH +
			GPIO_WAKE_MASK_REG0);
		mask = ~(GPIO_SUS1_WAKE_MASK | GPIO_SUS7_WAKE_MASK);
		write32(addr, read32(addr) & mask);

		break;
	}

	/* Disable SCI and SMI events */
	google_chromeec_set_smi_mask(0);
	google_chromeec_set_sci_mask(0);

	/* Clear pending events that may trigger immediate wake */
	while (google_chromeec_get_event() != EC_HOST_EVENT_NONE)
		;

	/* Set LPC lines to low power in S3/S5. */
	if ((slp_typ == ACPI_S3) || (slp_typ == ACPI_S5)) {
		lpc_set_low_power();
	}
}

int mainboard_smi_apmc(uint8_t apmc)
{
	switch (apmc) {
	case APM_CNT_ACPI_ENABLE:
		google_chromeec_set_smi_mask(0);
		/* Clear all pending events */
		while (google_chromeec_get_event() != EC_HOST_EVENT_NONE)
			;
		google_chromeec_set_sci_mask(MAINBOARD_EC_SCI_EVENTS);
		break;
	case APM_CNT_ACPI_DISABLE:
		google_chromeec_set_sci_mask(0);
		/* Clear all pending events */
		while (google_chromeec_get_event() != EC_HOST_EVENT_NONE)
			;
		google_chromeec_set_smi_mask(MAINBOARD_EC_SMI_EVENTS);
		break;
	}
	return 0;
}
