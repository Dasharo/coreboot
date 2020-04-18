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
#include <fmap.h>
#include <commonlib/region.h>
#include <console/console.h>

#include "s1_button.h"

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

void enable_console(void)
{
	size_t fsize;
	char bootorder_file[0x1000];
	int knob_index;
	struct region_device bootorder_area;

	if (fmap_locate_area_as_rdev_rw("BOOTORDER", &bootorder_area)) {
		printk(BIOS_WARNING, "Could not find bootorder area\n");
		return;
	}

	fsize = region_device_sz(&bootorder_area);

	if (fsize != 0x1000) {
		printk(BIOS_WARNING, "Wrong bootorder size\n");
		return;
	}

	if (rdev_readat(&bootorder_area, bootorder_file, 0, fsize) != fsize) {
		printk(BIOS_WARNING, "Could not read bootorder\n");
		return;
	}

	knob_index = find_knob_index(bootorder_file, "scon");

	if (knob_index == -1){
		printk(BIOS_WARNING,"scon knob not found in bootorder\n");
		return;
	}

	*(bootorder_file + (size_t)knob_index) = '1';

	if (fmap_overwrite_area("BOOTORDER", bootorder_file, fsize) != fsize)
		printk(BIOS_WARNING, "Failed to flash bootorder\n");
}
