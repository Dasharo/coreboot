/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi_gnvs.h>
#include <ec/google/chromeec/ec.h>
#include <vendorcode/google/chromeos/gnvs.h>

void gnvs_assign_chromeos(void)
{
	chromeos_acpi_t *gnvs_chromeos = gnvs_chromeos_ptr();
	chromeos_init_chromeos_acpi(gnvs_chromeos);

	/* EC can override to ECFW_RW. */
	gnvs_chromeos->vbt2 = ACTIVE_ECFW_RO;

	if (CONFIG(EC_GOOGLE_CHROMEEC) && !google_ec_running_ro())
		gnvs_chromeos->vbt2 = ACTIVE_ECFW_RW;
}
