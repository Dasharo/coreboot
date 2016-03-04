/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
 * Copyright (C) 2015 Eltan BV
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

#include <console/console.h>

#include <arch/io.h>
#include "eltanhudson.h"

void DumpClockConfiguration( void )
{
	int i;

	printk(BIOS_INFO, "Clock configuration MISC configuration registers\n");
	for ( i = 0x00 ; i <= 0x4c ; i = i + 4 ) printk(BIOS_INFO, "MISCx%02x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + MISC_BASE + i )));
	for ( i = 0x60 ; i <= 0x60 ; i = i + 4 )printk(BIOS_INFO, "MISCx%02x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + MISC_BASE + i )));
	for ( i = 0x68 ; i <= 0x78 ; i = i + 4 ) printk(BIOS_INFO, "MISCx%02x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + MISC_BASE + i )));
	for ( i = 0x80 ; i <= 0x84 ; i = i + 4 ) printk(BIOS_INFO, "MISCx%02x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + MISC_BASE + i )));
	for ( i = 0x90 ; i <= 0x9C ; i = i + 4 ) printk(BIOS_INFO, "MISCx%02x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + MISC_BASE + i )));
	for ( i = 0xC0 ; i <= 0xC4 ; i = i + 4 ) printk(BIOS_INFO, "MISCx%02x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + MISC_BASE + i )));
	for ( i = 0xD0 ; i <= 0xD4 ; i = i + 4 ) printk(BIOS_INFO, "MISCx%02x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + MISC_BASE + i )));
	for ( i = 0xF0 ; i <= 0xF4 ; i = i + 4 ) printk(BIOS_INFO, "MISCx%02x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + MISC_BASE + i )));
}



