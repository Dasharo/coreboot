/*
 * This file is part of the coreboot project.

 *
 * Copyright (C) 2017 3mdeb
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
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
		if ( *lpattern == 0)  // the pattern matches return the pointer
			return result;
		if ( *result == 0)  // We're at the end of the file content but don't have a patter match yet
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
	boot_file = cbfs_boot_map_with_leak(BOOTORDER_FILE, CBFS_TYPE_RAW, &boot_file_len);
	if (boot_file == NULL)
		printk(BIOS_ALERT, "file [%s] not found in CBFS\n", BOOTORDER_FILE);
	if (boot_file_len < 4096)
		printk(BIOS_ALERT, "Missing bootorder data.\n");
	if (boot_file == NULL || boot_file_len < 4096)
		return -1;

	token = findstr( boot_file, s );

	if (token) {
		if (*token == '0') return 0;
		if (*token == '1') return 1;
	}

	return -1;
}

bool check_console(void)
{

}

static bool check_uart(char uart_letter)
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
		return false;
		break;
	case 1:
		return true;
		break;
	default:
		printk(BIOS_EMERG, "Missing or invalid uart knob, disable port.\n");
		break;
	}

	return false;
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

	return true;
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

u8 check_pciepm(void)
{
	u8 pciepm;

	//
	// Find the mPCIe2 clock item
	//
	pciepm = check_knob_value("pciepm");

	switch (pciepm) {
	case 0:
		return 0;
		break;
	case 1:
		return 1;
		break;
	default:
		printk(BIOS_INFO, "Missing or invalid pciepm knob, disabling"
			"PCIe power management features.\n");
		break;
	}

	return 0;
}

u8 check_pciereverse(void)
{
	u8 pciereverse;
	pciereverse = check_knob_value("pciereverse");

	switch (pciereverse) {
	case 0:
		return 0;
		break;
	case 1:
		return 1;
		break;
	default:
		printk(BIOS_INFO, "Missing or invalid pciereverse knob, "
			"PCIe order remains unchanged.\n");
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

	return false;
}
