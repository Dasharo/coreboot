/* SPDX-License-Identifier: GPL-2.0-or-later */

#define __SIMPLE_DEVICE__

#include <cpu/x86/tsc.h>
#include <device/pci_ops.h>
#include <southbridge/intel/common/pmutil.h>

unsigned long tsc_freq_mhz(void)
{
	/* Mimics implementation of acpi_fill_fadt() in southbridge/intel/i82801ix/fadt.c. */
	u16 pmbase = pci_read_config16(PCI_DEV(0, 0x1f, 0), 0x40) & 0xfffe;
	u16 pm_tmr_blk = pmbase + PM1_TMR;
	return inl(pm_tmr_blk);
}
