/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <mainboard/variants.h>
#include <gpio.h>
#include <dasharo/options.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <device/pci_def.h>
#include <soc/intel/common/reset.h>

void variant_devtree_update(void)
{
	struct device *i2c_amp_dev = pcidev_on_root(0x19, 1);
	if (i2c_amp_dev) {
		bool have_smartamp = !gpio_get(GPP_E00);
		i2c_amp_dev->enabled = have_smartamp;
	}

	struct device *dgpu_dev = pcidev_on_root(0x01, 0);
	if (dgpu_dev) {
	        dgpu_dev->enabled = dasharo_is_dgpu_enabled() != 0;
	        printk(BIOS_DEBUG, "dgpu_dev->enabled: %d\n", dgpu_dev->enabled);
	}
}

void variant_final(void)
{
	struct device *dgpu_rp_dev = pcidev_on_root(0x01, 0);

	if (!dgpu_rp_dev)
		return;

	struct device *dgpu_dev = pcidev_path_behind(dgpu_rp_dev->downstream, PCI_DEVFN(0, 0));

	/*
	 * The dGPU may fail to come up after changing from iGPU mode to dGPU
	 * mode. If that happens, we need to kick the platform via global reset.
	 */
	if (!dgpu_dev && dasharo_is_dgpu_enabled()) {
		printk(BIOS_DEBUG, "dGPU did not come up! Kicking the platform to work around it\n");
		do_global_reset();
	} else {
		printk(BIOS_DEBUG, "dGPU is up.\n");
	}

}
