/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <acpi/acpi_device.h>
#include <acpi/acpigen.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pciexp.h>
#include <lib.h>
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

static void prom21_usp_read_resources(struct device *dev)
{
	struct resource *res;
	resource_t base;

	pci_bus_read_resources(dev);

	base = pci_read_config32(dev, 0x40) & ~PCI_BASE_ADDRESS_MEM_ATTR_MASK;
	base |= (uint64_t)pci_read_config32(dev, 0x44) << 32;

	/* GPIO MMIO */
	if (base == 0 || base == UINT64_MAX)
		return;

	res = new_resource(dev, 0x40);
	if (res) {
		res->base = base;
		res->size = 4 * KiB;
		res->align = log2(res->size);
		res->gran = log2(res->size);
		res->limit = 0xffffffffffffffffULL;
		res->flags = IORESOURCE_MEM | IORESOURCE_PCI64 | IORESOURCE_FIXED |
			     IORESOURCE_ASSIGNED | IORESOURCE_STORED;
	}
}

static void prom21_usp_fill_ssdt(const struct device *dev)
{
	acpi_device_write_pci_dev(dev);

	struct resource *res = probe_resource(dev, 0x40);
	if (!res)
		return;

	/* Scope */
	acpigen_write_scope("\\_SB");

	/* Device */
	acpigen_write_device("PTIO");
	acpigen_write_name_string("_HID", "AMDIF031");
	acpigen_write_name_string("_CID", "AMDIF031");
	acpigen_write_name_integer("_UID", 0);
	acpigen_write_STA(ACPI_STATUS_DEVICE_ALL_ON);

	/* Resources */
	acpigen_write_name("_CRS");
	acpigen_write_resourcetemplate_header();

	/* Add GPIO BAR resource. */
	acpigen_write_mem32fixed(1, res->base, res->size);

	acpigen_write_resourcetemplate_footer();

	acpigen_pop_len(); /* Device */
	acpigen_pop_len(); /* Scope */
}

struct device_operations amd_prom21_usp_ops = {
	.read_resources		= prom21_usp_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_bus_enable_resources,
	.scan_bus		= pciexp_scan_bridge,
	.reset_bus		= pci_bus_reset,
	.acpi_name		= prom21_usp_acpi_name,
	.acpi_fill_ssdt		= prom21_usp_fill_ssdt,
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
