/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef ARCH_SMP_SPINLOCK_H
#define ARCH_SMP_SPINLOCK_H

/*
 * Your basic SMP spinlocks, allowing only a single CPU anywhere
 */

typedef struct {
	volatile unsigned int lock;
} spinlock_t;

#define SPIN_LOCK_UNLOCKED { 1 }

#define STAGE_HAS_SPINLOCKS !ENV_ROMSTAGE_OR_BEFORE

#if STAGE_HAS_SPINLOCKS

#define DECLARE_SPIN_LOCK(x)	\
	static spinlock_t x = SPIN_LOCK_UNLOCKED;

/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */
#define barrier() __asm__ __volatile__("" : : : "memory")
#define spin_is_locked(x)	(*(volatile char *)(&(x)->lock) <= 0)
#define spin_unlock_wait(x)	do { barrier(); } while (spin_is_locked(x))
#undef barrier

#define spin_lock_string \
	"\n1:\t" \
	"lock ; decb %0\n\t" \
	"js 2f\n" \
	".section .text.lock,\"ax\"\n" \
	"2:\t" \
	"cmpb $0,%0\n\t" \
	"rep;nop\n\t" \
	"jle 2b\n\t" \
	"jmp 1b\n" \
	".previous"

/*
 * This works. Despite all the confusion.
 */
#define spin_unlock_string \
	"movb $1,%0"

static __always_inline void spin_lock(spinlock_t *lock)
{
	__asm__ __volatile__(
		spin_lock_string
		: "=m" (lock->lock) : : "memory");
}

static __always_inline void spin_unlock(spinlock_t *lock)
{
	__asm__ __volatile__(
		spin_unlock_string
		: "=m" (lock->lock) : : "memory");
}

#else

#define DECLARE_SPIN_LOCK(x)
#define spin_is_locked(lock)	0
#define spin_unlock_wait(lock)	do {} while (0)
#define spin_lock(lock)		do {} while (0)
#define spin_unlock(lock)	do {} while (0)

#endif

#endif /* ARCH_SMP_SPINLOCK_H */
