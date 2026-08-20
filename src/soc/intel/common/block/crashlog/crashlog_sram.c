/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_rom.h>
#include <device/resource.h>

static void crashlog_sram_read_resources(struct device *dev)
{
	struct resource *res;

	pci_dev_read_resources(dev);

	if (!ENV_X86_32)
		return;

	for (res = dev->resource_list; res; res = res->next) {
		if (!(res->flags & IORESOURCE_MEM))
			continue;

		/* Nothing to do for windows that are not allocated by us. */
		if (res->flags & (IORESOURCE_FIXED | IORESOURCE_ASSIGNED))
			continue;

		res->limit = 0xffffffff;
		res->flags &= ~IORESOURCE_ABOVE_4G;
	}
}

static const struct device_operations crashlog_sram_ops = {
	.read_resources		= crashlog_sram_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
#if CONFIG(HAVE_ACPI_TABLES)
	.write_acpi_tables	= pci_rom_write_acpi_tables,
	.acpi_fill_ssdt		= pci_rom_ssdt,
#endif
	.init			= pci_dev_init,
	.ops_pci		= &pci_dev_ops_pci,
};

static const unsigned short pci_device_ids[] = {
	PCI_DID_INTEL_TGL_CPU_CRASHLOG_SRAM,
	PCI_DID_INTEL_TGP_PMC_CRASHLOG_SRAM,
	PCI_DID_INTEL_ADL_CPU_CRASHLOG_SRAM,
	PCI_DID_INTEL_ADP_S_PMC_CRASHLOG_SRAM,
	PCI_DID_INTEL_ADP_P_PMC_CRASHLOG_SRAM,
	PCI_DID_INTEL_ADP_N_PMC_CRASHLOG_SRAM,
	PCI_DID_INTEL_RPL_CPU_CRASHLOG_SRAM,
	PCI_DID_INTEL_RPP_S_PMC_CRASHLOG_SRAM,
	0,
};

static const struct pci_driver crashlog_sram __pci_driver = {
	.ops		= &crashlog_sram_ops,
	.vendor		= PCI_VID_INTEL,
	.devices	= pci_device_ids,
};
