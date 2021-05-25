/* SPDX-License-Identifier: GPL-2.0-only */

#include <delay.h>
#include <timestamp.h>
#include <string.h>		// memcpy
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

/*
 * If udelay() were to be used in bootblock, a padding for interrupt vector must
 * be added to memlayout.ld or bootblock itself.
 *
 * In ramstage, take care to not overwrite payload with handlers or vice versa.
 */
#if ENV_ROMSTAGE

volatile uint64_t hdec_done;

/*
 * Interrupt handler - no stack and only one register (normally used by OS) can
 * be repurposed. This is enough, for now...
 *
 * "When a Hypervisor Decrementer interrupt occurs, the existing Hypervisor
 * Decrementer exception will cease to exist within a reasonable period of time,
 * but not later than the completion of the next context synchronizing
 * instruction or event."
 *
 * 'hrfid' is context synchronizing. Note that the above does not apply to
 * non-Hypervisor Decrementer - the interrupt does not result in exception
 * ceasing to exist:
 *
 * "When the contents of DEC[0] change from 1 to 0, the existing Decrementer
 * exception, if any, will cease to exist within a reasonable period of
 * time, but not later than the completion of the next context synchronizing
 * instruction or event."
 */
static uint32_t hdec_handler_template[] = {
	0xf9ad0000,		/* std     r13,0(r13) */
	0x4c000224		/* hrfid              */
};

void init_timer(void)
{
	uint64_t tmp;

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

	tmp = read_spr(SPR_LPCR);	/* LPCR */
	/*
	 * 46 - LD - Large Decrementer (does not apply to HDEC)
	 * 59 - HEIC - Hypervisor External Interrupt Control
	 * 63 - HDICE - Hypervisor Decrementer Interrupt Conditionally Enable
	 */
	write_spr(SPR_LPCR, tmp | SPR_LPCR_LD | SPR_LPCR_HEIC | SPR_LPCR_HDICE);

	write_spr(SPR_DEC, SPR_DEC_LONGEST_TIME);
	write_spr(SPR_HDEC, SPR_DEC_LONGEST_TIME);

	/* r13 is reserved for thread ID, we don't have threads so borrow it */
	asm volatile("mr 13, %0" :: "r"(&hdec_done));

	memcpy((void *)0x980, hdec_handler_template, sizeof(hdec_handler_template));

	/*
	 * Other interrupts are enabled when MSR[48] = 1, make them halt.
	 *
	 * `b .` =   0x48000000
	 * `rfid` =  0x4c000024
	 * `hrfid` = 0x4c000224
	 *
	 * TODO: these interrupts shouldn't happen, we don't know how to handle
	 * them, maybe add a (destructive) handler that calls die()?
	 */
	*(uint32_t *)0x500 = 0x48000000;	// External interrupt
	*(uint32_t *)0xF00 = 0x48000000;	// Performance monitor
	*(uint32_t *)0xA00 = 0x48000000;	// Privileged  Doorbell
	*(uint32_t *)0xE60 = 0x48000000;	// Hypervisor Maintenance
	*(uint32_t *)0xE80 = 0x48000000;	// Hypervisor Doorbell

	*(uint32_t *)0x900 = 0x48000000;	// Decrementer

	tmp = read_msr();
	write_msr(tmp | 0x8000);	/* EE - External Interrupt Enable */
}

/* TODO: with HDEC we can get ~2ns resolution, may be useful for RAM init. */
void udelay(unsigned int usec)
{
	uint64_t start = read_spr(SPR_TB);
	uint64_t end = start + usec * TB_TICKS_PER_USEC;

	hdec_done = 0;

	/*
	 * HDEC interrupt is generated "within a reasonable period of time", but
	 * this may not be precise enough. Set an interrupt for 1us less than
	 * requested and busy-loop the rest.
	 *
	 * In tests on Talos 2 this gives between 0 and 1/32 us more than requested,
	 * while interrupt only solution gave between 6/32 and 11/32 us more.
	 */
	if (usec > 1) {
		write_spr(SPR_HDEC, (usec - 1) * TB_TICKS_PER_USEC);
		asm volatile("or 31,31,31");	// Lower priority
		while(!hdec_done);
		asm volatile("or 2,2,2");		// Back to normal priority
	}

	while (end > read_spr(SPR_TB));
}
#endif
