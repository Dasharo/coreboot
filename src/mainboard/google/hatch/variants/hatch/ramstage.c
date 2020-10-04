/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <delay.h>
#include <gpio.h>
#include <baseboard/variants.h>
#include <soc/gpio.h>

void variant_ramstage_init(void)
{
	/*
	 * Enable power to FPMCU, wait for power rail to stabilize,
	 * and then deassert FPMCU reset.
	 * Waiting for the power rail to stabilize can take a while,
	 * a minimum of 400us on Kohaku.
	 */
	gpio_output(GPP_C11, 1);
	mdelay(1);
	gpio_output(GPP_A12, 1);
}
