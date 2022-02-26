## SPDX-License-Identifier: GPL-2.0-only

/* Load an immediate 64-bit value into a register */
#define LOAD_IMM64(r, e)			\
	lis     r,(e)@highest;			\
	ori     r,r,(e)@higher;			\
	rldicr  r,r, 32, 31;			\
	oris    r,r, (e)@h;			\
	ori     r,r, (e)@l;

/* Load an immediate 32-bit value into a register */
#define LOAD_IMM32(r, e)			\
	li      r, 0;				\
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

/*
 *
 */
.globl sys_reset_thread_int
sys_reset_thread_int:
	LOAD_IMM32(%r0, jump)
	mtlr %r0
	blr
.globl sys_reset_thread_int_end
sys_reset_thread_int_end:

int_vector_check_size sys_reset_thread_int 0x100

jump:
	;// Raise priority to high.
	or 2,2,2

	/* Load official procedure descriptor address for main() */
	LOAD_IMM64(%r12, main)

	/* Load TOC pointer */
	ld	%r2, 8(%r12)

	/* Stack */
	LOAD_IMM64(%r1, _esecond_stack)

	bl second_thread
	b .
