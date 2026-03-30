/* SPDX-License-Identifier: GPL-2.0-only */

/* TODO: Update for Phoenix */

#include <amdblocks/cpu.h>
#include <cpu/amd/msr.h>
#include <cpu/cpu.h>
#include <device/device.h>
#include <smp/node.h>
#include <soc/amd/common/block/psp/psp_def.h>
#include <soc/cpu.h>

_Static_assert(CONFIG_MAX_CPUS == 16, "Do not override MAX_CPUS. To reduce the number of "
	"available cores, use the downcore_mode and disable_smt devicetree settings instead.");

static void amd_phoenix_cpu_init(struct device *dev)
{
	msr_t ccp_addr_base;
	uint64_t ccp_mmio_base;

	if (boot_cpu()) {
		ccp_addr_base = rdmsr(PSP_ADDR_MSR);
		ccp_mmio_base = get_ccp_mmio_base();
		if (ccp_addr_base.raw == 0ul && ccp_mmio_base != 0ul) {
			ccp_addr_base.raw = ccp_mmio_base;
			wrmsr(PSP_ADDR_MSR, ccp_addr_base);
			printk(BIOS_SPEW, "Wrote PSP_ADDR_MSR 0x%llx to BSP\n", rdmsr(PSP_ADDR_MSR).raw);
		}
	}

	amd_cpu_init(dev);
}

static struct device_operations cpu_dev_ops = {
	.init = amd_phoenix_cpu_init,
};

static struct cpu_device_id cpu_table[] = {
	{ X86_VENDOR_AMD, PHOENIX_A0_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, PHOENIX2_A0_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, PHOENIX_A0_AM5_CPUID, CPUID_ALL_STEPPINGS_MASK },
	CPU_TABLE_END
};

static const struct cpu_driver zen_2_3 __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};
