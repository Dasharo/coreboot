
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/io.h>
#include <device/device.h>
#include <stdlib.h>

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

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
