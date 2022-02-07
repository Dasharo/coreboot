## SPDX-License-Identifier: GPL-2.0-only

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
/*
 * System reset vector (0x100)
 *
 * - reload r1 and r2 from saved state
 * - add saved TB value to the current value (self-restore took some time)
 *   - TB can't be written with one mtspr
 *   - have to use 3 writes to deal with possible overflow of lower half
 * - move nia and msr to HSRR0/1
 * - return from hypervisor interrupt
 * - due to clobbers in inline assembly in cpu_winkle all other registers are
 *   reloaded by compiler
 *   - contents of vector and floating point registers are lost
 */
.globl sys_reset_int
sys_reset_int:
	li      %r0, 0
	/* WARNING: this assumes that ramstage is not relocatable */
	LOAD_IMM64(%r3, sstate)
	/* Time Base */
	ld      %r2, 32(%r3)
	mftb    %r4
	add     %r4, %r2, %r4
	rldicl  %r5, %r4, 32, 32
	mttbl   %r0
	mttbu   %r5
	mttbl   %r4
	/* Stack */
	ld      %r1, 0(%r3)
	/* TOC */
	ld      %r2, 8(%r3)
	/* MSR -> HSRR1 */
	ld      %r4, 16(%r3)
	mtspr   315, %r4
	/* NIA -> HSRR0 */
	ld      %r4, 24(%r3)
	mtspr   314, %r4
	/* Link register */
	ld      %r4, 40(%r3)
	mtlr    %r4
	hrfid
.globl sys_reset_int_end
sys_reset_int_end:

int_vector_check_size sys_reset_int 0x100
