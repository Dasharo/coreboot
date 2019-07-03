/*
 * Copyright 2010 Google Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Ported from mosys project (http://code.google.com/p/mosys/)
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <uuid/uuid.h>

#include "lib/math.h"
#include "lib/lib_smbios.h"
#include "lib/vpd.h"
#include "lib/vpd_tables.h"

/*
 * This function simply looks for the pattern of two adjacent NULL bytes
 * following the table header.
 */
int vpd_sizeof_strings(void *table)
{
  uint8_t *p;
  struct vpd_header *header = table;
  size_t size = 0, offset = 0;
  unsigned char cmp[2] = { '\0', '\0' };
  uint8_t found = 0;

  /*
   * Search for double NULL. End of strings will be one byte before the
   * final terminator which indicates end of structure.
   */
  for (p = table + header->length - 1; p; p++, offset++) {
    if (!memcmp(p, cmp, 2)) {
      found = 1;
      break;
    }
  }

  if (found)
    size = offset;

  return size;
}

/**
 * vpd_crete_eps - create an entry point structure
 *
 * @structure_table_len:  structure table len
 * @num_structures:    number of structures in structure table
 *
 * As per SMBIOS spec, the caller must place this structure on a 16-byte
 * boundary so that the anchor strings can be found.
 *
 * returns pointer to newly allocated entry point structure if successful
 * returns NULL to indicate failure
 *
 * FIXME: This function needs to be more intelligent about parsing tables and
 * obtaining information on its own. These arguments need to go away.
 */
struct vpd_entry *vpd_create_eps(uint16_t structure_table_len,
                                 uint16_t num_structures,
                                 uint32_t eps_base) {
  struct vpd_entry *eps = NULL;

  /* size of structure only, no strings */
  eps = malloc(sizeof(struct vpd_entry));
  if (!eps)
    return NULL;
  memset(eps, 0, sizeof(*eps));

  memcpy(eps->anchor_string, VPD_ENTRY_MAGIC, 4);
  /* Note: entry point length should be 0x1F for v2.6 */
  eps->entry_length = sizeof(struct vpd_entry);
  eps->major_ver = CONFIG_EPS_VPD_MAJOR_VERSION;
  eps->minor_ver = CONFIG_EPS_VPD_MINOR_VERSION;
  /* EPS revision based on version 2.1 or later */
  eps->entry_rev = 0;
  /* note: nothing done with EPS formatted area */

  /* Intermediate EPS (IEPS) stuff */
  memcpy(eps->inter_anchor_string, "_DMI_", 5);

  /* FIXME: implement vpd_table_length() and vpd_num_structures() */
  eps->table_length = structure_table_len;

  /* immediately follow the entry point structure */
  eps->table_address = eps_base + eps->entry_length;

#ifdef CONFIG_EPS_NUM_STRUCTURES
  eps->table_entry_count = CONFIG_EPS_NUM_STRUCTURES;
#else
  eps->table_entry_count = num_structures;
#endif
  eps->bcd_revision = (CONFIG_EPS_VPD_MAJOR_VERSION << 4) |
                      CONFIG_EPS_VPD_MINOR_VERSION;

  /* calculate IEPS checksum first, then the EPS checksum */
  eps->inter_anchor_cksum = zero8_csum(&eps->inter_anchor_string[0], 0xf);
  eps->entry_cksum = zero8_csum((uint8_t *)eps, eps->entry_length);

  return eps;
}

/**
 * vpd_append_type127 - append type 127 (end of table) structure
 *
 * @handle:  handle for this structure
 * @buf:  buffer to append to
 * @len:  length of buffer
 *
 * returns total size of newly re-sized buffer if successful
 * returns <0 to indicate failure
 */
int vpd_append_type127(uint16_t handle, uint8_t **buf, size_t len)
{
  struct vpd_table_eot *data;
  size_t total_len, struct_len;

  struct_len = sizeof(struct vpd_table_eot) + 2;  /* double terminator */
  total_len = len + struct_len;
  *buf = realloc(*buf, total_len);

  data = (struct vpd_table_eot *)(*buf + len);
  data->header.type = 127;
  data->header.length = sizeof(*data);
  data->header.handle = handle;

  memset(*buf + len + sizeof(*data), 0, 2);  /* double terminator */

  return total_len;
}

