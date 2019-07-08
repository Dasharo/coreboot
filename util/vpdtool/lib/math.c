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
 * math.c: implementations of some numerical utilities
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>

#include "lib/math.h"

/*
 * ctz - count trailing zeros
 *
 * @u:  Bit vector to count trailing zeros in.
 *
 * Counts bit positions of lower significance than that of the least significant
 * bit set. Based off of an algorithm from:
 * http://graphics.stanford.edu/~seander/bithacks.html
 *
 * returns count of trailing zeros
 */
int ctz(unsigned long long int u)
{
  int num_zeros;
  union {
    float f;
    uint32_t i;
  } alias;

  if (u == 0)   /* The algorithm will return -127 on this condition */
    return 0;

  alias.f = (float)(u & (~u + 1));
  num_zeros = (alias.i >> 23) - 0x7f;

  return num_zeros;
}

/*
 * logbase2 - Return log base 2 of the absolute value of n (2^r = abs(n)) of an
 * integer by using a cast to float method (Requires IEEE-754).
 *
 * Note: We could just use log2() but that would require messing with our
 * compilation and linking options and hacking around the n = 0 case in other
 * areas of the code.
 *
 * @n:  The number to find the log base 2 of
 *
 * returns log2(n) if successful
 */
int logbase2(int n)
{
  float f;
  int r;

  /* This algorithm fails (Returns negative infinity) if n = 0. We'll be
   * using it mostly in the context of CPU numbers, so we'll take the
   * liberty of returning 0 instead of aborting */
  if (n == 0)
    return 0;

  f = (float)n;
  memcpy(&r, &f, sizeof(n));

  /* Isolate exponent and un-bias the exponent (Subtract +128) */
  r = ((r & 0x7F800000) >> 23) - 0x80;

  return r + 1;
}

/*
 * rolling8_csum  -  Bytewise rolling summation "checksum" of a buffer
 *
 * @buf:  buffer to sum
 * @len:  length of buffer
 */
uint8_t rolling8_csum(uint8_t *buf, size_t len)
{
  size_t i;
  uint8_t sum = 0;

  for (i = 0; i < len; ++i)
    sum += buf[i];
  return sum;
}

/*
  * zero8_csum - Calculates 8-bit zero-sum checksum
  *
  * @buf:  input buffer
  * @len:  length of buffer
  *
  * The summation of the bytes in the array and the csum will equal zero
  * for 8-bit data size.
  *
  * returns checksum to indicate success
  */
uint8_t zero8_csum(uint8_t *buf, size_t len)
{
  uint8_t *u = buf;
  uint8_t csum = 0;

  while (u < buf + len) {
    csum += *u;
    u++;
  }

  return (0x100 - csum);
}
