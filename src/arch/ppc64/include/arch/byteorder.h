/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _BYTEORDER_H
#define _BYTEORDER_H

#define __BIG_ENDIAN 4321

#define PPC_BIT(bit)		(0x8000000000000000UL >> (bit))
#define PPC_BITMASK(bs,be)	((PPC_BIT(bs) - PPC_BIT(be)) | PPC_BIT(bs))

#ifndef __ASSEMBLER__

#include <types.h>

/*
 * Assigns part of a 64-bit value: lhs[pos:pos + len] = rhs
 */
#define PPC_INSERT(lhs, rhs, pos, len) do { \
		uint64_t __placed = PPC_PLACE(rhs, pos, len); \
		uint64_t __mask = PPC_BITMASK(pos, (pos) + (len) - 1); \
		(lhs) = ((lhs) & ~__mask) | __placed; \
	} while (0)

/*
 * The pos parameter specifies MSB/leftmost bit.  Passing compile-time constants
 * (literals or expressions) for parameters allows for the following
 * compile-time checks (not all are performed, depends on which parameter values
 * are known at compile-time):
 *  - pos is in range [0; 63]
 *  - len is in range [1; 64]
 *  - (pos + len) <= 64
 *  - (val & ~len-based-mask) == 0
 */
#define PPC_PLACE(val, pos, len) \
	/* Incorrect arguments detected in PPC_PLACE */ __builtin_choose_expr( \
		PPC_PLACE_GOOD_ARGS(val, pos, len), \
		PPC_PLACE_IMPL(val, pos, len), \
		(void)0)

#define PPC_PLACE_GOOD_ARGS(val, pos, len) ( \
	/* pos value */ \
	__builtin_choose_expr( \
		__builtin_constant_p(pos), \
		((pos) >= 0) && ((pos) <= 63), \
		1) && \
	/* len value */ \
	__builtin_choose_expr( \
		__builtin_constant_p(len), \
		((len) >= 1) && ((len) <= 64), \
		1) && \
	/* range */ \
	__builtin_choose_expr( \
		__builtin_constant_p(pos) && __builtin_constant_p(len), \
		(pos) + (len) <= 64, \
		1) && \
	/* value */ \
	__builtin_choose_expr( \
		__builtin_constant_p(val) && __builtin_constant_p(len), \
		((val) & ~(((uint64_t)1 << (len)) - 1)) == 0, \
		1) \
	)

#define PPC_PLACE_IMPL(val, pos, len) \
	PPC_SHIFT((val) & (((uint64_t)1 << (len)) - 1), ((pos) + ((len) - 1)))

#define PPC_SHIFT(val, lsb)	(((uint64_t)(val)) << (63 - (lsb)))

/* Sanity checks and usage examples for PPC_PLACE */
_Static_assert(PPC_PLACE(0x12345, 0, 20) == 0x1234500000000000, "");
_Static_assert(PPC_PLACE(0x12345, 0, 24) == 0x0123450000000000, "");
_Static_assert(PPC_PLACE(0x12345, 8, 24) == 0x0001234500000000, "");

#else
#define PPC_SHIFT(val, lsb)	((val) << (63 - (lsb)))
#endif

#endif /* _BYTEORDER_H */
