/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpigen.h>
#include <acpi/acpi.h>
#include <assert.h>
#include <console/console.h>
#include <cpu/cpu.h>
#include <cpu/intel/microcode.h>
#include <cpu/intel/turbo.h>
#include <cpu/x86/lapic.h>
#include <cpu/x86/mp.h>
#include <cpu/x86/mtrr.h>
#include <intelblocks/cpulib.h>
#include <intelblocks/mp_init.h>
#include <soc/cpu.h>
#include <soc/msr.h>
#include <soc/soc_util.h>
#include "chip.h"

static const void *microcode_patch;

static const config_t *chip_config = NULL;

static void xeon_configure_mca(void)
{
	msr_t msr;
	struct cpuid_result cpuid_regs;

	/*
	 * Check feature flag in CPUID.(EAX=1):EDX[7]==1  MCE
	 *                   and CPUID.(EAX=1):EDX[14]==1 MCA
	 */
	cpuid_regs = cpuid(1);
	if ((cpuid_regs.edx & (1 << 7 | 1 << 14)) != (1 << 7 | 1 << 14))
		return;

	msr = rdmsr(IA32_MCG_CAP);
	if (msr.lo & IA32_MCG_CAP_CTL_P_MASK) {
		/* Enable all error logging */
		msr.lo = msr.hi = 0xffffffff;
		wrmsr(IA32_MCG_CTL, msr);
	}

	mca_configure();
}

void get_microcode_info(const void **microcode, int *parallel)
{
	*microcode = intel_mp_current_microcode();
	*parallel = 1;
}

const void *intel_mp_current_microcode(void)
{
	return microcode_patch;
}

static void each_cpu_init(struct device *cpu)
{
	msr_t msr;

	printk(BIOS_SPEW, "%s dev: %s, cpu: %d, apic_id: 0x%x\n",
		__func__, dev_path(cpu), cpu_index(), cpu->path.apic.apic_id);
	setup_lapic();

	/*
	 * Set HWP base feature, EPP reg enumeration, lock thermal and msr
	 * This is package level MSR. Need to check if it updates correctly on
	 * multi-socket platform.
	 */
	msr = rdmsr(MSR_MISC_PWR_MGMT);
	if (!(msr.lo & LOCK_MISC_PWR_MGMT_MSR)) { /* if already locked skip update */
		msr.lo = (HWP_ENUM_ENABLE | HWP_EPP_ENUM_ENABLE | LOCK_MISC_PWR_MGMT_MSR |
			LOCK_THERM_INT);
		wrmsr(MSR_MISC_PWR_MGMT, msr);
	}

	/* Enable Fast Strings */
	msr = rdmsr(IA32_MISC_ENABLE);
	msr.lo |= FAST_STRINGS_ENABLE_BIT;
	wrmsr(IA32_MISC_ENABLE, msr);
	/* Enable Turbo */
	enable_turbo();

	/* Enable speed step. */
	if (get_turbo_state() == TURBO_ENABLED) {
		msr = rdmsr(IA32_MISC_ENABLE);
		msr.lo |= SPEED_STEP_ENABLE_BIT;
		wrmsr(IA32_MISC_ENABLE, msr);
	}

	/* Clear out pending MCEs */
	xeon_configure_mca();
}

static struct device_operations cpu_dev_ops = {
	.init = each_cpu_init,
};

static const struct cpu_device_id cpu_table[] = {
	{X86_VENDOR_INTEL, CPUID_COOPERLAKE_SP_A0},
	{X86_VENDOR_INTEL, CPUID_COOPERLAKE_SP_A1},
	{0, 0},
};

static const struct cpu_driver driver __cpu_driver = {
	.ops = &cpu_dev_ops,
	.id_table = cpu_table,
};

