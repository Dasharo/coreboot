/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cpu/x86/post_code.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/cache.h>
#include <cpu/amd/msr.h>
#include <cpu/amd/mtrr.h>

#define jmp_if_not_k8(x)	comisd	%xmm2, %xmm1; jae x
#define jmp_if_k8(x)		comisd	%xmm2, %xmm1; jb x
#define jmp_if_not_fam15h(x)	comisd	%xmm3, %xmm1; jb x
#define jmp_if_fam15h(x)	comisd	%xmm3, %xmm1; jae x

#define CPUID_MASK		0x0ff00f00
#define CPUID_VAL_FAM10_ROTATED	0x0f000010
#define CPUID_VAL_FAM15_ROTATED	0x0f000060

.section .init, "ax", @progbits
.code32

.global bootblock_pre_c_entry

#include <cpu/amd/car/cache_as_ram_symbols.inc>


bootblock_pre_c_entry:
	/* Save the BIST result. */
	movl	%eax, %ebp

/*
 * XMM map:
 *   xmm1: CPU family
 *   xmm2: Fam10h comparison value
 *   xmm3: Fam15h comparison value
 *   xmm4: Backup EBX
 *   xmm5: coreboot init detect
 *   xmm6: tsc[31:0]
 *   xmm7: tsc[63:32]
 */
cache_as_ram_setup:
	post_code(0xa0)

	/* Save the timestamp */
	movups	%xmm1, %xmm6
	movups	%xmm2, %xmm7

	/* Figure out the CPU family. */
	cvtsi2sd %ebx, %xmm4
	movl	$0x01, %eax
	cpuid
	/* Base family is bits 8..11, extended family is bits 20..27. */
	andl	$CPUID_MASK, %eax
	/* Reorder bits for easier comparison by value. */
	roll	$0x10, %eax
	cvtsi2sd %eax, %xmm1
	movl	$CPUID_VAL_FAM10_ROTATED, %eax
	cvtsi2sd %eax, %xmm2
	movl	$CPUID_VAL_FAM15_ROTATED, %eax
	cvtsi2sd %eax, %xmm3
	cvtsd2si %xmm4, %ebx

	/* Check if cpu_init_detected. */
	movl	$MTRR_DEF_TYPE_MSR, %ecx
	rdmsr
	andl	$MTRR_DEF_TYPE_EN, %eax
	movl	%eax, %ebx	/* We store the status. */
	cvtsi2sd %ebx, %xmm5

	jmp_if_k8(CAR_FAM10_errata_applied)

	/*
	 * For GH, CAR need to set DRAM Base/Limit registers to direct that to node0.
	 * Only BSP needed, for other nodes set during HT/memory init.
	 * So we need to check if it is BSP.
	 */
	movl	$0x1b, %ecx
	rdmsr
	bt	$8, %eax	/* BSP */
	jnc	CAR_FAM10_out

	/* Enable RT tables on BSP. */
	movl	$0x8000c06c, %eax
	movw	$0xcf8, %dx
	outl	%eax, %dx
	addw	$4, %dx
	inl	%dx, %eax
	btr	$0, %eax
	outl	%eax, %dx

	/* Setup temporary DRAM map: [0,16M) bit 0-23. */
	movl	$0x8000c144, %eax
	movw	$0xcf8, %dx
	outl	%eax, %dx
	addw	$4, %dx
	movl	$0, %eax
	outl	%eax, %dx

	movl	$0x8000c140, %eax
	movw	$0xcf8, %dx
	outl	%eax, %dx
	addw	$4, %dx
	movl	$3, %eax
	outl	%eax, %dx

CAR_FAM10_out:

	jmp_if_fam15h(CAR_FAM10_errata_applied)
	/*
	 * Errata 193: Disable clean copybacks to L3 cache to allow cached ROM.
	 * Re-enable it in after RAM is initialized and before CAR is disabled.
	 */
	movl	$BU_CFG2_MSR, %ecx
	rdmsr
	bts	$15, %eax	/* Set bit 15 in EDX:EAX (bit 15 in EAX). */
	wrmsr

	/* Erratum 343, RevGuide for Fam10h, Pub#41322 Rev. 3.33 */
	movl	$BU_CFG2_MSR, %ecx
	rdmsr
	bts	$35-32, %edx	/* Set bit 35 in EDX:EAX (bit 3 in EDX). */
	wrmsr

CAR_FAM10_errata_applied:

	/* Fam15h APIC IDs do not depend on NB config bit 54 */
	jmp_if_not_fam15h(skip_nb54_set)
	movl	$NB_CFG_MSR, %ecx
	rdmsr
	bts	$(54 - 32), %edx	/* Set NB config bit 54 */
	wrmsr

