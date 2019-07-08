/*
 * Copyright (C) 2010 Google Inc.
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

#ifndef __LIB_LIB_SMBIOS__
#define __LIB_LIB_SMBIOS__

#include <inttypes.h>
#include "lib/vpd_tables.h"

struct vpd_entry *vpd_create_eps(unsigned short structure_table_len,
                                 unsigned short num_structures,
                                 uint32_t eps_base);
int vpd_append_type241(uint16_t handle, uint8_t **buf,
                       size_t len, const char *uuid, uint32_t offset,
                       uint32_t size, const char *vendor,
                       const char *desc, const char *variant);
int vpd_type241_size(struct vpd_header *header);
int vpd_append_type127(uint16_t handle,
                       uint8_t **buf, size_t len);

#endif  /* __LIB_LIB_SMBIOS__ */
