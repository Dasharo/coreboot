/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
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
#include <stdint.h>
#include <device/device.h>
#include <device/pci.h>
#include "eltannorthbridge.h"

#ifdef __PRE_RAM__


void DumpLinkConfiguration( void )
{
	u8 byte;
	device_t dev = PCI_DEV(0, 0x0, 0);

	u32 index, data;

	byte = pci_read_config8(dev, 0x4a);
	byte |= 1 << 5; /* enable port 80 */
	pci_write_config8(dev, 0x4a, byte);


	index = 0x01300080;
	pci_write_config32(dev, 0xE0, index);
	data = pci_read_config32( dev, 0xE4);
	printk(BIOS_INFO, "LINK register 0x%08x contains 0x%08x \n", index, data);

	index = 0x01100011;
	pci_write_config32(dev, 0xE0, index);
	data = pci_read_config32( dev, 0xE4);
	printk(BIOS_INFO, "LINK register 0x%08x contains 0x%08x \n", index, data);
}

#else // __PRE_RAM__


u32
ReadLinkConfigItem(
		u32 Index
		);

u32
ReadLinkConfigItem(
		u32 Index
		)
{
	device_t link_dev = dev_find_slot( 0, PCI_DEVFN( 0x0, 0));

	pci_write_config32(link_dev, 0xE0, Index);
	return pci_read_config32( link_dev, 0xE4);
}

u32
ReadRpConfigItem(
		u8	port,
		u32 Index
		);

u32
ReadRpConfigItem(
		u8	port,
		u32 Index
		)
{
	device_t link_dev = dev_find_slot( 0, PCI_DEVFN( 0x2, port));

	pci_write_config32(link_dev, 0xE0, Index);
	return pci_read_config32( link_dev, 0xE4);
}


void
DumpLinkConfiguration( void )
{
	u32 index;
	u8	i;

	printk(BIOS_INFO, "PCIe LANE CONFIGURATION REGISTERS START\n");

	for ( index = 0x01100010 ; index <= 0x01100013 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01100017 ; index <= 0x01100018 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01200004 ; index <= 0x01200004 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01204440 ; index <= 0x01204440 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01204450 ; index <= 0x01204450 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300046 ; index <= 0x01300046 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300080 ; index <= 0x01300080 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300800 ; index <= 0x01300800 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300803 ; index <= 0x01300805 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300900 ; index <= 0x01300900 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300903 ; index <= 0x01300905 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300A00 ; index <= 0x01300A00 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300A03 ; index <= 0x01300A05 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300B00 ; index <= 0x01300B00 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300B03 ; index <= 0x01300B05 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300C00 ; index <= 0x01300C00 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01300C03 ; index <= 0x01300C05 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01308002 ; index <= 0x01308002 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01308011 ; index <= 0x01308016 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01308021 ; index <= 0x01308029 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01308060 ; index <= 0x01308060 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01308062 ; index <= 0x01308062 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x013080f0 ; index <= 0x013080f1 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01400002 ; index <= 0x01400002 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01400010 ; index <= 0x01400011 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x0140001C ; index <= 0x0140001C ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01400020 ; index <= 0x01400020 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x01400040 ; index <= 0x01400040 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x014000b0 ; index <= 0x014000b0 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));
	for ( index = 0x014000c0 ; index <= 0x014000c1 ; index++ ) printk(BIOS_INFO, "LINKx%08x = 0x%08x \n", index, ReadLinkConfigItem(index));

	printk(BIOS_INFO, "PCIe LANE CONFIGURATION REGISTERS END\n");

	printk(BIOS_INFO, "ROOT PORT CONFIGURATION REGISTERS\n");

	for ( i = 1 ; i <= 5; i++ ) {

		printk(BIOS_INFO, "ROOT PORT 2.%d\n", i);
		index = 0x20; printk(BIOS_INFO, "D2F%dx%02x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x50; printk(BIOS_INFO, "D2F%dx%02x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x6A; printk(BIOS_INFO, "D2F%dx%02x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x70; printk(BIOS_INFO, "D2F%dx%02x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		for ( index = 0xA0 ; index <= 0xA5 ; index++ ) printk(BIOS_INFO, "D2F%dx%02x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0xB1; printk(BIOS_INFO, "D2F%dx%02x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0xB5; printk(BIOS_INFO, "D2F%dx%02x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		for ( index = 0xC0 ; index <= 0xC1 ; index++ ) printk(BIOS_INFO, "D2F%dx%02x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0xD0;  printk(BIOS_INFO, "D2F%dx%02x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x100; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x104; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x108; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x10C; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x128; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x150; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x154; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x158; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x15C; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x160; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x164; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x168; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x16C; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x170; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x174; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x178; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x17C; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x180; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x184; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
		index = 0x184; printk(BIOS_INFO, "D2F%dx%03x = 0x%08x \n", i, index, ReadRpConfigItem(i, index));
	}

	printk(BIOS_INFO, "ROOT PORT CONFIGURATION REGISTERS END\n");
}

#endif // __PRE_RAM__
