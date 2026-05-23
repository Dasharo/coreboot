/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cpu/x86/tsc.h>

unsigned long tsc_freq_mhz(void)
{
	/* ACPI's Power Management Timer frequency is fixed at 3.579545 MHz. */
	return 3579545 / 1000000;
}
