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
#include <drivers/vpd/lib_vpd.h>
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


static int find_scon_offset(void *vpd_buffer, struct google_vpd_info *info,
			    uint32_t *scon_offset)
{
	uint32_t offset = 0x600 + sizeof(struct google_vpd_info);
	uint8_t type;
	int32_t key_len, value_len, decoded_len;
	const uint8_t *key, *value;
	uint32_t consumed = 0;
	uint32_t strings_offset = offset;

	while (consumed < info->size) {
		offset = strings_offset + consumed;
		type = *(uint8_t *)(vpd_buffer + offset);
		if (type == 0) {
			printk(BIOS_WARNING, "VPD is empty or end of VPD\n");
			break;
		} else if (type == 1) {
			consumed++;
			if (decodeLen(info->size - consumed,
				       vpd_buffer + strings_offset + consumed,
				       &key_len, &decoded_len)) {
				if (consumed + decoded_len >= info->size) {
					printk(BIOS_WARNING, "String too long\n");
					return -1;
				}
			}
			consumed += decoded_len;
			key = (uint8_t *)(vpd_buffer + strings_offset +
					  consumed);
			consumed += key_len;
			if (decodeLen(info->size - consumed,
				       vpd_buffer + strings_offset + consumed,
				       &value_len, &decoded_len)) {
				if (consumed + decoded_len >= info->size) {
					printk(BIOS_WARNING, "Value too long\n");
					return -1;
				}
			}
			consumed += decoded_len;
			value = (uint8_t *)(vpd_buffer + strings_offset +
					    consumed);
			consumed += value_len;

			if (!memcmp("scon", key, strlen("scon")) &&
			    strlen("scon") == key_len)
				break;
			else
				continue;
		} else {
			printk(BIOS_DEBUG, "Unknown entry %x\n", type);
			return -1;
		}
	}
	*scon_offset = offset;
	return consumed;
}

static int replace_scon_tag(void *vpd_buffer, void *output_buffer,
			     struct google_vpd_info *info,
			     struct region vpd_region)
{
	uint32_t strings_offset, offset = 0; 
	int consumed;
	consumed = find_scon_offset(vpd_buffer, info, &offset);
	if (consumed == -1)
		return -1;

	strings_offset = 0x600 + sizeof(struct google_vpd_info);
	char scon_entry[2 + strlen("scon") + 1 + strlen("enabled")];
	scon_entry[0] = 0x01;
	scon_entry[1] = strlen("scon");
	memcpy(&scon_entry[2], "scon", strlen("scon"));
	scon_entry[2 + strlen("scon")] = strlen("enabled");
	memcpy(&scon_entry[3 + strlen("scon")], "enabled", strlen("enabled"));

	if (*(char *)(vpd_buffer + offset) == 0x00) {
		memcpy(output_buffer + offset, scon_entry,
		       sizeof(scon_entry));
		*(char *)(output_buffer + offset + sizeof(scon_entry)) = 0;
	} else {
		memset(output_buffer + offset, 0xFF,
		       vpd_region.size - offset);
		memcpy(output_buffer + offset, scon_entry,
		       sizeof(scon_entry));
		strcpy(output_buffer + offset + sizeof(scon_entry),
		       vpd_buffer + offset + consumed);
	}
	*(uint32_t *)(output_buffer + 0x600 + 12) = info->size - 1;
	return 0;
}

void enable_console(void)
{
	int tag_size;
	struct region vpd_region;
	void *vpd_buffer, *output_buffer;
	struct google_vpd_info info;
	struct cbmem_vpd *vpd;

	if (!vpd_find("scon", &tag_size, VPD_RW)) {
		printk(BIOS_WARNING, "scon tag not found in RW_VPD\n");
		return;
	}

	if (fmap_locate_area("RW_VPD", &vpd_region)) {
		printk(BIOS_WARNING, "Could not locate RW_VPD region\n");
		return;
	}

	vpd_buffer = malloc(vpd_region.size);
	output_buffer = malloc(vpd_region.size);
	if (!vpd_buffer || !output_buffer) {
		printk(BIOS_WARNING, "Could not allocate memory\n");
		return;
	}

	vpd = (struct cbmem_vpd *)cbmem_find(CBMEM_ID_VPD);
	if (!vpd || !vpd->ro_size || !vpd->rw_size)
		return;

	fmap_read_area("RW_VPD", vpd_buffer, vpd_region.size);
	memcpy(output_buffer, vpd_buffer, vpd_region.size);

	info = *(struct google_vpd_info *)(vpd_buffer + 0x600);
	if (memcmp(info.header.magic, VPD_INFO_MAGIC,
	           sizeof(info.header.magic))) {
		printk(BIOS_WARNING, "Bad VPD info header magic\n");
		free(vpd_buffer);
		free(output_buffer);
		return;
	}

	if (replace_scon_tag(vpd_buffer, output_buffer, &info,
			     vpd_region) == -1) {
		printk(BIOS_DEBUG, "Replacing scon tag failed\n");
		free(vpd_buffer);
		free(output_buffer);
		return;
	}

	vpd->rw_size = info.size - 1;
	memcpy(&vpd->blob[vpd->ro_size],
	       output_buffer + 0x600 + sizeof(struct google_vpd_info),
	       info.size);

	hexdump(vpd, vpd->ro_size + vpd->rw_size + 16);
	hexdump(output_buffer, vpd_region.size);

	if (fmap_overwrite_area("RW_VPD", output_buffer, vpd_region.size) == -1)
		printk(BIOS_ERR, "Flashing VPD failed\n");

	free(vpd_buffer);
	free(output_buffer);
}