skip_nb54_set:
	/* On Fam15h CPUs each compute unit's MTRRs are shared between two cores */
	jmp_if_not_fam15h(skip_cu_check)

	/* Get the initial APIC ID. */
	movl	$1, %eax
	cpuid
	movl	%ebx, %eax

	/* Restore init detect */
	cvtsd2si %xmm5, %ebx

	/* Determine if this is the second core to start in a compute unit; if so, wait for first core start, clear init detect and skip MTRR init */
	bt	$24, %eax
	jnc	skip_cu_check		/* First core in the compute unit jumps to skip_cu_check */

	/* Determine if this is the second core to start in a compute unit; if so, clear init detect and skip MTRR init */
	/* Busywait until the first core sets up the MTRRs */
check_init_detect_1:
	/* Check if cpu_init_detected. */
	movl	$MTRR_DEF_TYPE_MSR, %ecx
	rdmsr
	andl	$MTRR_DEF_TYPE_EN, %eax
	cmp	$0x00000000, %eax
	je	check_init_detect_1	/* First core has not yet started */

check_init_detect_2:
	movl	$SYSCFG_MSR, %ecx
	rdmsr
	andl	$(SYSCFG_MSR_MtrrFixDramEn | SYSCFG_MSR_MtrrVarDramEn), %eax
	cmp	$0x00000000, %eax
	je	check_init_detect_2	/* First core has not yet started */

	/* First core has now started */
	movl	$0x00000000, %ebx	/* Clear init detect flag */
	cvtsi2sd %ebx, %xmm5
	jmp	fam10_mtrr_setup_complete

skip_cu_check:

	jmp_if_not_fam15h(CAR_FAM15_errata_applied)

	/* Erratum 714, RevGuide for Fam15h, Pub#48063 Rev. 3.24 */
	movl	$BU_CFG2_MSR, %ecx
	rdmsr
	bts	$8, %eax	/* Set bit 8 in EDX:EAX (bit 8 in EAX). */
	wrmsr

CAR_FAM15_errata_applied:

	/* Set MtrrFixDramModEn for clear fixed MTRR. */
enable_fixed_mtrr_dram_modify:
	movl	$SYSCFG_MSR, %ecx
	rdmsr
	andl	$(~(SYSCFG_MSR_MtrrFixDramEn | SYSCFG_MSR_MtrrVarDramEn)), %eax
	orl	$SYSCFG_MSR_MtrrFixDramModEn, %eax
	wrmsr

	/* Clear all MTRRs. */
	xorl	%edx, %edx
	movl	$all_mtrr_msrs, %esi

clear_fixed_var_mtrr:
	lodsl	(%esi), %eax
	testl	%eax, %eax
	jz	clear_fixed_var_mtrr_out

	movl	%eax, %ecx
	xorl	%eax, %eax
	wrmsr

	jmp	clear_fixed_var_mtrr
clear_fixed_var_mtrr_out:

/*
 * 0x06 is the WB IO type for a given 4k segment.
 * 0x1e is the MEM IO type for a given 4k segment (K10 and above).
 * segs is the number of 4k segments in the area of the particular
 *      register we want to use for CAR.
 * reg is the register where the IO type should be stored.
 */
.macro extractmask segs, reg
.if \segs <= 0
	/*
	 * The xorl here is superfluous because at the point of first execution
	 * of this macro, %eax and %edx are cleared. Later invocations of this
	 * macro will have a monotonically increasing segs parameter.
	 */
	xorl	\reg, \reg
.else
	jmp_if_k8(1f)

.if \segs == 1
	movl	$0x1e000000, \reg /* WB MEM type */
.elseif \segs == 2
	movl	$0x1e1e0000, \reg /* WB MEM type */
.elseif \segs == 3
	movl	$0x1e1e1e00, \reg /* WB MEM type */
.elseif \segs >= 4
	movl	$0x1e1e1e1e, \reg /* WB MEM type */
.endif
	jmp 2f
1:
.if \segs == 1
	movl	$0x06000000, \reg /* WB IO type */
.elseif \segs == 2
	movl	$0x06060000, \reg /* WB IO type */
.elseif \segs == 3
	movl	$0x06060600, \reg /* WB IO type */
.elseif \segs >= 4
	movl	$0x06060606, \reg /* WB IO type */
.endif
2:
.endif /* if \segs <= 0 */
.endm

/*
 * carsize is the cache size in bytes we want to use for CAR.
 * windowoffset is the 32k-aligned window into CAR size.
 */
