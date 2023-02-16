/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/x86/mtrr.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/post_code.h>

#define NoEvictMod_MSR 0x2e0
#define BBL_CR_CTL3_MSR 0x11e
#define   BBL_CR_CTL3_MSR_L2_NOT_PRESENT 23
#define   BBL_CR_CTL3_MSR_L2_HW_ENABLED 0

.global bootblock_pre_c_entry

.code32
_cache_as_ram_setup:

bootblock_pre_c_entry:
	movl	$cache_as_ram, %esp /* return address */
	jmp	check_mtrr /* Check if CPU properly reset */

cache_as_ram:
	post_code(0x20)

	/* Send INIT IPI to all excluding ourself. */
	movl	$0x000C4500, %eax
	movl	$0xFEE00300, %esi
	movl	%eax, (%esi)

	/* All CPUs need to be in Wait for SIPI state */
wait_for_sipi:
	movl	(%esi), %eax
	bt	$12, %eax
	jc	wait_for_sipi

	post_code(0x21)
	/* Clean-up MTRR_DEF_TYPE_MSR. */
	movl	$MTRR_DEF_TYPE_MSR, %ecx
	xorl	%eax, %eax
	xorl	%edx, %edx
	wrmsr

	post_code(0x22)
	/* Clear/disable fixed MTRRs */
	mov	$fixed_mtrr_list_size, %ebx
	xor	%eax, %eax
	xor	%edx, %edx

clear_fixed_mtrr:
	add	$-2, %ebx
	movzwl	fixed_mtrr_list(%ebx), %ecx
	wrmsr
	jnz	clear_fixed_mtrr

	/* Zero out all variable range MTRRs. */
	movl	$MTRR_CAP_MSR, %ecx
	rdmsr
	andl	$0xff, %eax
	shl	$1, %eax
	movl	%eax, %edi
	movl	$0x200, %ecx
	xorl	%eax, %eax
	xorl	%edx, %edx
clear_var_mtrrs:
	wrmsr
	add	$1, %ecx
	dec	%edi
	jnz	clear_var_mtrrs

	/* Determine CPU_ADDR_BITS and load PHYSMASK high word to %edx. */
	movl	$0x80000008, %eax
	cpuid
	movb	%al, %cl
	sub	$32, %cl
	movl	$1, %edx
	shl	%cl, %edx
	subl	$1, %edx

	/* Preload high word of address mask (in %edx) for Variable
	 * MTRRs 0 and 1.
	 */
addrsize_set_high:
	xorl	%eax, %eax
	movl	$MTRR_PHYS_MASK(0), %ecx
	wrmsr
	movl	$MTRR_PHYS_MASK(1), %ecx
	wrmsr


	post_code(0x23)
	/* Set Cache-as-RAM base address. */
	movl	$(MTRR_PHYS_BASE(0)), %ecx
	movl	$_car_mtrr_start, %eax
	orl	$MTRR_TYPE_WRBACK, %eax
	xorl	%edx, %edx
	wrmsr

	post_code(0x24)
	/* Set Cache-as-RAM mask. */
	movl	$(MTRR_PHYS_MASK(0)), %ecx
	rdmsr
	movl	$_car_mtrr_mask, %eax
	orl	$MTRR_PHYS_MASK_VALID, %eax
	wrmsr

	/* Enable cache for our code in Flash because we do XIP here */
	movl	$MTRR_PHYS_BASE(1), %ecx
	xorl	%edx, %edx
	movl	$_rom_mtrr_base, %eax
	orl	$MTRR_TYPE_WRPROT, %eax
	wrmsr

	movl	$MTRR_PHYS_MASK(1), %ecx
	rdmsr
	movl	$_rom_mtrr_mask, %eax
	orl	$MTRR_PHYS_MASK_VALID, %eax
	wrmsr

	post_code(0x25)

	/* Enable MTRR. */
	movl	$MTRR_DEF_TYPE_MSR, %ecx
	rdmsr
	orl	$MTRR_DEF_TYPE_EN, %eax
	wrmsr

	movl	$BBL_CR_CTL3_MSR, %ecx
	rdmsr
	bt	$BBL_CR_CTL3_MSR_L2_NOT_PRESENT, %eax
	jc	no_l2_cache
	bt	$BBL_CR_CTL3_MSR_L2_HW_ENABLED, %eax
	jnc	no_l2_cache
	orl	$0x100, %eax
	wrmsr
no_l2_cache:

	/* Enable cache (CR0.CD = 0, CR0.NW = 0). */
	movl	%cr0, %eax
	andl	$(~(CR0_CacheDisable | CR0_NoWriteThrough)), %eax
	invd
	movl	%eax, %cr0

update_microcode:
	/* put the return address in %esp */
	movl	$end_microcode_update, %esp
	jmp	update_bsp_microcode
