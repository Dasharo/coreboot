/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_THREAD_H
#define __SOC_IBM_POWER9_THREAD_H

#include <stdbool.h>

struct spin_lock_t {
	volatile int value;
};

static inline void sync(void)
{
	asm volatile("sync" ::: "memory");
}

static inline int load_and_reserve(volatile int *p)
{
	int v;
	asm volatile ("lwarx %0, 0, %2" : "=r"(v) : "m"(*p), "r"(p));
	return v;
}

static inline int store_if_reserved(volatile int *p, int v)
{
	int r;
	asm volatile ("stwcx. %2, 0, %3 ; mfcr %0" :
		      "=r"(r), "=m"(*p) : "r"(v), "r"(p) : "memory", "cc");
	return r & 0x20000000;
}

static inline int compare_and_swap(volatile int *p, int v)
{
	int old;
	sync();

	do
		old = load_and_reserve(p);
	while (old == 0 && store_if_reserved(p, v) == 0);

	asm volatile ("isync" ::: "memory");
	return old;
}

static inline void atomic_store(volatile int *p, int v)
{
	sync();
	*p = v;
	sync();
}

static inline int spin_lock(struct spin_lock_t *s)
{
	asm volatile("or 31,31,31");	// Lower priority

	while (s->value || compare_and_swap(&s->value, 1) != 0)
		sync();

	asm volatile("or 2,2,2");	// Back to normal priority
	return 0;
}

static inline void spin_unlock(struct spin_lock_t *s)
{
	atomic_store(&s->value, 0);
}

void start_second_thread(void);
void stop_second_thread(void);
void wait_second_thread(void);
void on_second_thread(void (*func)(void *arg), void *arg);
void second_thread(void);

#endif /* __SOC_IBM_POWER9_THREAD_H */
