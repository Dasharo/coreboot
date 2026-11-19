/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <amdblocks/psp.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <soc/southbridge.h>

#define PSP_PCI_DID	0x1537

static bool psp_dram_ready;

static void psp_init(struct device *dev)
{
	if (acpi_is_wakeup_s3())
		return;

	/* The northbridge only enables the PSP BARs in its final hook */
	pci_or_config8(dev, PSP_BAR_ENABLES, PSP_BAR_ENABLES_ALL);

	if (psp_notify_dram())
		return;

	psp_dram_ready = true;

	if (apm_control(APM_CNT_SMMINFO) != CB_SUCCESS)
		printk(BIOS_ERR, "PSP: Failed to send SMM info\n");
}

/* Without DRAM_INFO, BOOT_DONE times out and the system hangs later */
bool soc_psp_boot_done_needed(void)
{
	return psp_dram_ready;
}

static struct device_operations psp_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= psp_init,
	.ops_pci		= &pci_dev_ops_pci,
};

static const struct pci_driver psp_driver __pci_driver = {
	.ops	= &psp_ops,
	.vendor	= PCI_VID_AMD,
	.device	= PSP_PCI_DID,
};
