/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef ARCH_SMP_ATOMIC_H
#define ARCH_SMP_ATOMIC_H

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct { volatile int counter; } atomic_t;

#define ATOMIC_INIT(i)	{ (i) }

/** @file x86/include/arch/smp/atomic.h
 *
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

/**
 * atomic_read - read atomic variable
 * @param v: pointer of type atomic_t
 *
 * Atomically reads the value of v.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
#define atomic_read(v)		((v)->counter)

/**
 * atomic_set - set atomic variable
 * @param v: pointer of type atomic_t
 * @param i: required value
 *
 * Atomically sets the value of v to i.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
#define atomic_set(v, i)	(((v)->counter) = (i))

/**
 * atomic_inc - increment atomic variable
 * @param v: pointer of type atomic_t
 *
 * Atomically increments v by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static __always_inline void atomic_inc(atomic_t *v)
{
	__asm__ __volatile__(
		"lock ; incl %0"
		: "=m" (v->counter)
		: "m" (v->counter));
}

/**
 * atomic_dec - decrement atomic variable
 * @param v: pointer of type atomic_t
 *
 * Atomically decrements v by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static __always_inline void atomic_dec(atomic_t *v)
{
	__asm__ __volatile__(
		"lock ; decl %0"
		: "=m" (v->counter)
		: "m" (v->counter));
}

#endif /* ARCH_SMP_ATOMIC_H */
