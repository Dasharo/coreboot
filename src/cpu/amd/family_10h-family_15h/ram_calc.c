/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/amd/common/common.h>
#include <cpu/amd/common/nums.h>
#include <device/pci_ops.h>
#include <device/device.h>
#include <device/pci.h>
#include <cbmem.h>

#include "ram_calc.h"

uint64_t get_uma_memory_size(uint64_t topmem)
{
	uint64_t uma_size = 0;
	if (CONFIG(GFXUMA)) {
		/* refer to UMA Size Consideration in 780 BDG. */
		if (topmem >= 0x40000000)	/* 1GB and above system memory */
			uma_size = 0x10000000;	/* 256M recommended UMA */

		else if (topmem >= 0x20000000)	/* 512M - 1023M system memory */
			uma_size = 0x8000000;	/* 128M recommended UMA */

		else if (topmem >= 0x10000000)	/* 256M - 511M system memory */
			uma_size = 0x4000000;	/* 64M recommended UMA */
	}

	return uma_size;
}

uint64_t get_cc6_memory_size()
{
	uint8_t enable_cc6;

	uint64_t cc6_size = 0;

	if (is_fam15h()) {
		enable_cc6 = 0;

		if (pci_read_config32(NODE_PCI(0, 2), 0x118) & (0x1 << 18))
			enable_cc6 = 1;

		if (enable_cc6) {
			/* Preserve the maximum possible CC6 save region
			 * This needs to be kept in sync with
			 * amdfam10_domain_read_resources() in northbridge.c
			 */
			cc6_size = 0x8000000;
		}
	}

	return cc6_size;
}

void *cbmem_top_chipset(void)
{
	uint32_t topmem = rdmsr(TOP_MEM).lo;

	/* If there is memory above 4G, CC6 storage will not be under 4G */
	if (rdmsr(TOP_MEM2).hi != 0)
		return (void *) topmem - get_uma_memory_size(topmem);
	else
		return (void *) topmem - get_uma_memory_size(topmem) - get_cc6_memory_size();
}
