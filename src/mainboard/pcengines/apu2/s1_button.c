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

#include <stdlib.h>
#include <string.h>
#include <boot_device.h>
#include <cbmem.h>
#include <fmap.h>
#include <lib.h>
#include <spi_flash.h>
#include <spi-generic.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <drivers/vpd/vpd.h>
#include <drivers/vpd/vpd_tables.h>

#include "s1_button.h"

struct cbmem_vpd {
	uint32_t magic;
	uint32_t version;
	uint32_t ro_size;
	uint32_t rw_size;
	uint8_t blob[0];
	/* The blob contains both RO and RW data. It starts with RO (0 ..
	 * ro_size) and then RW (ro_size .. ro_size+rw_size).
	 */
};

static int flash_vpd(size_t offset, size_t fsize, char *buffer)
{
	const struct spi_flash *flash;

	flash = boot_device_spi_flash();

	if (flash == NULL) {
		printk(BIOS_WARNING, "Can't get boot flash device\n");
		return -1;
	}

	if (spi_flash_erase(flash, (u32) offset, fsize)) {
		printk(BIOS_WARNING, "SPI erase failed\n");
		return -1;
	}

	if (spi_flash_write(flash, offset, fsize, buffer)) {
		printk(BIOS_WARNING, "SPI write failed\n");
		return -1;
	}

	printk(BIOS_INFO, "VPD write successed\n");
	return 0;
}

void enable_console(void)
{
	int tag_size;
	struct region vpd_region;
	void *vpd_buffer, *output_buffer;
	struct google_vpd_info info;

	if (!vpd_find("scon", &tag_size, VPD_RW)) {
		printk(BIOS_WARNING, "scon tag not found in RW_VPD\n");
		return;
	}

	if (fmap_locate_area("RW_VPD", &vpd_region)) {
		printk(BIOS_WARNING, "Could not locate RW_VPD region\n");
		return;
	}

	free(vpd_buffer);
	vpd_buffer = malloc(vpd_region.size);
	output_buffer = malloc(vpd_region.size);
	if (!vpd_buffer || !output_buffer) {
		printk(BIOS_WARNING, "Could not allocate memory\n");
		return;
	}

	fmap_read_area("RW_VPD", vpd_buffer, vpd_region.size);
	memcpy(output_buffer, vpd_buffer, vpd_region.size);

	info = *(struct google_vpd_info *)(vpd_buffer + 0x600);
	if (memcmp(info.header.magic, VPD_INFO_MAGIC,
	           sizeof(info.header.magic))) {
		printk(BIOS_WARNING, "Bad VPD info header magic\n");
		free(vpd_buffer);
		return;
	}

	uint32_t offset = 0x600 + sizeof(struct google_vpd_info);
	uint8_t type, key_len, value_len;

	struct cbmem_vpd *vpd;

	vpd = (struct cbmem_vpd *)cbmem_find(CBMEM_ID_VPD);
	if (!vpd || !vpd->ro_size)
		return;

	while (offset < vpd_region.size) {
		type = *(uint8_t *)(vpd_buffer + offset);
		key_len = *(uint8_t *)(vpd_buffer + offset + 1);
		if (type == 0) {
			printk(BIOS_WARNING, "VPD is empty\n");
			free(vpd_buffer);
			free(output_buffer);
			return;
		}
		else if (type == 1) {
			if (memcmp("scon", (vpd_buffer + offset + 2),
			           key_len)) {
				offset += key_len;
				value_len = *(uint8_t *)(vpd_buffer + offset);
				offset += value_len;
				continue;
			} else
				break;
		}
	}
	uint32_t offset_scon = offset;

	type = *(uint8_t *)(vpd_buffer + offset++);
	key_len = *(uint8_t *)(vpd_buffer + offset++);
	value_len = *(uint8_t *)(vpd_buffer + key_len + offset++);
	offset += key_len;
	offset += value_len;

	char scon_entry[2 + strlen("scon") + 1 + strlen("enabled")];
	scon_entry[0] = 0x01;
	scon_entry[1] = strlen("scon");
	memcpy(&scon_entry[2], "scon", strlen("scon"));
	scon_entry[2 + strlen("scon")] = strlen("enabled");
	memcpy(&scon_entry[3 + strlen("scon")], "enabled", strlen("enabled"));

	if (*(char *)(vpd_buffer + offset) == 0x00) {
		memcpy(output_buffer + offset_scon, scon_entry,
		       sizeof(scon_entry));
		*(char *)(output_buffer + offset_scon + sizeof(scon_entry)) = 0;
	} else {
		memset(output_buffer + offset_scon, 0xFF,
		       vpd_region.size - offset_scon);
		memcpy(output_buffer + offset_scon, scon_entry,
		       sizeof(scon_entry));
		strcpy(output_buffer + offset_scon +
					sizeof(scon_entry),
					vpd_buffer + offset);
	}
	*(uint32_t *)(output_buffer + 0x600 + 12) = info.size - 1;
	vpd->rw_size = info.size - 1;
	memcpy(&vpd->blob[vpd->ro_size],
	       output_buffer + 0x600 + sizeof(struct google_vpd_info),
	       info.size - 1);

	if(flash_vpd(vpd_region.offset, vpd_region.size, output_buffer)) {
		printk(BIOS_WARNING, "Failed to flash VPD\n");
	}
}
