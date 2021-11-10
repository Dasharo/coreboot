/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/amd/common/common.h>
#include <cpu/cpu.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/lapic.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/x86/smm.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>

#include "family_10h_15h.h"

static volatile uint8_t fam15h_startup_flags[MAX_NODES_SUPPORTED][MAX_CORES_SUPPORTED] = {{ 0 }};

static void setup_smm(void)
{
	msr_t msr;

	if (CONFIG(HAVE_SMI_HANDLER)) {
		printk(BIOS_DEBUG, "Initializing SMM ASeg memory\n");

		/* Set SMM base address for this CPU */
		msr = rdmsr(SMM_BASE_MSR);
		msr.lo = SMM_BASE - (lapicid() * 0x400);
		wrmsr(SMM_BASE_MSR, msr);

		/* Enable the SMM memory window */
		msr = rdmsr(SMM_MASK_MSR);
		msr.lo |= (1 << 0); /* Enable ASEG SMRAM Range */
		wrmsr(SMM_MASK_MSR, msr);
	} else {
		printk(BIOS_DEBUG, "Disabling SMM ASeg memory\n");

		/* Set SMM base address for this CPU */
		msr = rdmsr(SMM_BASE_MSR);
		msr.lo = SMM_BASE - (lapicid() * 0x400);
		wrmsr(SMM_BASE_MSR, msr);

		/* Disable the SMM memory window */
		msr.hi = 0x0;
		msr.lo = 0x0;
		wrmsr(SMM_MASK_MSR, msr);
	}

	/* Set SMMLOCK to avoid exploits messing with SMM */
	msr = rdmsr(HWCR_MSR);
	msr.lo |= (1 << 0);
	wrmsr(HWCR_MSR, msr);
}

static void clear_mce(void)
{
	msr_t msr;
	u8 i;
	int num_banks;

	/* zero the machine check error status registers */
	msr = rdmsr(IA32_MCG_CAP);
	num_banks = msr.lo & MCA_BANKS_MASK;
	msr.lo = 0;
	msr.hi = 0;
	for (i = 0; i < num_banks; i++)
		wrmsr(IA32_MC0_STATUS + (i * 4), msr);
}

static void misc_configuration(void)
{
	msr_t msr;

	/* Disable Cf8ExtCfg */
	msr = rdmsr(NB_CFG_MSR);
	msr.hi &= ~(1 << (46 - 32));
	wrmsr(NB_CFG_MSR, msr);

	if (is_fam15h()) {
		msr = rdmsr(BU_CFG3_MSR);
		/* Set CombineCr0Cd */
		msr.hi |= (1 << (49-32));
		wrmsr(BU_CFG3_MSR, msr);
	} else {
		msr = rdmsr(BU_CFG2_MSR);
		/* Clear ClLinesToNbDis */
		msr.lo &= ~(1 << 15);
		/* Clear bit 35 as per Erratum 343 */
		msr.hi &= ~(1 << (35-32));
		wrmsr(BU_CFG2_MSR, msr);
	}

}

static void set_bus_unit_configuration(struct node_core_id id)
{
	msr_t msr;

	if (is_fam15h()) {
		uint32_t f5x80;
		uint8_t enabled;
		uint8_t compute_unit_count = 0;
		f5x80 = pci_read_config32(pcidev_on_root(0x18 + id.nodeid, 5), 0x80);
		enabled = f5x80 & 0xf;
		if (enabled == 0x1)
			compute_unit_count = 1;
		if (enabled == 0x3)
			compute_unit_count = 2;
		if (enabled == 0x7)
			compute_unit_count = 3;
		if (enabled == 0xf)
			compute_unit_count = 4;
		msr = rdmsr(BU_CFG2_MSR);
		msr.lo &= ~(0x3 << 6);				/* ThrottleNbInterface[1:0] */
		msr.lo |= (((compute_unit_count - 1) & 0x3) << 6);
		wrmsr(BU_CFG2_MSR, msr);
	} else {
		uint32_t f0x60;
		uint32_t f0x160;
		uint8_t core_count = 0;
		uint8_t node_count = 0;
		f0x60 = pci_read_config32(pcidev_on_root(0x18 + id.nodeid, 0),
									0x60);
		core_count = (f0x60 >> 16) & 0x1f;
		node_count = ((f0x60 >> 4) & 0x7) + 1;
		if (is_gt_rev_d()) {
			f0x160 = pci_read_config32(
				   pcidev_on_root(0x18 + id.nodeid, 0), 0x160);
			core_count |= ((f0x160 >> 16) & 0x7) << 5;
		}
		core_count++;
		core_count /= node_count;
		msr = rdmsr(BU_CFG2_MSR);
		if (is_gt_rev_d()) {
			msr.hi &= ~(0x3 << (36 - 32));			/* ThrottleNbInterface[3:2] */
			msr.hi |= ((((core_count - 1) >> 2) & 0x3) << (36 - 32));
		}
		msr.lo &= ~(0x3 << 6);				/* ThrottleNbInterface[1:0] */
		msr.lo |= (((core_count - 1) & 0x3) << 6);
		msr.lo &= ~(0x1 << 24);				/* WcPlusDis = 0 */
		wrmsr(BU_CFG2_MSR, msr);
	}
}

