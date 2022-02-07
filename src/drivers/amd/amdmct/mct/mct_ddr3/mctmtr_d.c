/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <console/console.h>
#include <cpu/amd/mtrr.h>
#include <cpu/x86/mtrr.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static void set_mtrr_range_wb_d(u32 base, u32 *p_limit, u32 *p_mtrr_addr);
static void set_mtrr_range_d(u32 base, u32 *p_limit, u32 *p_mtrr_addr, u16 mtrr_type);

void cpu_mem_typing_d(struct MCTStatStruc *p_mct_stat,
			 struct DCTStatStruc *p_dct_stat_a)
{
	/* BSP only.  Set the fixed MTRRs for common legacy ranges.
	 * Set TOP_MEM and TOM2.
	 * Set some variable MTRRs with WB Uncacheable type.
	 */

	u32 bottom_32b_io, bottom_40b_io, cache_32b_top;
	u32 val;
	u32 addr;
	u32 lo, hi;

	/* Set temporary top of memory from Node structure data.
	 * Adjust temp top of memory down to accommodate 32-bit IO space.
	 * bottom_40b_io = top of memory, right justified 8 bits
	 *	(defines dram versus IO space type)
	 * bottom_32b_io = sub 4GB top of memory, right justified 8 bits
	 *	(defines dram versus IO space type)
	 * cache_32b_top = sub 4GB top of WB cacheable memory,
	 *	right justified 8 bits
	 */

	val = mct_get_nv_bits(NV_BOTTOM_IO);
	if (val == 0)
		val++;

	bottom_32b_io = val << (24-8);

	val = p_mct_stat->sys_limit + 1;
	if (val <= _4GB_RJ8) {
		bottom_40b_io = 0;
		if (bottom_32b_io >= val)
			bottom_32b_io = val;
	} else {
		bottom_40b_io = val;
	}

	cache_32b_top = bottom_32b_io;

	/*======================================================================
	 Set default values for CPU registers
	======================================================================*/

	/* NOTE : For coreboot, we don't need to set mtrr enables here because
	they are still enable from cache_as_ram.inc */

	addr = MTRR_FIX_64K_00000;
	lo = 0x1E1E1E1E;
	hi = lo;
	_wrmsr(addr, lo, hi);		/* 0 - 512K = WB Mem */
	addr = MTRR_FIX_16K_80000;
	_wrmsr(addr, lo, hi);		/* 512K - 640K = WB Mem */

	/*======================================================================
	  Set variable MTRR values
	 ======================================================================*/
	/* NOTE: for coreboot change from 0x200 to 0x204: coreboot is using
		0x200, 0x201 for [1M, CONFIG_TOP_MEM)
		0x202, 0x203 for ROM Caching
		 */
	addr = MTRR_PHYS_BASE(2);	/* MTRR phys base 2*/
			/* use TOP_MEM as limit*/
			/* Limit = TOP_MEM|TOM2*/
			/* base = 0*/
	printk(BIOS_DEBUG, "\t CPUMemTyping: cache_32b_top:%x\n", cache_32b_top);
	set_mtrr_range_wb_d(0, &cache_32b_top, &addr);
				/* base */
				/* Limit */
				/* MtrrAddr */
	if (addr == -1)		/* ran out of MTRRs?*/
		p_mct_stat->g_status |= 1 << GSB_MTRR_SHORT;

	p_mct_stat->sub_4G_cache_top = cache_32b_top << 8;

	/*======================================================================
	 Set TOP_MEM and TOM2 CPU registers
	======================================================================*/
	addr = TOP_MEM;
	lo = bottom_32b_io << 8;
	hi = bottom_32b_io >> 24;
	_wrmsr(addr, lo, hi);
	printk(BIOS_DEBUG, "\t CPUMemTyping: bottom_32b_io:%x\n", bottom_32b_io);
	printk(BIOS_DEBUG, "\t CPUMemTyping: bottom_40b_io:%x\n", bottom_40b_io);
	if (bottom_40b_io) {
		hi = bottom_40b_io >> 24;
		lo = bottom_40b_io << 8;
		addr += 3;		/* TOM2 */
		_wrmsr(addr, lo, hi);
	}
	addr = SYSCFG_MSR;		/* SYS_CFG */
	_rdmsr(addr, &lo, &hi);
	if (bottom_40b_io) {
		lo |= SYSCFG_MSR_TOM2En;	/* MtrrTom2En = 1 */
		lo |= SYSCFG_MSR_TOM2WB;	/* Tom2ForceMemTypeWB */
	} else {
		lo &= ~SYSCFG_MSR_TOM2En;	/* MtrrTom2En = 0 */
		lo &= ~SYSCFG_MSR_TOM2WB;	/* Tom2ForceMemTypeWB */
	}
	_wrmsr(addr, lo, hi);
}

static void set_mtrr_range_wb_d(u32 base, u32 *p_limit, u32 *p_mtrr_addr)
{
	/*set WB type*/
	set_mtrr_range_d(base, p_limit, p_mtrr_addr, 6);
}

