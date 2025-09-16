/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>

void mainboard_fill_fadt(acpi_fadt_t *fadt)
{
	fadt->iapc_boot_arch |= ACPI_FADT_LEGACY_DEVICES;
}