static void init_siblings(void)
{
	u32 siblings;
	msr_t msr;

	siblings = cpuid_ecx(0x80000008) & 0xff;

	if (siblings > 0) {
		msr = rdmsr_amd(CPU_ID_FEATURES_MSR);
		msr.lo |= 1 << 28;
		wrmsr_amd(CPU_ID_FEATURES_MSR, msr);

		msr = rdmsr_amd(CPU_ID_EXT_FEATURES_MSR);
		msr.hi |= 1 << (33 - 32);
		wrmsr_amd(CPU_ID_EXT_FEATURES_MSR, msr);
	}
	printk(BIOS_DEBUG, "siblings = %02d, ", siblings);
}

static void setup_mtrrs(u8 delay_start, struct node_core_id id)
{
	u8 i;
	msr_t msr;

	if (!delay_start) {
		/* Initialize all variable MTRRs except the first pair.
		 * This prevents Linux from having to correct an inconsistent
		 * MTRR setup, which would crash Family 15h CPUs due to the
		 * compute unit structure sharing MTRR MSRs between AP cores.
		 */
		msr.hi = 0x00000000;
		msr.lo = 0x00000000;

		disable_cache();

		for (i = 0x2; i < 0x10; i++) {
			wrmsr(MTRR_PHYS_BASE(0) | i, msr);
		}

		enable_cache();

		/* Set up other MTRRs */
		amd_setup_mtrrs();
	} else {
		while (!fam15h_startup_flags[id.nodeid][id.coreid - 1]) {
			/* Wait for CU first core startup */
		}
	}
}

static void model_10xxx_init(struct device *dev)
{
	struct node_core_id id;
	u8 delay_start;

	id = get_node_core_id(read_nb_cfg_54());	/* nb_cfg_54 can not be set */
	printk(BIOS_DEBUG, "nodeid = %02d, coreid = %02d\n", id.nodeid, id.coreid);

	if (is_fam15h())
		delay_start = !!(id.coreid & 0x1);
	else
		delay_start = 0;

	/* Turn on caching if we haven't already */
	x86_enable_cache();

	setup_mtrrs(delay_start, id);

	x86_mtrr_check();

	display_mtrrs();

	disable_cache();

	clear_mce();

	enable_cache();

	/* Enable the local CPU APICs */
	setup_lapic();

	/* Set the processor name string */
	init_processor_name();

	if (CONFIG(LOGICAL_CPUS))
		init_siblings();

	set_bus_unit_configuration(id);

	misc_configuration();

	setup_smm();

	fam15h_startup_flags[id.nodeid][id.coreid] = 1;
}

static struct device_operations cpu_dev_ops = {
	.init = model_10xxx_init,
};

static const struct cpu_device_id cpu_table[] = {
//AMD_GH_SUPPORT
	{ X86_VENDOR_AMD, 0x100f00 },		/* SH-F0 L1 */
	{ X86_VENDOR_AMD, 0x100f10 },		/* M2 */
	{ X86_VENDOR_AMD, 0x100f20 },		/* S1g1 */
	{ X86_VENDOR_AMD, 0x100f21 },
	{ X86_VENDOR_AMD, 0x100f2A },
	{ X86_VENDOR_AMD, 0x100f22 },
	{ X86_VENDOR_AMD, 0x100f23 },
	{ X86_VENDOR_AMD, 0x100f40 },		/* RB-C0 */
	{ X86_VENDOR_AMD, 0x100f42 },           /* RB-C2 */
	{ X86_VENDOR_AMD, 0x100f43 },           /* RB-C3 */
	{ X86_VENDOR_AMD, 0x100f52 },           /* BL-C2 */
	{ X86_VENDOR_AMD, 0x100f62 },           /* DA-C2 */
	{ X86_VENDOR_AMD, 0x100f63 },           /* DA-C3 */
	{ X86_VENDOR_AMD, 0x100f80 },           /* HY-D0 */
	{ X86_VENDOR_AMD, 0x100f81 },           /* HY-D1 */
	{ X86_VENDOR_AMD, 0x100f91 },           /* HY-D1 */
	{ X86_VENDOR_AMD, 0x100fa0 },           /* PH-E0 */
	{ X86_VENDOR_AMD, 0x600f12 },           /* OR-B2 */
	{ X86_VENDOR_AMD, 0x600f20 },           /* OR-C0 */
	{ 0, 0 },
};

static const struct cpu_driver model_10xxx __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};
