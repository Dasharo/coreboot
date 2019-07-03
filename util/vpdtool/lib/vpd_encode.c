/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib_vpd.h"


/* Encodes the len into multiple bytes with the following format.
 *
 *    7   6 ............ 0
 *  +----+------------------+
 *  |More|      Length      |  ...
 *  +----+------------------+
 *
 * Returns fail if the buffer is not long enough.
 */
vpd_err_t encodeLen(
    const int32_t len,
    uint8_t *encode_buf,
    const int32_t max_len,
    int32_t *encoded_len) {
  unsigned int shifting;
  unsigned int reversed_7bits = 0;
  int out_index = 0;

  assert(encoded_len);

  if (len < 0) return VPD_ERR_INVALID;
  shifting = len;
  /* reverse the len for every 7-bit. The little endian. */
  for (*encoded_len = 0; shifting; (*encoded_len)++) {
    reversed_7bits = (reversed_7bits << 7) | (shifting & 0x7f);
    shifting >>= 7;
  }
  if (*encoded_len > max_len) return VPD_ERR_OVERFLOW;
  if (!*encoded_len) *encoded_len = 1;  /* output at least 1 byte */

  /* Output in reverse order, now big endian. */
  while (out_index < *encoded_len) {
    /* always set MORE flag */
    encode_buf[out_index++] = 0x80 | (reversed_7bits & 0x7f);
    reversed_7bits >>= 7;
  }
  encode_buf[out_index - 1] &= 0x7f;  /* clear the MORE flag in last byte */

  return VPD_OK;
}

/*  Encodes the terminator.
 */
vpd_err_t encodeVpdTerminator(
    const int max_buffer_len,
    uint8_t *output_buf,
    int *generated_len) {
  assert(generated_len);

  if (*generated_len >= max_buffer_len) return VPD_FAIL;

  output_buf += *generated_len;  /* move cursor to end of string */
  *(output_buf++) = VPD_TYPE_TERMINATOR;
  (*generated_len)++;

  return VPD_OK;
}

/* Encodes a string with padding support. */
vpd_err_t encodeVpdString(
    const uint8_t *key,
    const uint8_t *value,
    const int pad_value_len,
    const int max_buffer_len,
    uint8_t *output_buf,
    int *generated_len) {
  int key_len, value_len;
  int ret_len;
  int pad_len = 0;
  vpd_err_t retval;

  assert(generated_len);

  key_len = strlen((char*)key);
  value_len = strlen((char*)value);
  output_buf += *generated_len;  /* move cursor to end of string */

  /* encode type */
  if (*generated_len >= max_buffer_len) return VPD_ERR_OVERFLOW;
  *(output_buf++) = VPD_TYPE_STRING;
  (*generated_len)++;

  /* encode key len */
  if (VPD_OK != encodeLen(key_len, output_buf,
                          max_buffer_len - *generated_len, &ret_len))
    return VPD_FAIL;
  output_buf += ret_len;
  *generated_len += ret_len;
  /* encode key string */
  if (*generated_len + key_len > max_buffer_len) return VPD_ERR_OVERFLOW;
  memcpy(output_buf, key, key_len);
  output_buf += key_len;
  *generated_len += key_len;

  /* count padding length */
  if (pad_value_len != VPD_AS_LONG_AS) {
    if (value_len < pad_value_len) {
      pad_len = pad_value_len - value_len;
    } else {
      value_len = pad_value_len;
    }
  }

  /* encode value len */
  retval = encodeLen(value_len + pad_len, output_buf,
                     max_buffer_len - *generated_len, &ret_len);
  if (VPD_OK != retval)
    return retval;
  output_buf += ret_len;
  *generated_len += ret_len;
  /* encode value string */
  if (*generated_len + value_len > max_buffer_len) return VPD_ERR_OVERFLOW;
  memcpy(output_buf, value, value_len);
  output_buf += value_len;
  *generated_len += value_len;
  /* encode padding (if applicable) */
  if (*generated_len + pad_len > max_buffer_len) return VPD_ERR_OVERFLOW;
  memset(output_buf, 0, pad_len);
  output_buf += pad_len;
  *generated_len += pad_len;

  return VPD_OK;
}
