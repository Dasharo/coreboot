/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Early initialization code for POWER8.
 */
#include <cpu/power/spr.h>

#define FIXUP_ENDIAN						   \
	tdi   0,0,0x48;	  /* Reverse endian of b . + 8		*/ \
	b     $+44;	  /* Skip trampoline if endian is good	*/ \
	.long 0xa600607d; /* mfmsr r11				*/ \
	.long 0x01006b69; /* xori r11,r11,1			*/ \
	.long 0x00004039; /* li r10,0				*/ \
	.long 0x6401417d; /* mtmsrd r10,1			*/ \
	.long 0x05009f42; /* bcl 20,31,$+4			*/ \
	.long 0xa602487d; /* mflr r10				*/ \
	.long 0x14004a39; /* addi r10,r10,20			*/ \
	.long 0xa6035a7d; /* mtsrr0 r10				*/ \
	.long 0xa6037b7d; /* mtsrr1 r11				*/ \
	.long 0x2400004c  /* rfid				*/

/* Load an immediate 64-bit value into a register */
#define LOAD_IMM64(r, e)			\
	lis     r,(e)@highest;			\
	ori     r,r,(e)@higher;			\
	rldicr  r,r, 32, 31;			\
	oris    r,r, (e)@h;			\
	ori     r,r, (e)@l;

.section ".text._start", "ax", %progbits
.globl _start
_start:
	/* QEMU with hb-mode=on starts at address 0x10, while hardware at 0x0. */
#if !CONFIG(BOOTBLOCK_IN_SEEPROM)
	nop
	nop
	nop
	nop
	FIXUP_ENDIAN
#endif

	/* Set program priority to medium */
	or		%r2, %r2, %r2

#if CONFIG(BOOTBLOCK_IN_SEEPROM)
	/* SBE sets HRMOR to 4G - 128M + 2M. Our bootblock is loaded 12 KB above
	 * that address - it is used as a place for interrupt vectors in Hostboot,
	 * but it isn't actually a part of the HBBL partition to save space in
	 * SEEPROM. At the entry we are not loaded at the address we are linked for.
	 *
	 * HRMOR isn't added to the addresses, it is OR'ed with them. Because the
	 * bit corresponding to 2M is set, address space overlaps every 2M. To be
	 * able to use full 10M of L3 cache we must change it to something with
	 * bigger alignment.
	 */

	lis		%r12, 0x20		/* Bit for 2M */
	mfspr	%r11, 313		/* HRMOR */
	li		%r10, 1
	rotldi	%r10, %r10, 63	/* Most significant bit is "ignore HRMOR" */

	/* Bit for 2M is set both in ignoreHRMOR and HRMOR, that is expected */
	lis		%r9, ignoreHRMOR@h
	ori		%r9, %r9, ignoreHRMOR@l
	or		%r9, %r9, %r11
	or		%r9, %r9, %r10

	mtlr %r9
	blr

ignoreHRMOR:
	/* Now we are at address 0x80000000F82030xx */
	andc	%r11, %r11, %r12	/* Clear bit for 2M */
	mtspr	SPR_HRMOR, %r11		/* Update HRMOR */
	isync

	/* We can't just "b stopIgnoringHRMOR", it would use relative offset */
	addi	%r9, %r9, stopIgnoringHRMOR - ignoreHRMOR
	mtlr %r9
	blr

stopIgnoringHRMOR:
	/* Now we are at linked address */

	/* L3 cache is invalid except for [2M, end of HBBL] range. Make the rest of
	 * it valid, or embrace the checkstops. End of loaded part of HBBL is at
	 * address 2M + 12 (see https://github.com/open-power/hostboot/blob/master/src/import/chips/p9/procedures/hwp/nest/p9_sbe_hb_structures.H#L84),
	 * the first 4 bytes are for the jump into HBBL). */

	/* Validate and initialize to zeroes the first 2M */
	li		%r5, 0
	lis		%r6, 0x20
	addi	%r6, %r6, -1
1:
	dcbz	0, %r5
	addi	%r5, %r5, 128
	cmpld	cr7, %r5, %r6
	blt		cr7, 1b

	/* Validate and initialize to zeroes [end of HBBL, 10M] range */
	ld	%r5, 12(%r12)
	add	%r5, %r5, %r12
	addis	%r6, %r6, 0x80		/* Add 8M = 10M - 2M */
1:
	dcbz	0, %r5
	addi	%r5, %r5, 128
	cmpld	cr7, %r5, %r6
	blt		cr7, 1b

#endif

	/* Stack */
	lis		%r1, _estack@ha
	addi	%r1, %r1, _estack@l

	/* This is tested by checkstack() just before jumping to payload */
	LOAD_IMM64(%r3, 0xDEADBEEFDEADBEEF)
	lis		%r5, _stack@ha
	addi	%r5, %r5, _stack@l
	subi	%r5, %r5, 8
	sub		%r4, %r1, %r5
	sradi	%r4, %r4, 3		/* Divide by 8 */
	mtctr	%r4
1:
	stdu	%r3, 8(%r5)
	bc		25, 0, 1b

	/* Enable floating point and vector operations */
	/* Vector operations are sometimes generated for code like
	 * 'uint8_t x[32] = {0}', this results in an exception when vector registers
	 * (VEC) are not enabled. VSX (vector-scalar extension) is also enabled,
	 * there is no reason not to. Floating point must also be enabled for VSX.
	 */
	mfmsr	%r3
	ori		%r3, %r3, 0x2000	/* FP = 1 */
	oris	%r3, %r3, 0x0280	/* VEC = 1, VSX = 1 */
	mtmsr	%r3

	/* Load official procedure descriptor address for main() */
	lis		%r12, main@ha
	addi	%r12, %r12, main@l

	/* Load TOC pointer and jump to main() */
	ld		%r2, 8(%r12)
	b main
