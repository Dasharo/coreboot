## SPDX-License-Identifier: GPL-2.0-only

#include <cpu/power/spr.h>

/* Load an immediate 64-bit value into a register */
#define LOAD_IMM64(r, e)			\
	lis     r,(e)@highest;			\
	ori     r,r,(e)@higher;			\
	rldicr  r,r, 32, 31;			\
	oris    r,r, (e)@h;			\
	ori     r,r, (e)@l;

/*
 * Macro to check if code fills in the specified size. For proper behavior
 * requires -Wa,--fatal-warnings. Value is saved into ".comment" section to be
 * discarded during linking.
 */
.macro int_vector_check_size vec, max_size
.pushsection ".comment"
.byte 255 - \max_size + (\vec\()_end - \vec)
.popsection
.endm

.section ".text", "ax", %progbits

/* System reset handler for starting a new thread */
.globl sys_reset_thread_int
sys_reset_thread_int:
	LOAD_IMM64(%r0, sys_reset_thread_body)
	mtlr	%r0
	blr
.globl sys_reset_thread_int_end
sys_reset_thread_int_end:

int_vector_check_size sys_reset_thread_int 0x100

sys_reset_thread_body:
	/* Raise priority to high */
	or	2,2,2

	/* Load official procedure descriptor address for main() */
	LOAD_IMM64(%r12, main)

	/* Load TOC pointer */
	ld	%r2, 8(%r12)

	/* Stack */
	LOAD_IMM64(%r1, _esecond_stack)

	bl	reset_hrmor
	// b	.

	mfspr	%r3, SPR_HRMOR
	bl	second_thread
	b	.

.globl reset_hrmor
reset_hrmor:
	/* Save return address */
	mflr	%r8

	li	%r10, 1
	rotldi	%r10, %r10, 63		/* MSB is "ignore HRMOR" */

	LOAD_IMM64(%r7, 1f)
	or	%r9, %r7, %r10

	mtlr	%r9
	blr

	/* This code block ignores HRMOR */
1:
	/* Reset HRMOR to 0 */
	li	%r10, 0
	mfspr	%r3, SPR_HRMOR
	mtspr	SPR_HRMOR, %r10
	isync
	/* We can't just "b 2f", it would use relative offset */
	addi	%r9, %r7, 2f - 1b
	mtlr	%r9
	blr

2:
	/* asdf */
	slbia	7
	sync
	isync
	/* Update stack, TOC and return pointers */
	or	%r1, %r1, %r3
	or	%r2, %r2, %r3
	or	%r8, %r8, %r3
	/* Restore return address and return */
	mtlr	%r8
	blr

.globl set_hrmor
set_hrmor:
	/* Save return address */
	mflr	%r8

	li	%r10, 1
	rotldi	%r10, %r10, 63		/* MSB is "ignore HRMOR" */

	LOAD_IMM64(%r7, 1f)
	or	%r9, %r7, %r10

	mtlr	%r9
	blr

	/* This code block ignores HRMOR */
1:
	mtspr	SPR_HRMOR, %r3
	isync
	/* We can't just "b 2f", it would use relative offset */
	addi	%r9, %r7, 2f - 1b
	mtlr	%r9
	blr

2:
	// slbia 7
	sync
	isync
	/* Update stack, TOC and return pointers */
	andc	%r1, %r1, %r3
	andc	%r2, %r2, %r3
	andc	%r8, %r8, %r3
	/* Restore return address and return */
	mtlr	%r8
	blr