/**
 * vpd_append_type241 - append type 241 (binary blob pointer) structure
 *
 * @handle:  handle for this structure
 * @buf:  buffer to append to
 * @len:  length of buffer
 * @vendor:  blob vendor string
 * @desc:  blob description string
 * @variant:  blob variant string
 *
 * returns total size of newly re-sized buffer if successful
 * returns <0 to indicate failure
 */
int vpd_append_type241(uint16_t handle, uint8_t **buf,
                       size_t len, const char *uuid, uint32_t offset,
                       uint32_t size, const char *vendor,
                       const char *desc, const char *variant)
{
  struct vpd_header *header;
  struct vpd_table_binary_blob_pointer *data;
  char *string_ptr;
  size_t struct_len, total_len;
  int string_index = 1;

  /* FIXME: Add sanity checking */
  struct_len = sizeof(struct vpd_header) +
               sizeof(struct vpd_table_binary_blob_pointer);
  if (vendor)
    struct_len += strlen(vendor) + 1;
  if (desc)
    struct_len += strlen(desc) + 1;
  if (variant)
    struct_len += strlen(variant) + 1;
  struct_len += 1;  /* structure terminator */
  total_len = len + struct_len;

  *buf = realloc(*buf, total_len);
  memset(*buf + len, 0, struct_len);

  header = (struct vpd_header *)(*buf + len);
  data = (struct vpd_table_binary_blob_pointer *)
            ((uint8_t *)header + sizeof(*header));
  string_ptr = (char *)data + sizeof(*data);

  /* fill in structure header details */
  header->type = VPD_TYPE_BINARY_BLOB_POINTER;
  header->length = sizeof(*header) + sizeof(*data);
  header->handle = handle;

  data->struct_major_version = 1;
  data->struct_minor_version = 0;

  if (vendor) {
    data->vendor = string_index;
    string_index++;
    sprintf(string_ptr, "%s%c", vendor, '\0');
    string_ptr += strlen(vendor) + 1;
  }

  if (desc) {
    data->description = string_index;
    string_index++;
    sprintf(string_ptr, "%s%c", desc, '\0');
    string_ptr += strlen(desc) + 1;
  }

  data->major_version = 2;
  data->minor_version = 0;

  if (variant) {
    data->variant = string_index;
    string_index++;
    sprintf(string_ptr, "%s%c", variant, '\0');
    string_ptr += strlen(variant) + 1;
  }

  memset(&data->reserved[0], 0, 5);

  if (uuid_parse(uuid, &data->uuid[0]) < 0) {
    fprintf(stderr, "invalid UUID \"%s\" specified\n", uuid);
    goto vpd_create_type241_fail;
  }

  data->offset = offset;
  data->size = size;

  return total_len;

vpd_create_type241_fail:
  return -1;
}

/**
 * vpd_type241_size - return the size of type 241 structure table.
 *
 * Type 241 structure contains 3 variant length of string at end of table.
 * It is non-trival to get the length by sizeof(vpd_table_binary_blob_pointer).
 * This function can help by adding 3 strlen(NULL-terminated string).
 *
 * @header:  pointer to the start address of this structure table.
 *
 * returns total size of this structure table.
 * returns <0 to indicate failure
 */
int vpd_type241_size(struct vpd_header *header) {
  uint8_t *ptr = (uint8_t*)header;
  char *str = (char*)ptr + header->length;
  int length = sizeof(struct vpd_header) +
               sizeof(struct vpd_table_binary_blob_pointer);
  int i;

  /* Sanity check */
  if (header->type != VPD_TYPE_BINARY_BLOB_POINTER) return -1;

  /* Three variant-length strings are following. */
  for (i = 0; i < 3; ++i) {
    int len = strlen(str) + 1;
    length += len;
    str += len;
  }

  /* Additional null(0) to indicate end of set, so called structure terminator.
   * Refer to SMBIOS spec 3.1.3 Text Strings.
   * However, in VPD 1.x, the mosys would generate a buggy type 241 structure,
   * which missed the terminator. See mosys between r169 and r211.
   * The workaround is simple, when counting VPD 241 length, look at the byte
   * byte after variant string. If it is 0x00, means it's the structure
   * terminator. If non-zero, then it must be the next table type.
   */
  if (ptr[length] == 0) length++;  /* increase only if this is terminator. */

  return length;
}

void vpd_free_table(void *data)
{
  uint8_t *foo = data;

  /* clean-up is trivially simple, for now... */
  free(foo);
}
