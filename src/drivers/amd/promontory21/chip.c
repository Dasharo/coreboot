/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <acpi/acpi_device.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pciexp.h>
#include <static.h>

#include "chip.h"

static const char *prom21_sata_acpi_name(const struct device *dev)
{
	return "STCR";
}

static const char *prom21_usp_acpi_name(const struct device *dev)
{
	return "UP00";
}

static const char *prom21_dsp_acpi_name(const struct device *dev)
{
	char *name;

	if (dev->path.type != DEVICE_PATH_PCI)
		return NULL;

	name = malloc(ACPI_NAME_BUFFER_SIZE);
	snprintf(name, ACPI_NAME_BUFFER_SIZE, "DP%02X", dev->path.pci.devfn);
	name[4] = '\0';

	return name;
}

struct device_operations prom21_sata_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.ops_pci		= &pci_dev_ops_pci,
	.acpi_name		= prom21_sata_acpi_name,
	.acpi_fill_ssdt		= acpi_device_write_pci_dev,
};

static struct pci_operations prom21_pcie_ops = {
	.set_subsystem		= pci_dev_set_subsystem,
};

struct device_operations amd_prom21_usp_ops = {
	.read_resources		= pci_bus_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_bus_enable_resources,
	.scan_bus		= pciexp_scan_bridge,
	.reset_bus		= pci_bus_reset,
	.acpi_name		= prom21_usp_acpi_name,
	.acpi_fill_ssdt		= acpi_device_write_pci_dev,
	.ops_pci		= &prom21_pcie_ops,
};

struct device_operations amd_prom21_dsp_ops = {
	.read_resources		= pci_bus_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_bus_enable_resources,
	.scan_bus		= pciexp_scan_bridge,
	.reset_bus		= pci_bus_reset,
	.acpi_name		= prom21_dsp_acpi_name,
	.acpi_fill_ssdt		= acpi_device_write_pci_dev,
	.ops_pci		= &prom21_pcie_ops,
};

struct chip_operations drivers_amd_promontory21_ops = {
	.name = "AMD Promontory21",
};
