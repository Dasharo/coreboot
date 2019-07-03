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
 * Ported from mosys project (http://code.google.com/p/mosys/).
 */

#ifndef __LIB_VPD_H__
#define __LIB_VPD_H__


/* VPD Table Types */
enum vpd_types {
  VPD_TYPE_FIRMWARE = 0,
  VPD_TYPE_SYSTEM,
  VPD_TYPE_END                  = 127,
  VPD_TYPE_BINARY_BLOB_POINTER  = 241,
};


#define GOOGLE_SPD_OFFSET 0x400
#define GOOGLE_SPD_UUID "75f4926b-9e43-4b32-8979-eb20c0eda76a"
#define GOOGLE_SPD_VENDOR "Google"
#define GOOGLE_SPD_DESCRIPTION "Google SPD"
#define GOOGLE_SPD_VARIANT ""

#define GOOGLE_VPD_2_0_OFFSET 0x600
#define GOOGLE_VPD_2_0_UUID "0a7c23d3-8a27-4252-99bf-7868a2e26b61"
#define GOOGLE_VPD_2_0_VENDOR "Google"
#define GOOGLE_VPD_2_0_DESCRIPTION "Google VPD 2.0"
#define GOOGLE_VPD_2_0_VARIANT ""

#define GOOGLE_VPD_1_2_OFFSET 0x100
#define GOOGLE_VPD_1_2_UUID "08f8a2b0-15fd-4cfd-968f-8378f2c508ce"
#define GOOGLE_VPD_1_2_VENDOR "Google"
#define GOOGLE_VPD_1_2_DESCRIPTION "Google VPD 1.2"
#define GOOGLE_VPD_1_2_VARIANT ""

#define CONFIG_EPS_VPD_MAJOR_VERSION 2
#define CONFIG_EPS_VPD_MINOR_VERSION 6

#endif /* __LIB_VPD_H__ */
