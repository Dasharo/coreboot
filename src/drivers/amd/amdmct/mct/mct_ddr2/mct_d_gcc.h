/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef MCT_D_GCC_H
#define MCT_D_GCC_H

#include <stdint.h>

void _WRMSR(u32 addr, u32 lo, u32 hi);
void _RDMSR(u32 addr, u32 *lo, u32 *hi);
void _RDTSC(u32 *lo, u32 *hi);
void _cpu_id(u32 addr, u32 *val);
u32 bsr(u32 x);
u32 bsf(u32 x);

#define _MFENCE asm volatile ("mfence")
#define _SFENCE asm volatile ("sfence")

/* prevent speculative execution of following instructions */
#define _EXECFENCE asm volatile ("outb %al, $0xed")

u32 set_upper_fs_base(u32 addr_hi);
void proc_CLFLUSH(u32 addr_hi);
void WriteLNTestPattern(u32 addr_lo, u8 *buf_a, u32 line_num);
u32 read32_fs(u32 addr_lo);
void FlushDQSTestPattern_L9(u32 addr_lo);
__attribute__((noinline)) void FlushDQSTestPattern_L18(u32 addr_lo);
void ReadL18TestPattern(u32 addr_lo);
void ReadL9TestPattern(u32 addr_lo);
void ReadMaxRdLat1CLTestPattern_D(u32 addr);
void WriteMaxRdLat1CLTestPattern_D(u32 buf, u32 addr);
void FlushMaxRdLatTestPattern_D(u32 addr);
u32 stream_to_int(u8 const *p);
u8 oemNodePresent_D(u8 Node, u8 *ret);

#endif