end_microcode_update:

	/* Disable caching to change MTRR's. */
	movl	%cr0, %eax
	orl	$CR0_CacheDisable, %eax
	movl	%eax, %cr0

	/* Clear the mask valid to disable the MTRR */
	movl	$MTRR_PHYS_MASK(1), %ecx
	rdmsr
	andl	$(~MTRR_PHYS_MASK_VALID), %eax
	wrmsr

	/* Enable cache. */
	movl	%cr0, %eax
	andl	$(~(CR0_CacheDisable | CR0_NoWriteThrough)), %eax
	invd
	movl	%eax, %cr0

	/* enable the 'no eviction' mode */
	movl	$NoEvictMod_MSR, %ecx
	rdmsr
	orl	$1, %eax
	andl	$~2, %eax
	wrmsr

	/* Clear the cache memory region. This will also fill up the cache. */
	cld
	xorl	%eax, %eax
	movl	$_car_mtrr_start, %edi
	movl	$_car_mtrr_size, %ecx
	shr	$2, %ecx
	rep	stosl

	/* enable the 'no eviction run' state */
	movl	$NoEvictMod_MSR, %ecx
	rdmsr
	orl	$3, %eax
	wrmsr

	post_code(0x26)
	/* Enable Cache-as-RAM mode by disabling cache. */
	movl	%cr0, %eax
	orl	$CR0_CacheDisable, %eax
	movl	%eax, %cr0

	movl	$MTRR_PHYS_MASK(1), %ecx
	rdmsr
	orl	$MTRR_PHYS_MASK_VALID, %eax
	wrmsr

	post_code(0x28)
	/* Enable cache. */
	movl	%cr0, %eax
	andl	$(~(CR0_CacheDisable | CR0_NoWriteThrough)), %eax
	movl	%eax, %cr0

	/* Fill in the fake FSP stack data to make FSP MemoryInit happy */
	lea	CAR_init_params, %edx
	movl	$CONFIG_DCACHE_RAM_BASE, %esp
	addl	$CONFIG_DCACHE_RAM_SIZE, %esp

	pushl	$0x18		/* Size of the data region */
	pushl	$0x4455434D	/* Signature of the  data region 'MCUD' */
	pushl	12(%edx)	/* Code size */
	pushl	8(%edx)		/* Code base */
	pushl	4(%edx)		/* Microcode size */
	pushl	(%edx)		/* Microcode base */

	/* Save fake API entry/exit timestamp into stack */
	pushl	$0x18		/* Size of the data region */
	pushl	$0x30524550	/* Signature of the  data region 'PER0' */

	/* First coreboot timestamp as TempRamInit Entry */
	movd	%mm2, %eax
	pushl	%eax		/* tsc[63:32] */
	movd	%mm1, %eax
	pushl	%eax		/* tsc[31:0] */

	/* Second timestamp as TempRamInit Exit freshly read from TSC */
	rdtsc
	pushl	%edx
	pushl	%eax

	pushl	$0		/* Terminator for the data on stack */

	/* Setup the coreboot stack. */
	mov	$_ecar_stack, %esp

	/* Need to align stack to 16 bytes at call instruction. Account for
	the pushes below. */
	andl	$0xfffffff0, %esp
	subl	$4, %esp

	/* push TSC and BIST to stack */
	movd	%mm0, %eax
	pushl	%eax	/* BIST */
	movd	%mm2, %eax
	pushl	%eax	/* tsc[63:32] */
	movd	%mm1, %eax
	pushl	%eax	/* tsc[31:0] */

before_c_entry:
	post_code(0x29)
	call	bootblock_c_entry_bist

	/* Should never see this postcode */
	post_code(POST_DEAD_CODE)


.Lhlt:
	hlt
	jmp	.Lhlt

fixed_mtrr_list:
	.word	MTRR_FIX_64K_00000
	.word	MTRR_FIX_16K_80000
	.word	MTRR_FIX_16K_A0000
	.word	MTRR_FIX_4K_C0000
	.word	MTRR_FIX_4K_C8000
	.word	MTRR_FIX_4K_D0000
	.word	MTRR_FIX_4K_D8000
	.word	MTRR_FIX_4K_E0000
	.word	MTRR_FIX_4K_E8000
	.word	MTRR_FIX_4K_F0000
	.word	MTRR_FIX_4K_F8000
fixed_mtrr_list_size = . - fixed_mtrr_list

CAR_init_params:
	.long	fake_microcode				/* Microcode Location */
	.long	fake_microcode_end - fake_microcode	/* Microcode Length */
	.long	0xFFFFFFFF - CONFIG_ROM_SIZE + 1	/* Firmware Location */
	.long	CONFIG_ROM_SIZE				/* Firmware Length */

	/* coreboot updates microcode itself. FSP still needs a pointer
	   to something that looks like microcode, so provide it with fake
	   microcode. */
fake_microcode:
fake_microcode_header_start:
	.long	1		/* Header Version */
	.long	1		/* Microcode revision */
	.long	0x10232019	/* Date: Time of writing 23-10-2019 */
	.long	0x00010ff0	/* Sig: (non existing) Family: 0xf, Model: 0x1f, stepping: 0 */
	.long	0		/* Checksum: not checked by FSP, so won't care */
	.long	1		/* Loader Revision */
	.long	1		/* Processor Flags */
	.long	fake_microcode_end - fake_microcode_header_end	/* Data Size */
	.long	fake_microcode_end - fake_microcode	/* Total Size */
	.space	12		/* Reserved */
fake_microcode_header_end:
	.space 0x10	/* 16 bytes of empty data */
fake_microcode_end:

_cache_as_ram_setup_end:
