/* SPDX-License-Identifier: GPL-2.0-or-later */
/* This file is part of the coreboot project. */

#include <cpu/x86/cr.h>
#include <cpu/x86/mtrr.h>

#include <arch/x86/prologue.inc>
#include <cpu/x86/16bit/reset16.inc>

/* For starting coreboot in protected mode */

#include <arch/rom_segs.h>
#include <cpu/x86/post_code.h>
#include <arch/x86/gdt_init.S>
#include <cpu/intel/microcode/microcode_asm.S>

/* Symbol _start16bit must be aligned to 4kB to start AP CPUs with
 * Startup IPI message without RAM.
 */
.align 4096
.code16
.globl _start16bit
.type _start16bit, @function

_start16bit:
	cli

	xorl	%eax, %eax
	movl	%eax, %cr3    /* Invalidate TLB*/

	movw	%cs, %ax
	shlw	$4, %ax
	movw	$nullidt_offset, %bx
	subw	%ax, %bx
	lidt	%cs:(%bx)
	movw	$gdtptr16_offset, %bx
	subw	%ax, %bx
	lgdtl	%cs:(%bx)

	movl	%cr0, %eax
	andl	$0x7FFAFFD1, %eax /* PG,AM,WP,NE,TS,EM,MP = 0 */
	orl	$0x60000001, %eax /* CD, NW, PE = 1 */
	movl	%eax, %cr0

	/* Restore BIST to %eax */
	movl	%ebp, %eax

	/* Now that we are in protected mode jump to a 32 bit code segment. */
	ljmpl	$ROM_CODE_SEG, $__protected_start

	/**
	 * The gdt is defined in entry32.inc, it has a 4 Gb code segment
	 * at 0x08, and a 4 GB data segment at 0x10;
	 */
.align	4
.globl gdtptr16
gdtptr16:
	.word	gdt_end - gdt -1 /* compute the table limit */
	.long	gdt		 /* we know the offset */

.align	4
.globl nullidt
nullidt:
	.word	0	/* limit */
	.long	0
	.word	0

.globl _estart16bit
_estart16bit:
	.code32

	.align	4

	lgdt	%cs:gdtptr
	ljmp	$ROM_CODE_SEG, $__protected_start

__protected_start:
	movw	$ROM_DATA_SEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs

	mov	%cr4, %eax
	or	$CR4_OSFXSR, %ax
	mov	%eax, %cr4

cache_rom:
	/* Disable cache */
	movl	%cr0, %eax
	orl	$CR0_CacheDisable, %eax
	movl	%eax, %cr0

	movl	$MTRR_PHYS_BASE(1), %ecx
	xorl	%edx, %edx
	movl	$(CACHE_ROM_BASE | MTRR_TYPE_WRPROT), %eax
	wrmsr

	movl	$MTRR_PHYS_MASK(1), %ecx
	rdmsr
	movl	$(~(CACHE_ROM_SIZE - 1) | MTRR_PHYS_MASK_VALID), %eax
	wrmsr

	/* Enable cache */
	movl	%cr0, %eax
	andl	$(~(CR0_CacheDisable | CR0_NoWriteThrough)), %eax
	invd
	movl	%eax, %cr0

	/* Enable MTRR. */
	movl	$MTRR_DEF_TYPE_MSR, %ecx
	rdmsr
	orl	$MTRR_DEF_TYPE_EN, %eax
	wrmsr

update_microcode:
	/* put the return address in %esp */
	movl	$end_microcode_update, %esp
	jmp	update_bsp_microcode
end_microcode_update:

	/* Cache needs to be disabled for ACM calls */
	mov	%cr0, %eax
	andl	$(~(CR0_CacheDisable | CR0_NoWriteThrough)), %eax
	movl	%eax, %cr0

	/*
	 * Clean MC[i]_STATUS MSR registers
	 * SCLEAN will generate GPF otherwise
	 */
	mov	$0x179, %ecx
	rdmsr
	movzx	%al, %ebx	/* Bank count to ebx */
	sub	%eax, %eax	/* Write 0 into all MCi_STATUS registers */
	sub	%edx, %edx	
	mov	$0x401, %ecx

mca_error_clean_loop:
	wrmsr
	dec	%ebx
	jz	mca_error_clean_loop_end
	add	$4, %ecx		/* Number of MSRs per bank */
	jmp	mca_error_clean_loop

mca_error_clean_loop_end:

	mov	%cr4, %eax
	or	$(CR4_DE | CR4_SMXE), %eax
	mov	CR4, eax

	mov	$1, %eax
	cpuid
	shr	$24, %ebx	/* initial APIC ID shifted rightmostly */

	/*
	 * Since accesses to semaphore cannot be serialized, accesses among
	 * different CPUs are orchestrated as following:
	 * - BSP will only READ semaphore
	 * - All APs will keep READING semaphore until its value EQUALS to that
	 *   AP's APIC ID minus 1. Only AFTER that AP will INCREMENT semaphore.
	 *   This allows BSP to judge WHEN all APs finished.
	 */
	mov     ecx, [TXT_PUBLIC_BASE + SEMAPHORE]	/* change this to TXT scratchpad */

keep_waiting:
	mov	(%ecx), %eax
	inc	%eax
	cmp	%ebx, %eax
	jb	keep_waiting
	ja	hlt_loop
	mov	%eax, (%ecx)

hlt_loop:
	cli
	hlt
	jmp	hlt_loop
