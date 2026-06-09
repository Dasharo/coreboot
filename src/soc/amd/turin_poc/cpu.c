/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/cpu.h>
#include <bootmem.h>
#include <cpu/cpu.h>
#include <cpu/amd/msr.h>
#include <cpu/amd/mtrr.h>
#include <device/device.h>
#include <memrange.h>
#include <smp/node.h>
#include <soc/cpu.h>
#include <string.h>

#include <xPRF-api.h>

#define RMP_CFG_MSR 0xC0010136

static uintptr_t allocate_memory_for_rmp(const resource_t size,
					 const resource_t align)
{
	struct memranges mem;
	const struct range_entry *r;
	const struct range_entry *r_max = NULL;
	unsigned long mask = IORESOURCE_MEM | IORESOURCE_FIXED |
			     IORESOURCE_STORED | IORESOURCE_ASSIGNED |
			     IORESOURCE_RESERVE | IORESOURCE_CACHEABLE;

	/* Search all RAM memory resources except reserved RAM */
	memranges_init(&mem, mask, mask & (~IORESOURCE_RESERVE), BM_MEM_RAM);

	memranges_each_entry(r, &mem) {
		if (range_entry_tag(r) != BM_MEM_RAM)
			continue;

		if (ALIGN_DOWN(range_entry_end(r) - size, align) <
		    range_entry_base(r))
			continue;

		/* Find the highest possible address for RMP */
		if (r_max == NULL)
			r_max = r;
		else if (r->begin > r_max->begin)
			r_max = r;
	}

	if (r_max == NULL) {
		printk(BIOS_ERR, "Could not find suitable memory for RMP\n");
		return 0;
	}

	return ALIGN_DOWN(range_entry_end(r_max) - size, align);
}

static void amd_turin_cpu_init(struct device *dev)
{
	CCXCLASS_DATA_BLK *ccx_data;
	static msr_t rmp_cfg;
	static msr_t sys_cfg;
	static uint64_t rmp_base = 0;
	static uint64_t rmp_size = 0;

	amd_cpu_init(dev);

	if (boot_cpu()) {
		rmp_cfg = rdmsr (RMP_CFG_MSR);
		sys_cfg = rdmsr (SYSCFG_MSR);

		ccx_data = SilFindStructure(SilId_CcxClass, 0);
		if (ccx_data != NULL && ccx_data->CcxOutputBlock.AmdIsSnpSupported) {
			rmp_size = ccx_data->CcxOutputBlock.AmdRmpTableSize;
			if (!rmp_size)
				return;
		}

		/*
		 * OpenSIL can't allocate memory on its own, so we have to find
		 * highest memory available to reserve for RMP.
		 */
		rmp_base = allocate_memory_for_rmp(rmp_size, 1 * MiB);

		/* Update the RMP base in OpenSIL data to be later used by bootmem */
		if (rmp_base)
			ccx_data->CcxOutputBlock.AmdRmpTableBase = rmp_base;

		/*
		 * Clear the RMP table memory.
		 * FIXME: RMP may not be covered by page tables. Avoid page faults for now.
		 * if (rmp_base && rmp_size)
		 * 	memset((void *)rmp_base, 0, rmp_size);
		 */
	}

	if (rmp_base && rmp_size)
		xPrfSetSnpRmp (rmp_base, rmp_size);

	/* Sync RMP_CFG and SYS_CFG MSR on APs */
	wrmsr (RMP_CFG_MSR, rmp_cfg);
	wrmsr (SYSCFG_MSR, sys_cfg);
}

static struct device_operations cpu_dev_ops = {
	.init = amd_turin_cpu_init
};

static struct cpu_device_id cpu_table[] = {
	{ X86_VENDOR_AMD, TURIN_Ax_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, TURIN_Bx_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, TURIN_Cx_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, TURIN_DENSE_Ax_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, TURIN_DENSE_Bx_CPUID, CPUID_ALL_STEPPINGS_MASK },
	CPU_TABLE_END
};

static const struct cpu_driver model_19 __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};
