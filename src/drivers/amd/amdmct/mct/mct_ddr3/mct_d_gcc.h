/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef MCT_D_GCC_H
#define MCT_D_GCC_H

#include <stdint.h>
#include <cpu/x86/cr.h>

void _wrmsr(u32 addr, u32 lo, u32 hi);
void _rdmsr(u32 addr, u32 *lo, u32 *hi);
void _rdtsc(u32 *lo, u32 *hi);
void _cpuid(u32 addr, u32 *val);
u32 bsr(u32 x);
u32 bsf(u32 x);
#define _MFENCE asm volatile ("mfence")
#define _SFENCE asm volatile ("sfence")

/* prevent speculative execution of following instructions */
#define _EXECFENCE asm volatile ("outb %al, $0xed")

u32 set_upper_fs_base(u32 addr_hi);

void proc_mfence(void);
void proc_clflush(u32 addr_hi);
void write_ln_test_pattern(u32 addr_lo, u8 *buf_a, u32 line_num);
u32 read32_fs(u32 addr_lo);
u64 read64_fs(u32 addr_lo);
void flush_dqs_test_pattern_l9(u32 addr_lo);
__attribute__((noinline)) void flush_dqs_test_pattern_l18(u32 addr_lo);
void read_max_rd_lat_1_cl_test_pattern_d(u32 addr);
void write_max_rd_lat_1_cl_test_pattern_d(u32 buf, u32 addr);
void flush_max_rd_lat_test_pattern_d(u32 addr);
u32 stream_to_int(u8 *p);
u8 oem_node_present_d(u8 node, u8 *ret);

#endif
