/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <arch/cpu.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <cpu/x86/tsc.h>
#include <cpu/amd/common/nums.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <timer.h>

unsigned long tsc_freq_mhz(void)
{
	msr_t msr;
	u8 cpufid;
	u8 cpudid;
	u8 model;
	u32 cpuid_fms;
	u8 boost_capable = 0;

	/* Get CPU model */
	cpuid_fms = cpuid_eax(0x80000001);
	model = ((cpuid_fms & 0xf0000) >> 16) | ((cpuid_fms & 0xf0) >> 4);

	/* Get boost capability */
	if ((model == 0x8) || (model == 0x9)) {	/* revision D */
		boost_capable = (pci_read_config32(NODE_PCI(0, 4), 0x15c) & 0x4) >> 2;
	}

	/* On Family 10h/15h CPUs the TSC increments at the non-boosted P0
	 * clock rate. Read the P0 clock frequency from the P0 MSR and convert
	 * to MHz. See also the Family 15h BKDG Rev. 3.14 page 569.
	 */
	msr = rdmsr(PSTATE_0_MSR + boost_capable);
	cpufid = (msr.lo & 0x3f);
	cpudid = (msr.lo & 0x1c0) >> 6;

	return (100 * (cpufid + 0x10)) / (0x01 << cpudid);
}
