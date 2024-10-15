/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <pc80/i8254.h>
#include <delay.h>

void die_notify(void)
{
	uint8_t beep_count = 0;

	if (ENV_POSTCAR)
		return;

	/* Beep 12 times at most, constant beeps may be annoying */
	if (beep_count < 12) {
		beep(800, 300);
		mdelay(200);
		beep_count++;
	} else {
		mdelay(500);
	}
}
