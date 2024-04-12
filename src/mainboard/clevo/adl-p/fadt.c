/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>

void mainboard_fill_fadt(acpi_fadt_t *fadt)
{
	/* Control method power button (EC) */
	fadt->preferred_pm_profile = PM_MOBILE;
}
