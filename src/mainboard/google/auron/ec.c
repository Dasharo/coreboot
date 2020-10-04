/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <vendorcode/google/chromeos/chromeos.h>
#include <console/console.h>
#include <ec/google/chromeec/ec.h>
#include "ec.h"

void mainboard_ec_init(void)
{
	const struct google_chromeec_event_info info = {
		.log_events = MAINBOARD_EC_LOG_EVENTS,
		.sci_events = MAINBOARD_EC_SCI_EVENTS,
		.s3_wake_events = MAINBOARD_EC_S3_WAKE_EVENTS,
		.s5_wake_events = MAINBOARD_EC_S5_WAKE_EVENTS,
	};

	int s3_wakeup = acpi_is_wakeup_s3();

	printk(BIOS_DEBUG, "mainboard_ec_init\n");
	post_code(0xf0);

	google_chromeec_events_init(&info, s3_wakeup);
	if (s3_wakeup) {
		/* Clear pending events. */
		while (google_chromeec_get_event() != 0)
			;
	}

	post_code(0xf1);
}