static void set_max_turbo_freq(void)
{
	msr_t msr, perf_ctl;

	FUNC_ENTER();
	perf_ctl.hi = 0;

	/* Check for configurable TDP option */
	if (get_turbo_state() == TURBO_ENABLED) {
		msr = rdmsr(MSR_TURBO_RATIO_LIMIT);
		perf_ctl.lo = (msr.lo & 0xff) << 8;
	} else if (cpu_config_tdp_levels()) {
		/* Set to nominal TDP ratio */
		msr = rdmsr(MSR_CONFIG_TDP_NOMINAL);
		perf_ctl.lo = (msr.lo & 0xff) << 8;
	} else {
		/* Platform Info bits 15:8 give max ratio */
		msr = rdmsr(MSR_PLATFORM_INFO);
		perf_ctl.lo = msr.lo & 0xff00;
	}
	wrmsr(IA32_PERF_CTL, perf_ctl);

	printk(BIOS_DEBUG, "cpu: frequency set to %d\n",
	       ((perf_ctl.lo >> 8) & 0xff) * CONFIG_CPU_BCLK_MHZ);
	FUNC_EXIT();
}

/*
 * Do essential initialization tasks before APs can be fired up
 */
static void pre_mp_init(void)
{
	x86_setup_mtrrs_with_detect();
	x86_mtrr_check();
}

static int get_thread_count(void)
{
	unsigned int num_phys = 0, num_virts = 0;

	cpu_read_topology(&num_phys, &num_virts);
	printk(BIOS_SPEW, "Detected %u cores and %u threads\n", num_phys, num_virts);
	/*
	 * Currently we do not know a way to figure out how many CPUs we have total
	 * on multi-socketed. So we pretend all sockets are populated with CPUs with
	 * same thread/core fusing.
	 * TODO: properly figure out number of active sockets OR refactor MPinit code
	 * to remove requirements of having to know total number of CPUs in advance.
	 */
	return num_virts * CONFIG_MAX_SOCKET;
}

static void post_mp_init(void)
{
	/* Set Max Ratio */
	set_max_turbo_freq();

	/*
	 * TODO: Now that all APs have been relocated as well as the BSP let SMIs
	 * start flowing.
	 */
	if (0) global_smi_enable();
}

static const struct mp_ops mp_ops = {
	.pre_mp_init = pre_mp_init,
	.get_cpu_count = get_thread_count,
	.get_microcode_info = get_microcode_info,
	.post_mp_init = post_mp_init,
};

void cpx_init_cpus(struct device *dev)
{
	microcode_patch = intel_microcode_find();

	if (!microcode_patch)
		printk(BIOS_ERR, "microcode not found in CBFS!\n");

	intel_microcode_load_unlocked(microcode_patch);

	if (mp_init_with_smm(dev->link_list, &mp_ops) < 0)
		printk(BIOS_ERR, "MP initialization failure.\n");

	/*
	 * chip_config is used in cpu device callback. Other than cpu 0,
	 * rest of the CPU devices do not have chip_info updated.
	 */
	chip_config = dev->chip_info;

	/* update numa domain for all cpu devices */
	xeonsp_init_cpu_config();
}

msr_t read_msr_ppin(void)
{
	msr_t ppin = {0};
	msr_t msr;

	/* If MSR_PLATFORM_INFO PPIN_CAP is 0, PPIN capability is not supported */
	msr = rdmsr(MSR_PLATFORM_INFO);
	if ((msr.lo & MSR_PPIN_CAP) == 0) {
		printk(BIOS_ERR, "MSR_PPIN_CAP is 0, PPIN is not supported\n");
		return ppin;
	}

	/* Access to MSR_PPIN is permitted only if MSR_PPIN_CTL LOCK is 0 and ENABLE is 1 */
	msr = rdmsr(MSR_PPIN_CTL);
	if (msr.lo & MSR_PPIN_CTL_LOCK) {
		printk(BIOS_ERR, "MSR_PPIN_CTL_LOCK is 1, PPIN access is not allowed\n");
		return ppin;
	}

	if ((msr.lo & MSR_PPIN_CTL_ENABLE) == 0) {
		/* Set MSR_PPIN_CTL ENABLE to 1 */
		msr.lo |= MSR_PPIN_CTL_ENABLE;
		wrmsr(MSR_PPIN_CTL, msr);
	}
	ppin = rdmsr(MSR_PPIN);
	/* Set enable to 0 after reading MSR_PPIN */
	msr.lo &= ~MSR_PPIN_CTL_ENABLE;
	wrmsr(MSR_PPIN_CTL, msr);
	return ppin;
}
