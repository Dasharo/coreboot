/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <device/device.h>

static void amdfam10_nb_init(void *chip_info)
{
}

struct chip_operations northbridge_amd_amdfam10_ops = {
	CHIP_NAME("AMD Family 10h/15h Northbridge")
	.enable_dev = 0,
	.init = amdfam10_nb_init,
};

static void root_complex_enable_dev(struct device *dev)
{
}

static void root_complex_finalize(void *chip_info)
{

}

struct chip_operations northbridge_amd_amdfam10_root_complex_ops = {
	CHIP_NAME("AMD Family 10h/15h Root Complex")
	.enable_dev = root_complex_enable_dev,
	.final = root_complex_finalize,
};
