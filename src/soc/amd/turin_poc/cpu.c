/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/cpu.h>
#include <cpu/cpu.h>
#include <cpu/amd/msr.h>
#include <device/device.h>
#include <smp/node.h>
#include <soc/amd/common/block/psp/psp_def.h>
#include <soc/cpu.h>

static void amd_turin_cpu_init(struct device *dev)
{
	const uintptr_t psp_mmio = get_psp_mmio_base();
	msr_t msr;

	amd_cpu_init(dev);

	if (psp_mmio == 0)
		return;

	msr = rdmsr(PSP_ADDR_MSR);
	if (msr.lo == 0 && msr.hi == 0) {
		msr.lo = psp_mmio;
		msr.hi = psp_mmio >> 32;
		wrmsr(PSP_ADDR_MSR, msr);
	}
}

static struct device_operations cpu_dev_ops = {
	.init = amd_turin_cpu_init,
};

static struct cpu_device_id cpu_table[] = {
	{ X86_VENDOR_AMD, TURIN_A0_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, TURIN_B0_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, TURIN_B1_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, TURIN_C0_CPUID, CPUID_ALL_STEPPINGS_MASK },
	{ X86_VENDOR_AMD, TURIN_C1_CPUID, CPUID_ALL_STEPPINGS_MASK },
	CPU_TABLE_END
};

static const struct cpu_driver model_19 __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};
