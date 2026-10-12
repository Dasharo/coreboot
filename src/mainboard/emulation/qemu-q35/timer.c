/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cpu/x86/tsc.h>
#include <pc80/i8254.h>

unsigned long tsc_freq_mhz(void)
{
	static unsigned long tsc_mhz;

	if (tsc_mhz > 0)
		return tsc_mhz;

	/* QEMU reports no TSC frequency, so measure it against the PIT. */
	tsc_mhz = calibrate_tsc_with_pit();

	/* Payloads divide by this rate, so use a high rate if measuring fails. */
	if (tsc_mhz == 0)
		tsc_mhz = 5000;

	return tsc_mhz;
}
