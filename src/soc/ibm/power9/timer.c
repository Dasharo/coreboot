/* SPDX-License-Identifier: GPL-2.0-only */

#include <delay.h>
#include <timer.h>
#include <timestamp.h>
#include <cpu/power/spr.h>

/* Time base frequency is 512 MHz so 512 ticks per usec */
#define TB_TICKS_PER_USEC 512

#if CONFIG(COLLECT_TIMESTAMPS)
uint64_t timestamp_get(void)
{
	return read_spr(SPR_TB);
}
#endif

int timestamp_tick_freq_mhz(void)
{
	return TB_TICKS_PER_USEC;
}

void init_timer(void)
{
	/*
	 * Set both decrementers to the highest possible value. POWER9 implements
	 * 56 bits, they decrement with 512MHz frequency. Decrementer exception
	 * condition exists when the MSB implemented bit gets (HDEC) or is (DEC)
	 * set, meaning that maximal possible timeout for DEC is one bit less than
	 * that (this gives roughly 7 * 10^7 s = ~2.2 years for DEC and twice that
	 * for HDEC). By default DEC uses only 32 bits, this can be changed by
	 * setting bit 46 (LD) of LPCR (Logical Partitioning Control Register).
	 * Without it the counter overflows and generates an interrupt after ~4.2 s.
	 */

	write_spr(SPR_LPCR, read_spr(SPR_LPCR) | SPR_LPCR_LD);
	write_spr(SPR_DEC, SPR_DEC_LONGEST_TIME);
	write_spr(SPR_HDEC, SPR_DEC_LONGEST_TIME);
}

/* TODO: with HDEC we can get ~2ns resolution, may be useful for RAM init. */
void udelay(unsigned int usec)
{
	uint64_t start = read_spr(SPR_TB);
	uint64_t end = start + usec * TB_TICKS_PER_USEC;

	/*
	 * "When the contents of the DEC0 change from 0 to 1, a Decrementer
	 * exception will come into existence within a reasonable period of time",
	 * but this may not be precise enough. Set an interrupt for 1us less than
	 * requested and busy-loop the rest.
	 *
	 * In tests on Talos 2 this gives between 0 and 1/32 us more than requested,
	 * while interrupt only solution gave between 6/32 and 11/32 us more.
	 */
	if (usec > 1) {
		write_spr(SPR_DEC, (usec - 1) * TB_TICKS_PER_USEC);
		asm volatile("or 31,31,31");	// Lower priority

		do {
			asm volatile("wait");
		} while(read_spr(SPR_DEC) < SPR_DEC_LONGEST_TIME);

		/*
		 * "When the contents of DEC0 change from 1 to 0, the existing
		 * Decrementer exception, if any, will cease to exist within a
		 * reasonable period of time, but not later than the completion of
		 * the next context synchronizing instruction or event" - last part
		 * of sentence doesn't matter, in worst case 'wait' in next udelay()
		 * will be executed more than once but this is still cheaper than
		 * synchronizing context explicitly.
		 */
		write_spr(SPR_DEC, SPR_DEC_LONGEST_TIME);
		asm volatile("or 2,2,2");		// Back to normal priority
	}

	while (end > read_spr(SPR_TB));
}
