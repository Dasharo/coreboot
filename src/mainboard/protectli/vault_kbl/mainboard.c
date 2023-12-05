
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <device/device.h>
#include <pc80/i8254.h>
#include <soc/gpio.h>
#include <soc/ramstage.h>
#include <stdlib.h>

#include "gpio.h"

void mainboard_silicon_init_params(FSP_SIL_UPD *params)
{
	/*
	 * Configure pads prior to SiliconInit() in case there's any
	 * dependencies during hardware initialization.
	 */
	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));

	params->PchPort61hEnable = 1;
}


static void mainboard_final(void *unused)
{
	if (CONFIG(BEEP_ON_BOOT))
		beep(1500, 100);
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