.macro simplemask carsize, windowoffset
	.set gas_bug_workaround,(((\carsize - \windowoffset) >> 12) - 4)
	extractmask gas_bug_workaround, %eax
	.set gas_bug_workaround,(((\carsize - \windowoffset) >> 12))
	extractmask gas_bug_workaround, %edx
	/*
	 * Without the gas bug workaround, the entire macro would consist
	 * only of the two lines below:
	 *   extractmask (((\carsize - \windowoffset) >> 12) - 4), %eax
	 *   extractmask (((\carsize - \windowoffset) >> 12)), %edx
	 */
.endm

#if CONFIG(CPU_AMD_MODEL_10XXX)
  #if CONFIG_DCACHE_RAM_SIZE > 0x80000
  #error Invalid CAR size, must be at most 128k (processor limit is 512k).
  #endif
#else
  #if CONFIG_DCACHE_RAM_SIZE > 0x10000
  #error Invalid CAR size, must be at most 64k.
  #endif
#endif
#if CONFIG_DCACHE_RAM_SIZE < 0x1000
#error Invalid CAR size, must be at least 4k. This is a processor limitation.
#endif
#if (CONFIG_DCACHE_RAM_SIZE & (0x1000 - 1))
#error Invalid CAR size, is not a multiple of 4k. This is a processor limitation.
#endif

#if CONFIG_DCACHE_RAM_SIZE > 0x8000
	/* Enable caching for 32K-64K using fixed MTRR. */
	movl	$MTRR_FIX_4K_C0000, %ecx
	simplemask CONFIG_DCACHE_RAM_SIZE, 0x8000
	wrmsr
#endif

#if CONFIG_DCACHE_RAM_SIZE > 0x10000
	/* Enable caching for 64K-96K using fixed MTRR. */
	movl	$MTRR_FIX_4K_D0000, %ecx
	simplemask CONFIG_DCACHE_RAM_SIZE, 0x10000
	wrmsr
#endif

#if CONFIG_DCACHE_RAM_SIZE > 0x18000
	/* Enable caching for 96K-128K using fixed MTRR. */
	movl	$MTRR_FIX_4K_D8000, %ecx
	simplemask CONFIG_DCACHE_RAM_SIZE, 0x18000
	wrmsr
#endif

	/* Enable caching for 0-32K using fixed MTRR. */
	movl	$MTRR_FIX_4K_C8000, %ecx
	simplemask CONFIG_DCACHE_RAM_SIZE, 0
	wrmsr

	jmp_if_fam15h(fam15_skip_dram_mtrr_setup)

	/* Enable memory access for first MBs using top_mem. */
	movl	$TOP_MEM, %ecx
	xorl	%edx, %edx
	movl	$(((CONFIG_RAMTOP) + TOP_MEM_MASK) & ~TOP_MEM_MASK) , %eax
	wrmsr

fam15_skip_dram_mtrr_setup:

	/* Enable cache for our code in Flash because we do XIP here */
	movl	$MTRR_PHYS_BASE(1), %ecx
	movl	$_program, %eax
	andl	xip_mtrr_mask, %eax
	orl	$MTRR_TYPE_WRPROT, %eax
	wrmsr

	/* Determine CPU_ADDR_BITS and load PHYSMASK high word to %edx. */
	movl	$0x80000008, %eax
	cpuid
	movb	%al, %cl
	sub	$32, %cl
	movl	$1, %edx
	shl	%cl, %edx
	subl	$1, %edx

	movl	$MTRR_PHYS_MASK(1), %ecx
	movl	xip_mtrr_mask, %eax
	orl	$MTRR_PHYS_MASK_VALID, %eax
	wrmsr

	/* Set the default memory type and enable fixed and variable MTRRs. */
	movl	$MTRR_DEF_TYPE_MSR, %ecx
	xorl	%edx, %edx
	movl	$(MTRR_DEF_TYPE_EN | MTRR_DEF_TYPE_FIX_EN), %eax
	wrmsr

	/* Enable the MTRRs and IORRs in SYSCFG. */
	movl	$SYSCFG_MSR, %ecx
	rdmsr
	orl	$(SYSCFG_MSR_MtrrVarDramEn | SYSCFG_MSR_MtrrFixDramEn), %eax
	wrmsr

