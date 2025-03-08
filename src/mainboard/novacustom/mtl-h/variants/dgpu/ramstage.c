/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <mainboard/variants.h>
#include <gpio.h>
#include <dasharo/options.h>

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
