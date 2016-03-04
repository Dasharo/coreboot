/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Eltan B.V.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <console/console.h>
#include <boot/coreboot_tables.h>
#include <string.h>
#include <cbmem.h>
//#include "../../payloads/external/SeaBIOS/seabios/src/fw/coreboot.h" // cbfs_romfile overrides
#include <boot/cbmemfile.h>

unsigned long cbmem_file_area = 0;

void
add_cbmem_file(
		const char *file_name,
		u32 file_size,
		const void *file_data )
{
	unsigned long current_file = cbmem_file_area;
	struct file_container *file_ptr = (struct file_container *) cbmem_file_area;

	if (!cbmem_file_area) {
		printk(BIOS_EMERG, "[CBMEM_FILE] Unable to add file area to CBMEM, no FILE AREA!\n");
		return;
	}
	//
	// First find the end of the already added files
	//
	while ( ( current_file < cbmem_file_area + CBMEM_FILE_AREA_SIZE ) && (file_ptr->file_signature == CBMEM_ID_FILE ) ) {

		current_file = current_file + sizeof(struct file_container) + file_ptr->file_size;
		current_file = ALIGN(current_file, 16);
		file_ptr = (struct file_container *) current_file;
	}

	//
	// Now check if we have space
	//
	if ((( current_file - cbmem_file_area ) + sizeof(struct file_container) + file_size ) < CBMEM_FILE_AREA_SIZE )  {

		//
		// Add the file
		//
		file_ptr->file_signature = CBMEM_ID_FILE;
		file_ptr->file_size = file_size;
		strncpy(file_ptr->file_name, file_name, sizeof(file_ptr->file_name));
		memcpy(file_ptr->file_data, file_data, file_size);

		printk(BIOS_INFO, "[CBMEM_FILE] Added [%s] to CBMEM - file size %d\n", file_ptr->file_name, file_ptr->file_size);
		printk(BIOS_INFO, "[CBMEM_FILE] Used %d bytes of %d avalable\n", (int) (( current_file - cbmem_file_area ) + sizeof(struct file_container) + file_size ), CBMEM_FILE_AREA_SIZE);

	} else {

		printk(BIOS_EMERG, "Unable to add file area to CBMEM\n");
	}
}

void
hide_cbmem_file(
		const char *file_name )
{
	unsigned long current_file = cbmem_file_area;
	struct file_container *file_ptr = (struct file_container *) cbmem_file_area;

	if (!cbmem_file_area) {
		printk(BIOS_EMERG, "[CBMEM_FILE] Unable to add file area to CBMEM, no FILE AREA!\n");
		return;
	}
	//
	// First find the end of the already added files
	//
	while ( ( current_file < cbmem_file_area + CBMEM_FILE_AREA_SIZE ) && (file_ptr->file_signature == CBMEM_ID_FILE ) ) {

		current_file = current_file + sizeof(struct file_container) + file_ptr->file_size;
		current_file = ALIGN(current_file, 16);
		file_ptr = (struct file_container *) current_file;
	}

	//
	// Now check if we have space
	//
	if ((( current_file - cbmem_file_area ) + sizeof(struct file_container) ) < CBMEM_FILE_AREA_SIZE )  {

		//
		// Add the file
		//
		file_ptr->file_signature = CBMEM_ID_FILE;
		file_ptr->file_size = 0;
		strncpy(file_ptr->file_name, file_name, sizeof(file_ptr->file_name));

		printk(BIOS_INFO, "[CBMEM_FILE] Hiding [%s]\n", file_ptr->file_name);
		printk(BIOS_INFO, "[CBMEM_FILE] Used %d bytes of %d avalable\n", (int) (( current_file - cbmem_file_area ) + sizeof(struct file_container) ), CBMEM_FILE_AREA_SIZE);

	} else {

		printk(BIOS_EMERG, "Unable to hide file in CBMEM\n");
	}
}

void
create_cbmem_file_area( void )
{
	cbmem_file_area = (unsigned long) cbmem_add (CBMEM_ID_FILE, CBMEM_FILE_AREA_SIZE);

	if (cbmem_file_area )
		memset( (void *) cbmem_file_area, 0, CBMEM_FILE_AREA_SIZE);
	else
		printk(BIOS_EMERG, "[CBMEM_FILE] Unable to add file area to CBMEM\n");

//	return cbmem_file_area;
}


unsigned long
get_cbmem_file_area( void ){
	return	cbmem_file_area;
}

