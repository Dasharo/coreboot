/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "lib_vpd.h"

/* Wrappers for vpd_decode.c. */

vpd_err_t decodeVpdString(
    const uint32_t max_len, const uint8_t *input_buf, uint32_t *consumed,
    VpdDecodeCallback callback, void *callback_arg)
{
  int res = vpd_decode_string(
      max_len, input_buf, consumed, callback, callback_arg);
  return res == VPD_DECODE_OK ? VPD_OK : VPD_ERR_DECODE;
}

/* Include vpd_decode.c so we can test static functions */
#define vpd_decode_string _dummy_
#include "vpd_decode.c"

vpd_err_t decodeLen(
    const uint32_t max_len, const uint8_t *in, uint32_t *length,
    uint32_t *decoded_len)
{
  int res = vpd_decode_len(max_len, in, length, decoded_len);
  return res == VPD_DECODE_OK ? VPD_OK : VPD_ERR_DECODE;
}
