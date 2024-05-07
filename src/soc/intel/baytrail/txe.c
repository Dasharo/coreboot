/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/pci_ops.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>

#include <soc/iosf.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>
#include <soc/txe.h>
#include "chip.h"

static void txe_enable(struct device *dev)
{
	southcluster_enable_dev(dev);

	txe_fw_shadow_done();

	txe_print_secure_boot_status();
}

static void txe_init(struct device *dev)
{
	uint32_t reg;

	dump_txe_status();
	txe_txei_init();

	reg = pci_read_config32(dev, TXE_FWSTS0);
	if ((reg & DID_MSG_ACK_MASK) == DID_MSG_GOT_ACK) {
		reg &= DID_ACK_DATA_MASK;
		txe_handle_bios_action(reg);
	}
}

static void txe_final(struct device *dev)
{
	struct resource *heci_bar1;

	dump_txe_status();

	// FIXME: does not work yet
	// txe_do_send_end_of_post();

	heci_bar1 = find_resource(dev, PCI_BASE_ADDRESS_1);

	if (!heci_bar1 || heci_bar1->base == 0) {
		printk(BIOS_DEBUG, "HECI BAR1 not assigned, not sending aliveness request");
		return;
	}

	txe_aliveness_request((uintptr_t)heci_bar1->base, false);
}

static const struct device_operations device_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= txe_init,
	.enable			= txe_enable,
	.final			= txe_final,
	.ops_pci		= &soc_pci_ops,
};

static const struct pci_driver southcluster __pci_driver = {
	.ops		= &device_ops,
	.vendor		= PCI_VID_INTEL,
	.device		= TXE_DEVID,
};
