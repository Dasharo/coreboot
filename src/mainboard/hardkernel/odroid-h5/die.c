/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <soc/gpio.h>
#include <delay.h>
#include <gpio.h>

void die_notify(void)
{
	static uint8_t blink = 0;

	if (ENV_POSTCAR)
		return;

	/* Make SATA LED blink */
	gpio_output(GPP_B14, 0);

	while (1) {
		gpio_set(GPP_B14, blink);
		blink ^= 1;
		mdelay(500);
	}
}
