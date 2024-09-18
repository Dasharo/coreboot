/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi_gnvs.h>
#include <device/pci_ops.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>

#include <soc/device_nvs.h>
#include <soc/iosf.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>
#include "chip.h"

#define CAP_OVERRIDE_LOW	0xa0
#define CAP_OVERRIDE_HIGH	0xa4
#define USE_CAP_OVERRIDES	(1 << 31)

static void acpi_store_nvs(struct device *dev, int nvs_index)
{
	struct resource *bar;
	struct device_nvs *dev_nvs = acpi_get_device_nvs();

	/* Save BAR0 and BAR1 to ACPI NVS */
	bar = probe_resource(dev, PCI_BASE_ADDRESS_0);
	if (bar)
		dev_nvs->scc_bar0[nvs_index] = (u32)bar->base;

	bar = probe_resource(dev, PCI_BASE_ADDRESS_1);
	if (bar)
		dev_nvs->scc_bar1[nvs_index] = (u32)bar->base;
}

static void sd_init(struct device *dev)
{
	struct soc_intel_baytrail_config *config = config_of(dev);

	if (config->sdcard_cap_low != 0 || config->sdcard_cap_high != 0) {
		printk(BIOS_DEBUG, "Overriding SD Card controller caps.\n");
		pci_write_config32(dev, CAP_OVERRIDE_LOW, config->sdcard_cap_low);
		pci_write_config32(dev, CAP_OVERRIDE_HIGH, config->sdcard_cap_high |
							   USE_CAP_OVERRIDES);
	}

	if (config->scc_acpi_mode)
		scc_enable_acpi_mode(dev, SCC_SD_CTL, SCC_NVS_SD);
	else
		acpi_store_nvs(dev, SCC_NVS_SD);
}

static const struct device_operations device_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= sd_init,
	.ops_pci		= &soc_pci_ops,
};

static const struct pci_driver southcluster __pci_driver = {
	.ops		= &device_ops,
	.vendor		= PCI_VID_INTEL,
	.device		= SD_DEVID,
};
