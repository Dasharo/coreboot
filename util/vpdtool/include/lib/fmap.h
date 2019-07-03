/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * Ported from mosys project (http://code.google.com/p/mosys/)
 *
 * The fmap document can be found in
 *     http://code.google.com/p/flashmap/wiki/FmapSpec.
 */

#ifndef __LIB_FMAP_H__
#define __LIB_FMAP_H__

#include <inttypes.h>

#define FMAP_SIGNATURE    "__FMAP__"
#define FMAP_VER_MAJOR    1  /* this header's FMAP minor version */
#define FMAP_VER_MINOR    0  /* this header's FMAP minor version */
#define FMAP_STRLEN    32    /* maximum length for strings, */
                             /* including null-terminator */

enum {
  FMAP_OK = 0,
  FMAP_FAIL,
};

enum fmap_flags {
  FMAP_AREA_STATIC  = 1 << 0,
  FMAP_AREA_COMPRESSED  = 1 << 1,
};

struct crypto_algo;  /* forward declaration */

/* Mapping of volatile and static regions in firmware binary */
struct fmap {
  uint8_t signature[8];    /* "__FMAP__" (0x5F5F50414D465F5F) */
  uint8_t ver_major;    /* major version */
  uint8_t ver_minor;    /* minor version */
  uint64_t base;      /* address of the firmware binary */
  uint32_t size;      /* size of firmware binary in bytes */
  char name[FMAP_STRLEN];  /* name of this firmware binary */
  uint16_t nareas;    /* number of areas described by
             fmap_areas[] below */
  struct fmap_area {
    uint32_t offset;    /* offset relative to base */
    uint32_t size;      /* size in bytes */
    char name[FMAP_STRLEN];  /* descriptive name */
    uint16_t flags;      /* flags for this area */
  }  __attribute__((packed)) areas[];
} __attribute__((packed));

/*
 * fmapNormalizeAreaName - to protect malicious attack, this function
 * replace not allowed chars into underline. Only following are allowed:
 *
 *   numbers
 *   digits
 *   space
 *
 * The allowing list should increase if fmap area name allows more chars.
 *
 * @name: point to the name.
 */
void fmapNormalizeAreaName(char *name);


/*
 * fmapFind - finds FMAP signature in a binary image
 *
 * @image:  binary image
 * @len:  length of binary image
 *
 * This function does no error checking. The caller is responsible for
 * verifying that the contents are sane.
 *
 * returns offset of FMAP signature to indicate success
 * returns <0 to indicate failure
 */
off_t fmapFind(const uint8_t *image, size_t image_len);


/*
 * fmapGetArea - finds the area in a binary image
 *
 * @name: name to search
 * @image: points to the start of image
 * @image_len: length of the image
 * @offset: pointer to store the offset of the found area
 * @size: pointer to store the size of the found area.
 *
 * This function returns FMAP_OK if found. Returns FMAP_FAIL if not found.
 *
 */
int fmapGetArea(const char *name, const struct fmap *fmap,
                uint32_t *offset, uint32_t *size);


#endif  /* __LIB_FMAP_H__*/
