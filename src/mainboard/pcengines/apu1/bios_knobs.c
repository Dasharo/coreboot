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
#include <stdint.h>
#include <console/console.h>
#include <program_loading.h>
#include <cbfs.h>
#include <commonlib/cbfs.h>
#include <commonlib/region.h>
#include "bios_knobs.h"

#define BOOTORDER_FILE "bootorder"

static char * findstr(const char *s, const char *pattern)
{
	char *result = (char *) s;
	char *lpattern = (char *) pattern;

	while (*result && *pattern ) {
		if ( *lpattern == 0)
		// the pattern matches return the pointer
			return result;
		if ( *result == 0)
		// We're at the end of the file content but
		// don't have a pattern match yet
			return NULL;
		if (*result == *lpattern ) {
			// The string matches, simply advance
			result++;
			lpattern++;
		} else {
			// The string doesn't match restart the pattern
			result++;
			lpattern = (char *) pattern;
		}
	}

	return NULL;
}

static u8 check_knob_value(const char *s)
{
	const char *boot_file = NULL;
	size_t boot_file_len = 0;
	char * token = NULL;

	//
	// This function locates a file in cbfs, maps it to memory and returns
	// a void* pointer
	//
	boot_file = cbfs_boot_map_with_leak(BOOTORDER_FILE, CBFS_TYPE_RAW,
						&boot_file_len);
	if (boot_file == NULL)
		printk(BIOS_INFO, "file [%s] not found in CBFS\n",
			BOOTORDER_FILE);
	if (boot_file_len < 4096)
		printk(BIOS_INFO, "Missing bootorder data.\n");
	if (boot_file == NULL || boot_file_len < 4096)
		return -1;

	token = findstr( boot_file, s );

	if (token) {
		if (*token == '0') return 0;
		if (*token == '1') return 1;
	}

	return -1;
}

u8 check_console(void)
{
	u8 scon;

	//
	// Find the serial console item
	//
	scon = check_knob_value("scon");

	switch (scon) {
	case 0:
		return 0;
		break;
	case 1:
		return 1;
		break;
	default:
		printk(BIOS_INFO,
			"Missing or invalid scon knob, enable console.\n");
		break;
	}

	return 1;
}

int check_com2(void)
{
	u8 com2en;

	//
	// Find the COM2 redirection item
	//
	com2en = check_knob_value("com2en");

	switch (com2en) {
	case 0:
		return 0;
		break;
	case 1:
		return 1;
		break;
	default:
		printk(BIOS_INFO,
			"Missing or invalid com2 knob, disable COM2 output.\n");
		break;
	}

	return 0;
}

static u8 check_uart(char uart_letter)
{
	u8 uarten;

	switch (uart_letter) {
	case 'c':
		uarten = check_knob_value("uartc");
		break;
	case 'd':
		uarten = check_knob_value("uartd");
		break;
	default:
		uarten = -1;
		break;
	}

	switch (uarten) {
	case 0:
		return 0;
		break;
	case 1:
		return 1;
		break;
	default:
		printk(BIOS_INFO,
			 "Missing or invalid uart knob, disable port.\n");
		break;
	}

	return 0;
}

inline u8 check_uartc(void)
{
	return check_uart('c');
}

inline u8 check_uartd(void)
{
	return check_uart('d');
}

u8 check_ehci0(void)
{
	// Not supported on apu1
	return 0;
}

u8 check_mpcie2_clk(void)
{
	// Not supported on apu1
	return 0;
}
u8 check_pciepm(void)
{
	// Not supported on apu1
	return 1;
}
