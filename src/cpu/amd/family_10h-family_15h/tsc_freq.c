/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>
#include <arch/cpu.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <cpu/x86/tsc.h>
#include <device/pci_ops.h>

static uint8_t tsc_initted = 0;

static void init_tsc(void)
{
	/* Set up TSC (BKDG v3.62 section 2.9.4)*/
	msr_t msr = rdmsr(HWCR_MSR);
	msr.lo |= 0x1000000;
	wrmsr(HWCR_MSR, msr);
}

static get_boost_states(void)
{
#if ENV_PCI_SIMPLE_DEVICE
	return (pci_read_config32(PCI_DEV(0, 0x18, 4), 0x15c) >> 2) & 7;
#else
	return (pci_read_config32(pcidev_on_root(0x18, 4), 0x15c) >> 2) & 7;
#endif

}

unsigned long tsc_freq_mhz(void)
{
	msr_t msr;
	uint8_t cpufid;
	uint8_t cpudid;
	uint8_t boost_states = get_boost_states();

	if (!tsc_initted)
		init_tsc();

	/* On Family 10h/15h CPUs the TSC increments
	 * at the P0 clock rate.  Read the P0 clock
	 * frequency from the P0 MSR and convert
	 * to MHz.  See also the Family 15h BKDG
	 * Rev. 3.14 page 569.
	 */
	msr = rdmsr(PSTATE_0_MSR + boost_states);
	cpufid = (msr.lo & 0x3f);
	cpudid = (msr.lo & 0x1c0) >> 6;

	return (100 * (cpufid + 0x10)) / (0x01 << cpudid);
}
