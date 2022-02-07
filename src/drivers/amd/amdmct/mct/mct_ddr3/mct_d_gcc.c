/* SPDX-License-Identifier: GPL-2.0-only */

#include "mct_d_gcc.h"
#include <stdint.h>
#include <arch/cpu.h>

void _wrmsr(u32 addr, u32 lo, u32 hi)
{
	__asm__ volatile (
		"wrmsr"
		:
		:"c"(addr),"a"(lo), "d" (hi)
		);
}

void _rdmsr(u32 addr, u32 *lo, u32 *hi)
{
	__asm__ volatile (
		"rdmsr"
		:"=a"(*lo), "=d" (*hi)
		:"c"(addr)
		);
}

void _rdtsc(u32 *lo, u32 *hi)
{
	__asm__ volatile (
		 "rdtsc"
		 : "=a" (*lo), "=d"(*hi)
		);
}

void _cpuid(u32 addr, u32 *val)
{
	__asm__ volatile(
		 "cpuid"
		 : "=a" (val[0]),
		   "=b" (val[1]),
		   "=c" (val[2]),
		   "=d" (val[3])
		 : "0" (addr));

}

u32 bsr(u32 x)
{
	u8 i;
	u32 ret = 0;

	for (i = 31; i > 0; i--) {
		if (x & (1 << i)) {
			ret = i;
			break;
		}
	}

	return ret;

}

u32 bsf(u32 x)
{
	u8 i;
	u32 ret = 32;

	for (i = 0; i < 32; i++) {
		if (x & (1 << i)) {
			ret = i;
			break;
		}
	}

	return ret;
}

void proc_mfence(void)
{
	__asm__ volatile (
		"outb %%al, $0xed\n\t"  /* _EXECFENCE */
		"mfence\n\t"
		:::"memory"
	);
}

void proc_clflush(u32 addr_hi)
{
	set_upper_fs_base(addr_hi);

	__asm__ volatile (
		/* clflush fs:[eax] */
		"outb	%%al, $0xed\n\t"	/* _EXECFENCE */
		"clflush	%%fs:(%0)\n\t"
		"mfence\n\t"
		::"a" (addr_hi << 8)
	);
}


void write_ln_test_pattern(u32 addr_lo, u8 *buf_a, u32 line_num)
{
	u32 step = 16;
	u32 count = line_num * 4;

	__asm__ volatile (
		/*prevent speculative execution of following instructions*/
		/* FIXME: needed ? */
		"outb %%al, $0xed\n\t"	/* _EXECFENCE */
		"1:\n\t"
		"movdqa (%3), %%xmm0\n\t"
		"movntdq %%xmm0, %%fs:(%0)\n\t"	/* xmm0 is 128 bit */
		"addl %1, %0\n\t"
		"addl %1, %3\n\t"
		"loop 1b\n\t"
		"mfence\n\t"

		 : "+a" (addr_lo), "+d" (step), "+c" (count), "+b" (buf_a) : :
	);

}

u32 read32_fs(u32 addr_lo)
{
	u32 value;
	__asm__ volatile (
		"outb %%al, $0xed\n\t"	/* _EXECFENCE */
		"movl %%fs:(%1), %0\n\t"
		:"=b"(value): "a" (addr_lo)
	);
	return value;
}

u64 read64_fs(u32 addr_lo)
{
	u64 value = 0;
	u32 value_lo;
	u32 value_hi;

	__asm__ volatile (
		"outb %%al, $0xed\n\t"  /* _EXECFENCE */
		"mfence\n\t"
		"movl %%fs:(%2), %0\n\t"
		"movl %%fs:(%3), %1\n\t"
		:"=c"(value_lo), "=d"(value_hi): "a" (addr_lo), "b" (addr_lo + 4) : "memory"
	);
	value |= value_lo;
	value |= ((u64)value_hi) << 32;
	return value;
}

void flush_dqs_test_pattern_l9(u32 addr_lo)
{
	__asm__ volatile (
		"outb %%al, $0xed\n\t"	/* _EXECFENCE */
		"clflush %%fs:-128(%%ecx)\n\t"
		"clflush %%fs:-64(%%ecx)\n\t"
		"clflush %%fs:(%%ecx)\n\t"
		"clflush %%fs:64(%%ecx)\n\t"

		"clflush %%fs:-128(%%eax)\n\t"
		"clflush %%fs:-64(%%eax)\n\t"
		"clflush %%fs:(%%eax)\n\t"
		"clflush %%fs:64(%%eax)\n\t"

		"clflush %%fs:-128(%%ebx)\n\t"

		 ::  "b" (addr_lo + 128 + 8 * 64), "c"(addr_lo + 128),
		     "a"(addr_lo + 128 + 4 * 64)
	);

}

