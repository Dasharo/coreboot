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

#ifndef _CBMEM_FILE_H_
#define _CBMEM_FILE_H_

#define CBMEM_FILE_AREA_SIZE 	CONFIG_CBMEM_FILE_AREA_SIZE

struct cbmem_entry {
	u32 magic;
	u32 id;
	u64 base;
	u64 size;
};

//
// Intended for cbfs file overrides
//
struct file_container {
    u32  file_signature;   /* "FILE" */
    u32  file_size;        /* size of file_data[] */
    char file_name[32];
    char file_data[0];     /* Variable size */
};

#define CBMEM_ID_FILE  			0x46494c45 //'FILE'
#define CB_TAG_FILE 			0x25
#define CBMEM_MAGIC         	0x434f5245

struct cbfile_record {
    u32 tag;
    u32 size;
    u64 forward;
};


void
add_cbmem_file(
		const char *file_name,
		u32 file_size,
		const void *file_data );

void
hide_cbmem_file(
		const char *file_name );

void
create_cbmem_file_area( void );

unsigned long
get_cbmem_file_area( void );

#endif // _CBMEM_FILE_H_

