/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <acpi/acpi_device.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <static.h>

#include "chip.h"

static const char *prom21_sata_acpi_name(const struct device *dev)
{
	return "STCR";
}

struct device_operations prom21_sata_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.ops_pci		= &pci_dev_ops_pci,
	.acpi_name		= prom21_sata_acpi_name,
	.acpi_fill_ssdt		= acpi_device_write_pci_dev,
};

struct chip_operations drivers_amd_promontory21_ops = {
	.name = "AMD Promontory21",
};