__attribute__((noinline)) void flush_dqs_test_pattern_l18(u32 addr_lo)
{
	__asm__ volatile (
		"outb %%al, $0xed\n\t"	/* _EXECFENCE */
		"clflush %%fs:-128(%%eax)\n\t"
		"clflush %%fs:-64(%%eax)\n\t"
		"clflush %%fs:(%%eax)\n\t"
		"clflush %%fs:64(%%eax)\n\t"

		"clflush %%fs:-128(%%edi)\n\t"
		"clflush %%fs:-64(%%edi)\n\t"
		"clflush %%fs:(%%edi)\n\t"
		"clflush %%fs:64(%%edi)\n\t"

		"clflush %%fs:-128(%%ebx)\n\t"
		"clflush %%fs:-64(%%ebx)\n\t"
		"clflush %%fs:(%%ebx)\n\t"
		"clflush %%fs:64(%%ebx)\n\t"

		"clflush %%fs:-128(%%ecx)\n\t"
		"clflush %%fs:-64(%%ecx)\n\t"
		"clflush %%fs:(%%ecx)\n\t"
		"clflush %%fs:64(%%ecx)\n\t"

		"clflush %%fs:-128(%%edx)\n\t"
		"clflush %%fs:-64(%%edx)\n\t"

		 :: "b" (addr_lo + 128 + 8 * 64), "c" (addr_lo + 128 + 12 * 64),
		    "d" (addr_lo + 128 + 16 * 64), "a"(addr_lo + 128),
		    "D"(addr_lo + 128 + 4 * 64)
	);
}

void read_max_rd_lat_1_cl_test_pattern_d(u32 addr)
{
	set_upper_fs_base(addr);

	__asm__ volatile (
		"outb %%al, $0xed\n\t"			/* _EXECFENCE */
		"movl %%fs:-128(%%esi), %%eax\n\t"	/* TestAddr cache line */
		"movl %%fs:-64(%%esi), %%eax\n\t"	/* +1 */
		"movl %%fs:(%%esi), %%eax\n\t"		/* +2 */
		"mfence\n\t"
		 :: "a"(0), "S"((addr << 8) + 128)
	);

}

void write_max_rd_lat_1_cl_test_pattern_d(u32 buf, u32 addr)
{
	u32 addr_phys = addr << 8;
	u32 step = 16;
	u32 count = 3 * 4;

	set_upper_fs_base(addr);

	__asm__ volatile (
		"outb %%al, $0xed\n\t"	/* _EXECFENCE */
		"1:\n\t"
		"movdqa (%3), %%xmm0\n\t"
		"movntdq %%xmm0, %%fs:(%0)\n\t" /* xmm0 is 128 bit */
		"addl %1, %0\n\t"
		"addl %1, %3\n\t"
		"loop 1b\n\t"
		"mfence\n\t"

		 : "+a" (addr_phys), "+d" (step), "+c" (count), "+b" (buf) : :
	);
}

void flush_max_rd_lat_test_pattern_d(u32 addr)
{
	/*  Flush a pattern of 72 bit times (per DQ) from cache.
	 * This procedure is used to ensure cache miss on the next read training.
	 */

	set_upper_fs_base(addr);

	__asm__ volatile (
		"outb %%al, $0xed\n\t"	/* _EXECFENCE */
		"clflush %%fs:-128(%%esi)\n\t"	 /* TestAddr cache line */
		"clflush %%fs:-64(%%esi)\n\t"	 /* +1 */
		"clflush %%fs:(%%esi)\n\t"  /* +2 */
		"mfence\n\t"

		 :: "S"((addr << 8) + 128)
	);
}

u32 stream_to_int(u8 *p)
{
	int i;
	u32 val;
	u32 valx;

	val = 0;

	for (i = 3; i >= 0; i--) {
		val <<= 8;
		valx = *(p + i);
		val |= valx;
	}

	return val;
}

u8 oem_node_present_d(u8 node, u8 *ret)
{
	*ret = 0;
	return 0;
}
