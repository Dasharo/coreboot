/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <acpi/acpi_gnvs.h>
#include <arch/ioapic.h>
#include <device/device.h>
#include <soc/acpi.h>
#include <soc/iomap.h>
#include <soc/nvs.h>
#include <boardid.h>
#include "onboard.h"

void acpi_create_gnvs(struct global_nvs *gnvs)
{
	acpi_init_gnvs(gnvs);

	/* Enable USB ports in S3 */
	gnvs->s3u0 = 1;
	gnvs->s3u1 = 1;

	/* Disable USB ports in S5 */
	gnvs->s5u0 = 0;
	gnvs->s5u1 = 0;

	/* Enable DPTF */
	gnvs->dpte = 1;

	/* PMIC is configured in I2C1, hidden it from OS */
	gnvs->dev.lpss_en[LPSS_NVS_I2C2] = 0;
}

unsigned long acpi_fill_madt(unsigned long current)
{
	/* Local APICs */
	current = acpi_create_madt_lapics(current);

	/* IOAPIC */
	current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *) current,
				2, IO_APIC_ADDR, 0);

	current = acpi_madt_irq_overrides(current);

	return current;
}

void mainboard_fill_fadt(acpi_fadt_t *fadt)
{
	fadt->preferred_pm_profile = PM_MOBILE;
}
