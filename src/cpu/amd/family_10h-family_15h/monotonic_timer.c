/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <cpu/x86/tsc.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <timer.h>

static struct monotonic_counter {
	int initialized;
	u32 core_frequency;
	struct mono_time time;
	u64 last_value;
} mono_counter;

static void init_tsc_timer(void)
{
	/* Set up TSC (BKDG v3.62 section 2.9.4)*/
	msr_t msr = rdmsr(HWCR_MSR);
	msr.lo |= 0x1000000;
	wrmsr(HWCR_MSR, msr);

	mono_counter.core_frequency = tsc_freq_mhz();
	mono_counter.last_value = rdtscll();
	mono_counter.initialized = 1;
}

void timer_monotonic_get(struct mono_time *mt)
{
	u64 current_tick;
	u32 usecs_elapsed = 0;

	if (!mono_counter.initialized)
		init_tsc_timer();

	current_tick = rdtscll();
	if (mono_counter.core_frequency != 0)
		usecs_elapsed = (current_tick - mono_counter.last_value) / mono_counter.core_frequency;

	/* Update current time and tick values only if a full tick occurred. */
	if (usecs_elapsed) {
		mono_time_add_usecs(&mono_counter.time, usecs_elapsed);
		mono_counter.last_value = current_tick;
	}

	/* Save result. */
	*mt = mono_counter.time;
}
