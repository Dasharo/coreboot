/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
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

 #include <spi-generic.h>
 #include <spi_flash.h>
 #include <cbfs_core.h>
 #include <console/console.h>
 #include <string.h>
 #include <stdlib.h>
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

static char* locate_and_map_bootorder(size_t *offset, size_t *length)
{
        const char *file_name = "bootorder";
        size_t boot_file_len = 0;
        size_t file_offset;
        struct cbfs_file *bootorder_cbfs_file = NULL;
        char* bootorder_copy;

        char *boot_file = cbfs_get_file_content(
                CBFS_DEFAULT_MEDIA, file_name, CBFS_TYPE_RAW, &boot_file_len);

        if (boot_file == NULL) {
                printk(BIOS_WARNING, "file [%s] not found in CBFS\n", file_name);
                return NULL;
        }

        if (boot_file_len < 4096) {
                printk(BIOS_WARNING, "Missing bootorder data.\n");
                return NULL;
        }
	
	if(boot_file_len & 0xFFF) {
		printk(BIOS_WARNING,"Bootorder file not multiple size of 4k\n");
		return NULL;
	}


        file_offset = cbfs_locate_file(CBFS_DEFAULT_MEDIA, bootorder_cbfs_file, file_name);

        if(file_offset == -1) {
                printk(BIOS_WARNING,"Failed to retrieve bootorder file offset\n");
                return NULL;
        }

        bootorder_copy = (char *) malloc(boot_file_len);

        if(bootorder_copy == NULL) {
                printk(BIOS_WARNING,"Failed to allocate memory for bootorder\n");
                return NULL;
        }

        if(memcpy(bootorder_copy, boot_file, boot_file_len) == NULL) {
                printk(BIOS_WARNING,"Copying bootorder failed\n");
                return NULL;
        }

        *offset = file_offset;
        *length = boot_file_len;

        return bootorder_copy;
}

static int flash_bootorder(size_t offset, size_t length, char *buffer)
{
        const struct spi_flash *flash;

        spi_init();

        flash = spi_flash_probe(0, 0);

        if (flash == NULL) {
                printk(BIOS_DEBUG, "Could not find SPI device\n");
                return -1;
        }

        if (spi_flash_erase(flash, (u32) offset, length)) {
                printk(BIOS_WARNING, "SPI erase failed\n");
                return -1;
        }

        if (spi_flash_write(flash, (u32) offset, length, buffer)) {
                printk(BIOS_WARNING, "SPI write failed\n");
                return -1;
        }

        return 0;
}

void enable_console(void)
{
        char *bootorder_file;
        size_t bootorder_length;
        size_t bootorder_offset;
        int knob_index;

        bootorder_file = locate_and_map_bootorder(&bootorder_offset, &bootorder_length);

        knob_index = find_knob_index(bootorder_file, "scon");

        if(knob_index == -1){
                printk(BIOS_WARNING,"scon knob not found in bootorder\n");
                return;
        }

        *(bootorder_file + knob_index) = '1';

        if(flash_bootorder(bootorder_offset, bootorder_length, bootorder_file)) {
                printk(BIOS_WARNING, "Failed to flash bootorder\n");
        } else {
                printk(BIOS_INFO, "Bootorder write successed\n");
        }
}
