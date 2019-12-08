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

#include <ctype.h>
#include <stdint.h>
#include <console/console.h>
#include <program_loading.h>
#include <cbfs.h>
#include <commonlib/cbfs.h>
#include <commonlib/region.h>
#include <commonlib/cbfs_serialized.h>
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

int check_boost(void)
{
	u8 boost;

	//
	// Find the CPU boost item
	//
	boost = check_knob_value("boosten");

	switch (boost) {
	case 0:
		return 0;
		break;
	case 1:
		return 1;
		break;
	default:
		printk(BIOS_INFO,
			"Missing or invalid boost knob, disable CPU boost.\n");
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
	u8 ehci0;

	//
	// Find the EHCI0 item
	//
	ehci0 = check_knob_value("ehcien");

	switch (ehci0) {
	case 0:
		return 0;
		break;
	case 1:
		return 1;
		break;
	default:
		printk(BIOS_INFO,
			"Missing or invalid ehci0 knob, enable ehci0.\n");
		break;
	}

	return 1;
}

u8 check_mpcie2_clk(void)
{
	u8 mpcie2_clk;

	//
	// Find the mPCIe2 clock item
	//
	mpcie2_clk = check_knob_value("mpcie2_clk");

	switch (mpcie2_clk) {
	case 0:
		return 0;
		break;
	case 1:
		return 1;
		break;
	default:
		printk(BIOS_INFO, "Missing or invalid mpcie2_clk knob, forcing"
			" CLK of mPCIe2 slot is not enabled.\n");
		break;
	}

	return 0;
}

u8 check_sd3_mode(void)
{
	u8 sd3mode;

	//
	// Find the SD 3.0 mode item
	//
	sd3mode = check_knob_value("sd3mode");

	switch (sd3mode) {
	case 0:
		return 0;
		break;
	case 1:
		return 1;
		break;
	default:
		printk(BIOS_INFO, "Missing or invalid sd3mode knob."
				  " Disable SD3.0 mode.\n");
		break;
	}

	return 0;
}

static int _valid(char ch, int base)
{
	char end = (base > 9) ? '9' : '0' + (base - 1);

	/* all bases will be some subset of the 0-9 range */

	if (ch >= '0' && ch <= end)
		return 1;

	/* Bases > 11 will also have to match in the a-z range */

	if (base > 11) {
		if (tolower(ch) >= 'a' &&
		    tolower(ch) <= 'a' + (base - 11))
			return 1;
	}

	return 0;
}

/* Return the "value" of the character in the given base */

static int _offset(char ch, int base)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else
		return 10 + tolower(ch) - 'a';
}


static unsigned long long int strtoull(const char *ptr, char **endptr, int base)
{
	unsigned long long int ret = 0;

	if (endptr != NULL)
		*endptr = (char *) ptr;

	/* Purge whitespace */

	for( ; *ptr && isspace(*ptr); ptr++);

	if (!*ptr)
		return 0;

	/* Determine the base */

	if (base == 0) {
		if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
			base = 16;
		else if (ptr[0] == '0') {
			base = 8;
			ptr++;
		}
		else
			base = 10;
	}

	/* Base 16 allows the 0x on front - so skip over it */

	if (base == 16) {
		if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X') &&
		    _valid(ptr[2], base))
			ptr += 2;
	}

	for( ; *ptr && _valid(*ptr, base); ptr++)
		ret = (ret * base) + _offset(*ptr, base);

	if (endptr != NULL)
		*endptr = (char *) ptr;

	return ret;
}

#define ULONG_MAX	((unsigned long int)~0UL)

static unsigned long int strtoul(const char *ptr, char **endptr, int base)
{
	unsigned long long val = strtoull(ptr, endptr, base);
	if (val > ULONG_MAX) return ULONG_MAX;
	return val;
}

u16 get_watchdog_timeout(void)
{
	const char *boot_file = NULL;
	size_t boot_file_len = 0;
	u16 timeout;

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

	timeout = (u16) strtoul(findstr(boot_file, "watchdog"), NULL, 16);

	return timeout;
}