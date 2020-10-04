/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <stdbool.h>
#include <asan.h>

void *memcpy(void *dest, const void *src, size_t n)
{
	unsigned long d0, d1, d2;

#if (ENV_ROMSTAGE && CONFIG(ASAN_IN_ROMSTAGE)) || \
		(ENV_RAMSTAGE && CONFIG(ASAN_IN_RAMSTAGE))
	check_memory_region((unsigned long)src, n, false, _RET_IP_);
	check_memory_region((unsigned long)dest, n, true, _RET_IP_);
#endif

	asm volatile(
#ifdef __x86_64__
		"rep ; movsd\n\t"
		"mov %4,%%rcx\n\t"
#else
		"rep ; movsl\n\t"
		"movl %4,%%ecx\n\t"
#endif
		"rep ; movsb\n\t"
		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		: "0" (n >> 2), "g" (n & 3), "1" (dest), "2" (src)
		: "memory"
	);

	return dest;
}
