/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <mainboard/variants.h>
#include <gpio.h>

void variant_devtree_update(void)
{
	struct device *i2c_amp_dev = pcidev_on_root(0x19, 1);
	bool have_smartamp = !gpio_get(GPP_E00);
	i2c_amp_dev->enabled = have_smartamp;
}
