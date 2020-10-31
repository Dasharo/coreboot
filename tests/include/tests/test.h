/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _TESTS_TEST_H
#define _TESTS_TEST_H

/*
 * Standard test header that should be included in all tests. For now it just encapsulates the
 * include dependencies for Cmocka. Test-specific APIs that are so generic we would want them
 * available everywhere could also be added here.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/*
 * Set symbol value and make it global.
 */
#define TEST_SYMBOL(symbol, address) asm(".set " #symbol ", " #address "\n\t.globl " #symbol)

/*
 * Define memory region for testing purpose.
 *
 * Create buffer with specified name and size.
 * Create end symbol for it.
 */
#define TEST_REGION(region, size) uint8_t _##region[size]; \
	TEST_SYMBOL(_e##region, _##region + size)

#endif /* _TESTS_TEST_H */
