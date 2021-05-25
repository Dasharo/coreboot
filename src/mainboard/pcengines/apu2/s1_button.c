/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018 PC Engines GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <string.h>
#include <stdlib.h>
#include <spi_flash.h>
#include <spi-generic.h>
#include <boot_device.h>
#include <fmap.h>
#include <commonlib/region.h>
#include <console/console.h>

#include "s1_button.h"
#include "bios_knobs.h"

static int find_knob_index(const char *s, const char *pattern)
{

	int pattern_index = 0;
	char *result = (char *) s;
	char *lpattern = (char *) pattern;

	while (*result && *pattern ) {
		if ( *lpattern == 0)  // the pattern matches return the pointer
			return pattern_index;
		if ( *result == 0)  // We're at the end of the file content but don't have a patter match yet
			return -1;
		if (*result == *lpattern ) {
			// The string matches, simply advance
			result++;
			pattern_index++;
			lpattern++;
		} else {
			// The string doesn't match restart the pattern
			result++;
			pattern_index++;
			lpattern = (char *) pattern;
		}
	}

	return -1;
}

static int flash_bootorder_file(size_t offset, size_t fsize, char *buffer)
{
	const struct spi_flash *flash;

	flash = boot_device_spi_flash();

	if (flash == NULL) {
		printk(BIOS_WARNING, "Can't get boot flash device\n");
		return -1;
	}

	printk(BIOS_DEBUG, "Erasing SPI flash at %lx size %lx\n", offset, fsize);

	if (spi_flash_erase(flash, (u32) offset, fsize)) {
		printk(BIOS_WARNING, "SPI erase failed\n");
		return -1;
	}

	printk(BIOS_DEBUG, "Writing SPI flash at %lx size %lx\n", offset, fsize);

	if (spi_flash_write(flash, offset, fsize, buffer)) {
		printk(BIOS_WARNING, "SPI write failed\n");
		return -1;
	}

	printk(BIOS_INFO, "Bootorder write succeed\n");
	return 0;
}

void enable_console(void)
{
	size_t size = 0x1000;
	char bootorder_copy[0x1000];
	char *bootorder = get_bootorder();
	int knob_index;

	if (!bootorder) {
		printk(BIOS_WARNING,"Could not enable console, no bootorder\n");
	}

	memcpy(bootorder_copy, bootorder, size);

	knob_index = find_knob_index(bootorder, "scon");

	if (knob_index == -1){
		printk(BIOS_WARNING,"scon knob not found in bootorder\n");
		return;
	}

	*(bootorder_copy + (size_t)knob_index) = '1';

	if (CONFIG(SEABIOS_BOOTORDER_IN_FMAP)) {
		if (fmap_overwrite_area("BOOTORDER", bootorder_copy, size) != size)
			printk(BIOS_WARNING, "Failed to flash bootorder\n");
	} else {
		if (flash_bootorder_file((size_t)(uintptr_t)bootorder, size, bootorder_copy))
			printk(BIOS_WARNING, "Failed to flash bootorder file\n");
	}
}
