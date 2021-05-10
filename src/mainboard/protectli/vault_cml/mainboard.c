
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/io.h>
#include <device/device.h>
#include <stdlib.h>
#include <soc/gpio.h>
#include <soc/ramstage.h>
#include "gpio.h"

static void beep(unsigned int frequency)
{

	unsigned int count = 1193180 / frequency;

	// Switch on the speaker
	outb(inb(0x61)|3, 0x61);

	// Set command for counter 2, 2 byte write
	outb(0xb6, 0x43);

	// Select desired Hz
	outb(count & 0xff, 0x42);
	outb((count >> 8) & 0xff, 0x42);

	// Block for 100 milioseconds
	int i;
	for (i = 0; i < 10000; i++) inb(0x61);

	// Switch off the speaker
	outb(inb(0x61) & 0xfc, 0x61);
}

static void mainboard_final(void *unused)
{
	beep(1500);
}

void mainboard_silicon_init_params(FSPS_UPD *supd)
{
	const struct pad_config *pads;
	size_t num;

	pads = board_gpio_table(&num);
	/* Configure pads prior to SiliconInit() in case there's any
	 * dependencies during hardware initialization. */
	cnl_configure_pads(pads, num);

	/* FIXME: disable GFX PM and RC6 causing global resets initiated by PMC
	 * when the screen goes blank/inactive/idle in the OS */
	supd->FspsTestConfig.RenderStandby = 0;
	supd->FspsTestConfig.PmSupport = 0;
}


struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
