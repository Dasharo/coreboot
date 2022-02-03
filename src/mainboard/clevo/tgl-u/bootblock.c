/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <bootblock_common.h>
#include <soc/gpio.h>
#include <console/console.h>
#include <delay.h>
#include <gpio.h>

#define DGPU_RST_N GPP_U4
#define DGPU_PWR_EN GPP_U5

#ifdef DRIVER_NVIDIA_OPTIMUS
static void dgpu_power_enable(int onoff) {
	printk(BIOS_DEBUG, "nvidia: DGPU power %d\n", onoff);
	if (onoff) {
		gpio_set(DGPU_RST_N, 0);
		mdelay(4);
		gpio_set(DGPU_PWR_EN, 1);
		mdelay(4);
		gpio_set(DGPU_RST_N, 1);
	} else {
		gpio_set(DGPU_RST_N, 0);
		mdelay(4);
		gpio_set(DGPU_PWR_EN, 0);
	}
	mdelay(50);
}
#endif

void bootblock_mainboard_early_init(void)
{
	const struct pad_config *pads;
	size_t num;

	pads = variant_early_gpio_table(&num);
	gpio_configure_pads(pads, num);
#ifdef DRIVER_NVIDIA_OPTIMUS
	dgpu_power_enable(1);
#endif
}