fam10_mtrr_setup_complete:
	post_code(0xa1)

	/* Disable conversion of INVD to WBINVD (INVDWBINVD = 0) */
	mov	$HWCR_MSR, %ecx
	rdmsr
	btr	$4, %eax
	wrmsr

	jmp_if_not_fam15h(fam15_car_msr_setup_complete)
	/* Disable streaming store (DisSS = 1) */
	mov	$LS_CFG_MSR, %ecx
	rdmsr
	bts	$28, %eax
	wrmsr

	/* Disable speculative ITLB reloads (DisSpecTlbRld = 1) */
	mov	$IC_CFG_MSR, %ecx
	rdmsr
	bts	$9, %eax
	wrmsr

	/* Disable speculative DTLB reloads (DisSpecTlbRld = 1) and set DisHwPf = 1 */
	mov	$DC_CFG_MSR, %ecx
	rdmsr
	bts	$4, %eax
	bts	$13, %eax
	wrmsr

	/* Disable CR0 combining (CombineCr0Cd = 0) */
	mov	$BU_CFG3_MSR, %ecx
	rdmsr
	btr	$49-32, %edx
	wrmsr
fam15_car_msr_setup_complete:

	/* Enable cache. */
	movl	%cr0, %eax
	andl	$(~(CR0_CacheDisable | CR0_NoWriteThrough)), %eax
	movl	%eax, %cr0

	jmp_if_not_k8(CAR_skip_k8_errata_part1)

	/* Set DisFillP on BSP. */
	movl	$0x8000c068, %eax
	movw	$0xcf8, %dx
	outl	%eax, %dx
	addw	$4, %dx
	inl	%dx, %eax
	bts	$10, %eax
	outl	%eax, %dx

CAR_skip_k8_errata_part1:

	jmp_if_k8(fam10_end_part1)

	/* So we need to check if it is BSP. */
	movl	$0x1b, %ecx
	rdmsr
	bt	$8, %eax	/* BSP */
	jnc	CAR_FAM10_ap
fam10_end_part1:

	post_code(0xa2)

	/* Read the range with lodsl. */
	cld
	movl	$CONFIG_DCACHE_RAM_BASE, %esi
	movl	$(CONFIG_DCACHE_RAM_SIZE >> 2), %ecx
	rep	lodsl

	/* Clear the range. */
	movl	$CONFIG_DCACHE_RAM_BASE, %edi
	movl	$(CONFIG_DCACHE_RAM_SIZE >> 2), %ecx
	xorl	%eax, %eax
	rep	stosl

	jmp_if_not_k8(CAR_skip_k8_errata_part2)

	/* Clear DisFillP on BSP. */
	movl	$0x8000c068, %eax
	movw	$0xcf8, %dx
	outl	%eax, %dx
	addw	$4, %dx
	inl	%dx, %eax
	btr	$10, %eax
	outl	%eax, %dx

CAR_skip_k8_errata_part2:

	/* Set up the stack pointer. */
	movl	$(CONFIG_DCACHE_RAM_BASE + CONFIG_DCACHE_RAM_SIZE), %eax
	movl	%eax, %esp

	/* Poison the lower stack boundary */
	movl	$((CONFIG_DCACHE_RAM_BASE + CONFIG_DCACHE_RAM_SIZE) - CONFIG_DCACHE_BSP_STACK_SIZE), %eax
	movl	$0xdeadbeef, (%eax)

	post_code(0xa3)

	jmp	CAR_FAM10_ap_out
CAR_FAM10_ap:
	/*
	 * Need to set stack pointer for AP.
	 * It will be from:
	 *   CONFIG_DCACHE_RAM_BASE + (CONFIG_DCACHE_RAM_SIZE - (CONFIG_DCACHE_BSP_STACK_SIZE + CONFIG_DCACHE_BSP_TOP_STACK_SLUSH))
	 *   - (NodeID << CoreIDbits + CoreID) * CONFIG_DCACHE_AP_STACK_SIZE
	 * The spacing between the BSP stack and the top of the AP
	 * stacks is purposefully set larger (an extra CONFIG_DCACHE_BSP_TOP_STACK_SLUSH
	 * worth of unused space) than necessary to aid debugging when
	 * additional stack variables are added by future developers.
	 * The extra space will allow BSP overruns to be caught by
	 * the warning logic and easily fixed instead of crashing the
	 * system with no obvious clues of what went wrong.
	 *
	 * So, need to get the NodeID and CoreID at first.
	 * If NB_CFG_MSR bit 54 is set just use initial APIC ID, otherwise need
	 * to reverse it.
	 */

	/* Get the coreid bits at first. */
	movl	$0x80000008, %eax
	cpuid
	shrl	$12, %ecx
	andl	$0x0f, %ecx
	movl	%ecx, %edi

	/* Get the initial APIC ID. */
	movl	$1, %eax
	cpuid
	shrl	$24, %ebx

	/* Get the nb cfg bit 54. */
	movl	$NB_CFG_MSR, %ecx
	rdmsr
	movl	%edi, %ecx		/* CoreID bits */
	bt	$(54 - 32), %edx
	jc	roll_cfg

	/* Fam10h NB config bit 54 was not set */
	rolb	%cl, %bl
