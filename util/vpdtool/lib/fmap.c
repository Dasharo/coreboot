/* Copyright 2010, Google Inc.
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
 *     https://github.com/dhendrix/flashmap/blob/wiki/FmapSpec.md
 */

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "lib/lib_vpd.h"
#include "lib/fmap.h"


void fmapNormalizeAreaName(char *name) {
  assert(name);

  while (*name) {
    if (!(isascii(*name) && isalnum(*name))) {
      *name = '_';
    }
    name++;
  }
}

off_t fmapFind(const uint8_t *image, size_t image_len) {
  off_t offset;
  uint64_t sig;
  const struct fmap *header;

  assert(sizeof(sig) == strlen(FMAP_SIGNATURE));
  memcpy(&sig, FMAP_SIGNATURE, strlen(FMAP_SIGNATURE));

  /* Find 4-byte aligned FMAP signature within image.
   * According to fmap document, http://code.google.com/p/flashmap/wiki/FmapSpec
   * fmap 1.01 changes to use 64-byte alignment, but 1.00 used 4-byte.
   * For backward-compatible, uses 4-byte alignment to search.
   */
  for (offset = 0; offset + sizeof(*header) <= image_len; offset += 4) {
    if (0 != memcmp(&image[offset], &sig, sizeof(sig))) {
      continue;
    }
    header = (const struct fmap *)(&image[offset]);
    if (header->ver_major != FMAP_VER_MAJOR) {
      continue;
    }
    return offset;
  }

  return -1;
}

/*
 *  TODO: need more sanity check for fmap.
 */
int fmapGetArea(const char *name, const struct fmap *fmap,
                uint32_t *offset, uint32_t *size) {
  int i;

  assert(offset);
  assert(size);

  /* traverse whole table */
  for (i = 0; i < fmap->nareas; i++) {
    if (0 == strcmp(fmap->areas[i].name, name)) {
      *offset = fmap->areas[i].offset;
      *size = fmap->areas[i].size;
      return FMAP_OK;
    }
  }

  return FMAP_FAIL;
}
