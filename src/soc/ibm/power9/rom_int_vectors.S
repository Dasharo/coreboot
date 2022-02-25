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
	;// Raise priority to high.
	or 2,2,2

	;// Need to send a msgysnc to prevent weak consistency issues
	;// with doorbells (they execute this path prior to dbell intr)
	.long 0x7C0006EC

	// ;// Clear MSR[TA] (bit 1)
	// mfmsr %r2
	// rldicl %r2,%r2,1,1    ;// Clear bit 1 - result [1-63,0]
	// rotrdi %r2,%r2,1      ;// Rotate right 1 - result [0,63]
	// ;// Set up SRR0 / SRR1 to enable new MSR.
	// mtsrr1 %r2
	// // something here (updating MSR probably) causes a checkstop
	// li %r2, _start_postmsr@l
	// mtsrr0 %r2
	// // lis     %r9,49      ;// Want to default the NAP value
	// // ori     %r9,%r9,1    ;// Value is 0x0000000000310001
	// // mtspr   855,%r9     ;// set actual PSSCR
	// rfid
// _start_postmsr:

	/* TODO: Stack */
	LOAD_IMM64(%r1, _esecond_stack)

	/* WARNING: this assumes that ramstage is not relocatable */
	bl second_thread
	b .
.globl sys_reset_thread_int_end
sys_reset_thread_int_end:

int_vector_check_size sys_reset_thread_int 0x100


// /*
//  *
//  */
// .globl spinlock_lock
// spinlock_lock:
// 	// Read spinlock value.
// 	lis %r2, thread_spinlock@h
// 	ori %r2, %r2, thread_spinlock@l
// 1:
// 	ld %r3, 0(%r2)
// 	// Loop until value is 0...
// 	cmpwi %cr0, %r3, 1
// 	beq 1f
// 	// Lower thread priority.
// 	or 1,1,1
// 	b 1b
// 1:
// 	blr

// /*
//  *
//  */
// .globl spinlock_unlock
// spinlock_unlock:
// 	// Read spinlock value.
// 	lis %r2, thread_spinlock@h
// 	ori %r2, %r2, thread_spinlock@l
// 	lwsync
// 	stdi %r2, 0
// 	blr


// .section .data
//     .balign 1024

// .global thread_spinlock
// thread_spinlock:
//     .space 8