static void set_mtrr_range_d(u32 base, u32 *p_limit, u32 *p_mtrr_addr, u16 mtrr_type)
{
	/* Program MTRRs to describe given range as given cache type.
	 * Use MTRR pairs starting with the given MTRRphys base address,
	 * and use as many as is required up to (excluding) MSR 020C, which
	 * is reserved for OS.
	 *
	 * "Limit" in the context of this procedure is not the numerically
	 * correct limit, but rather the Last address+1, for purposes of coding
	 * efficiency and readability.  Size of a region is then Limit-base.
	 *
	 * 1. Size of each range must be a power of two
	 * 2. Each range must be naturally aligned (base is same as size)
	 *
	 * There are two code paths: the ascending path and descending path
	 * (analogous to bsf and bsr), where the next limit is a function of the
	 * next set bit in a forward or backward sequence of bits (as a function
	 * of the Limit). We start with the ascending path, to ensure that
	 * regions are naturally aligned, then we switch to the descending path
	 * to maximize MTRR usage efficiency. base = 0 is a special case where we
	 * start with the descending path. Correct Mask for region is
	 * 2comp(Size-1)-1, which is 2comp(Limit-base-1)-1
	 */

	u32 cur_base, cur_limit, cur_size;
	u32 val, valx;
	u32 addr;

	val = cur_base = base;
	cur_limit = *p_limit;
	addr = *p_mtrr_addr;
	while ((addr >= 0x200) && (addr < 0x20C) && (val < *p_limit)) {
		/* start with "ascending" code path */
		/* alignment (largest block size)*/
		valx = 1 << bsf(cur_base);
		cur_size = valx;

		/* largest legal limit, given current non-zero range base*/
		valx += cur_base;
		if ((cur_base == 0) || (*p_limit < valx)) {
			/* flop direction to "descending" code path*/
			valx = 1 << bsr(*p_limit - cur_base);
			cur_size = valx;
			valx += cur_base;
		}
		cur_limit = valx;		/*eax = cur_base, edx = cur_limit*/
		valx = val >> 24;
		val <<= 8;

		/* now program the MTRR */
		val |= mtrr_type;		/* set cache type (UC or WB)*/
		_wrmsr(addr, val, valx);	/* prog. MTRR with current region base*/
		val = ((~(cur_size - 1))+1) - 1;	/* Size-1*/ /*Mask = 2comp(Size-1)-1*/
		valx = (val >> 24) | (0xff00);	/* GH have 48 bits addr */
		val <<= 8;
		val |= (1 << 11);			/* set MTRR valid*/
		addr++;
		_wrmsr(addr, val, valx);	/* prog. MTRR with current region Mask*/
		val = cur_limit;
		cur_base = val;			/* next base = current Limit (loop exit)*/
		addr++;				/* next MTRR pair addr */
	}
	if (val < *p_limit) {
		*p_limit = val;
		addr = -1;
	}
	*p_mtrr_addr = addr;
}

void uma_mem_typing_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a)
{
	/* UMA memory size may need splitting the MTRR configuration into two
	 * Before training use NB_BottomIO or the physical memory size to set the MTRRs.
	 * After training, add UMAMemTyping function to reconfigure the MTRRs based on
	 * NV_BOTTOM_UMA (for UMA systems only).
	 * This two-step process allows all memory to be cached for training
	 */

	u32 bottom_32b_io, cache_32b_top;
	u32 val;
	u32 addr;
	u32 lo, hi;

	/*======================================================================
	 * Adjust temp top of memory down to accommodate UMA memory start
	 *======================================================================*/
	/* bottom_32b_io = sub 4GB top of memory, right justified 8 bits
	 * (defines dram versus IO space type)
	 * cache_32b_top = sub 4GB top of WB cacheable memory, right justified 8 bits */

	bottom_32b_io = p_mct_stat->sub_4G_cache_top >> 8;

	val = mct_get_nv_bits(NV_BOTTOM_UMA);
	if (val == 0)
		val++;

	val <<= (24-8);
	if (val < bottom_32b_io) {
		cache_32b_top = val;
		p_mct_stat->sub_4G_cache_top = val;

		/*======================================================================
		 * Clear variable MTRR values
		 *======================================================================*/
		addr = MTRR_PHYS_BASE(0);
		lo = 0;
		hi = lo;
		while (addr < MTRR_PHYS_BASE(6)) {
			_wrmsr(addr, lo, hi);	/* prog. MTRR with current region Mask */
			addr++;			/* next MTRR pair addr */
		}

		/*======================================================================
		 * Set variable MTRR values
		 *======================================================================*/
		printk(BIOS_DEBUG, "\t uma_mem_typing_d: cache_32b_top:%x\n", cache_32b_top);
		set_mtrr_range_wb_d(0, &cache_32b_top, &addr);
		if (addr == -1)		/* ran out of MTRRs?*/
			p_mct_stat->g_status |= 1 << GSB_MTRR_SHORT;
	}
}