roll_cfg:
	jmp_if_not_fam15h(ap_apicid_ready)
	cmp	$0x5, %ecx
	jne	ap_apicid_ready

	/* This is a multi-node CPU
	 * Adjust the maximum APIC ID to a more reasonable value
	 * given that no 32-core Family 15h processors exist
	 */
	movl	%ebx, %ecx
	and	$0x0f, %ecx		/* Get lower 4 bits of CPU number */
	and	$0x60, %ebx		/* Get node ID */
	shrl	$0x1, %ebx		/* Shift node ID part of APIC ID down by 1 */
	or	%ecx, %ebx		/* Recombine node ID and CPU number */

ap_apicid_ready:

	/* Calculate stack pointer using adjusted APIC ID stored in ebx */
	movl	$CONFIG_DCACHE_AP_STACK_SIZE, %eax
	mull	%ebx
	movl	$(CONFIG_DCACHE_RAM_BASE + (CONFIG_DCACHE_RAM_SIZE - (CONFIG_DCACHE_BSP_STACK_SIZE + CONFIG_DCACHE_BSP_TOP_STACK_SLUSH))), %esp
	subl	%eax, %esp

	/* Restore init detect */
	cvtsd2si %xmm5, %ebx

	post_code(0xa4)

CAR_FAM10_ap_out:

	post_code(0xa5)

	/* Need to align stack to 16 bytes at call instruction. Account for
	the pushes below. */
	andl	$0xfffffff0, %esp
	subl	$4, %esp

	/* Restore the BIST result. */
	movl	%ebp, %eax

	post_code(0xa6)

	/* push TSC and BIST to stack */
	pushl	%eax	/* BIST */
	cvtsd2si %xmm7, %eax
	pushl	%eax	/* tsc[63:32] */
	cvtsd2si %xmm6, %eax
	pushl	%eax	/* tsc[31:0] */

	post_code(0xa7)

	/* Disable SSE. */
	movl	%cr4, %eax
	andl	$~(3 << 9), %eax
	movl	%eax, %cr4

before_c_entry:
	post_code(0x2f)
	call	bootblock_c_entry_bist

	/* Should never see this postcode */
	post_code(POST_DEAD_CODE)

.Lhlt:
	hlt
	jmp	.Lhlt

all_mtrr_msrs:
	/* fixed MTRR MSRs */
	.long	MTRR_FIX_64K_00000
	.long	MTRR_FIX_16K_80000
	.long	MTRR_FIX_16K_A0000
	.long	MTRR_FIX_4K_C0000
	.long	MTRR_FIX_4K_C8000
	.long	MTRR_FIX_4K_D0000
	.long	MTRR_FIX_4K_D8000
	.long	MTRR_FIX_4K_E0000
	.long	MTRR_FIX_4K_E8000
	.long	MTRR_FIX_4K_F0000
	.long	MTRR_FIX_4K_F8000

	/* var MTRR MSRs */
	.long	MTRR_PHYS_BASE(0)
	.long	MTRR_PHYS_MASK(0)
	.long	MTRR_PHYS_BASE(1)
	.long	MTRR_PHYS_MASK(1)
	.long	MTRR_PHYS_BASE(2)
	.long	MTRR_PHYS_MASK(2)
	.long	MTRR_PHYS_BASE(3)
	.long	MTRR_PHYS_MASK(3)
	.long	MTRR_PHYS_BASE(4)
	.long	MTRR_PHYS_MASK(4)
	.long	MTRR_PHYS_BASE(5)
	.long	MTRR_PHYS_MASK(5)
	.long	MTRR_PHYS_BASE(6)
	.long	MTRR_PHYS_MASK(6)
	.long	MTRR_PHYS_BASE(7)
	.long	MTRR_PHYS_MASK(7)

	/* Variable IORR MTRR MSRs */
	.long	IORRBase_MSR(0)
	.long	IORRMask_MSR(0)
	.long	IORRBase_MSR(1)
	.long	IORRMask_MSR(1)

	/* Top of memory MTRR MSRs */
	.long	TOP_MEM
	.long	TOP_MEM2

	.long	0x000 /* NULL, end of table */

cache_as_ram_setup_out:
