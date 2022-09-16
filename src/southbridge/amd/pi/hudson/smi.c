/* SPDX-License-Identifier: GPL-2.0-or-later */

/*
 * Utilities for SMM setup
 */

#include <cbmem.h>
#include <cpu/x86/smm.h>

void smm_region(uintptr_t *start, size_t *size)
{
	*start = (uintptr_t)cbmem_top();
	*size = CONFIG_SMM_TSEG_SIZE;
}
