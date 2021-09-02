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

/*
 * External interrupt vector (0x500)
 *
 * Only one LSI source is enabled - PSU. It has vector 0xD, which is written
 * to bits 48:51 in the address below.
 * - mask interrupt - read 0xD00
 * - read state (needed?) - read 0x800
 * - send EOI to XIVE IC (read returns 1 if interrupt pending) read 0x0006030203103000
 * - unmask interrupt
 *
 * No need for eieio:
 * - "If two Store instructions or two Load instructions specify storage
 *    locations that are both Caching Inhibited and Guarded, the corresponding
 *    storage accesses are performed in program order with respect to any
 *    processor or mechanism." - POWER ISA 3.0B, Book II, 1.7.1 - Storage Access
 *   Ordering, we have only loads here, and
 * - "The storage accesses caused by the instructions described in this section
 *    are performed as though the specified storage location is Caching
 *    Inhibited and Guarded." - Book III, 4.4.1 - Fixed-Point Load and Store
 *   Caching Inhibited Instructions
 * - both interrupts and '(h)rfid' instructions are context synchronizing.
 */
.globl ext_int
ext_int:
	mtsprg0 %r3
	mtsprg1 %r4
	LOAD_IMM64(%r3, 0x00060302031CDD00)
	/* Mask interrupt */
	ldcix   %r4, 0, %r3
	subi    %r3, %r3, 0x500
	/* Read state */
	ldcix   %r4, 0, %r3
	LOAD_IMM64(%r4, 0x0006030203103000)
	/* Send EOI */
	ldcix   %r4, 0, %r4
	addi    %r3, %r3, 0x400
	/* Unmask interrupt */
	ldcix   %r4, 0, %r3
	mfsprg0 %r3
	mfsprg1 %r4
	hrfid
.globl ext_int_end
ext_int_end:

int_vector_check_size ext_int 0x100

/*
 * Hypervisor Virtualization vector (0xEA0)
 *
 * Taken when cores are waken up by deadman loop. The occurrence of the
 * interrupt does not cause the exception to cease to exist, it must be
 * acknowledged by read16(0x0006020000001830). After that, External Exception
 * is still asserted and must also be handled.
 *
 * There are only 0x20 bytes reserved for this handler, which gives just 8
 * instructions. LOAD_IMM64 would use 5 of those, so don't use it. Bits in
 * address are set in a way that makes it possible to use two load instructions
 * and one rotate/shift operation.
 */
.globl hyp_virt_int
hyp_virt_int:
	mtsprg0 %r3
	li      %r3, 0x0602
	rldicr  %r3, %r3, 40, 23
	ori     %r3, %r3, 0x1830
	lhzcix  %r3, 0, %r3
	/* Jump to External Interrupt handler, skipping 'mtsprg0 %r3' */
	ba      0x504
.globl hyp_virt_int_end
hyp_virt_int_end:

int_vector_check_size hyp_virt_int 0x20
