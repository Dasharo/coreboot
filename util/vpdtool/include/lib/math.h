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

#ifndef __LIB_MATH_H__
#define __LIB_MATH_H__

#include <inttypes.h>
#include <sys/types.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/*
 * Count the number of low-order 0 bits.
 */
extern int ctz(unsigned long long int u);

/*
 * Get the integral log2 of a number.
 *
 * If n is not a perfect power of 2, this function will return return the
 * log2 of the largest power of 2 less than n.
 *
 * If n is negative, this functions will return the log of abs(n).
 */
extern int logbase2(int n);

/*
 * rolling8_csum  -  Bytewise rolling summation "checksum" of a buffer
 *
 * @buf:  buffer to sum
 * @len:  length of buffer
 */
extern uint8_t rolling8_csum(uint8_t *buf, size_t len);

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
extern uint8_t zero8_csum(uint8_t *buf, size_t len);

#ifndef __mask
# define __mask(high, low) ((1ULL << (high)) + \
                            (((1ULL << (high)) - 1) - ((1ULL << (low)) - 1)))
#endif

#ifndef __min
# define __min(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef __max
# define __max(a, b)  ((a) > (b) ? (a) : (b))
#endif

#ifndef __abs
# define __abs(x) (x < 0 ? -x : x)
#endif

#endif /* __LIB_MATH_H__ */
