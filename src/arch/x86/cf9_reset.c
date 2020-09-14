/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <arch/cache.h>
#include <cf9_reset.h>
#include <console/console.h>
#include <halt.h>

/*
 * A system reset in terms of the CF9 register asserts the INIT#
 * signal to reset the CPU along the PLTRST# signal to reset other
 * board components. It is usually the hardest reset type that
 * does not power cycle the board. Thus, it could be called a
 * "warm reset".
 */
void do_system_reset(void)
{
	dcache_clean_all();
	outb(SYS_RST, RST_CNT);
	outb(RST_CPU | SYS_RST, RST_CNT);
}

/*
 * A full reset in terms of the CF9 register triggers a power cycle
 * (i.e. S0 -> S5 -> S0 transition). Thus, it could be called a
 * "cold reset".
 * Note: Not all x86 implementations comply with this definition,
 *       some may require additional configuration to power cycle.
 */
void do_full_reset(void)
{
	dcache_clean_all();
	outb(FULL_RST | SYS_RST, RST_CNT);
	outb(FULL_RST | RST_CPU | SYS_RST, RST_CNT);
}

void system_reset(void)
{
	printk(BIOS_INFO, "%s() called!\n", __func__);
	cf9_reset_prepare();
	do_system_reset();
	halt();
}

void full_reset(void)
{
	printk(BIOS_INFO, "%s() called!\n", __func__);
	cf9_reset_prepare();
	do_full_reset();
	halt();
}

void global_system_reset(void)
{
	printk(BIOS_INFO, "%s() called!\n", __func__);
	cf9_reset_prepare();
	cf9_global_reset_prepare();
	do_system_reset();
	halt();
}

void global_full_reset(void)
{
	printk(BIOS_INFO, "%s() called!\n", __func__);
	cf9_reset_prepare();
	cf9_global_reset_prepare();
	do_full_reset();
	halt();
}
