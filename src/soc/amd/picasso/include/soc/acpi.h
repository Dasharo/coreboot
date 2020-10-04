/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __SOC_PICASSO_ACPI_H__
#define __SOC_PICASSO_ACPI_H__

#include <acpi/acpi.h>
#include <amdblocks/acpi.h>
#include <amdblocks/gpio_banks.h>

unsigned long southbridge_write_acpi_tables(const struct device *device,
		unsigned long current, struct acpi_rsdp *rsdp);

uintptr_t agesa_write_acpi_tables(const struct device *device, uintptr_t current,
				  acpi_rsdp_t *rsdp);

const char *soc_acpi_name(const struct device *dev);

/* Object to capture state of chipset for logging events. */
struct chipset_state {
	struct acpi_pm_gpe_state gpe_state;
	struct gpio_wake_state gpio_state;
};

#endif /* __SOC_PICASSO_ACPI_H__ */
