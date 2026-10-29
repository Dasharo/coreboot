/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _EDK2_CAPSULES_H_
#define _EDK2_CAPSULES_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if CONFIG(DRIVERS_EFI_UPDATE_CAPSULES)

void efi_parse_capsules(uintptr_t *base, size_t *size);

void efi_add_capsules_to_bootmem(void);

/*
 * Check if a RAM capsule was loaded by reading CapsuleUpdateData.
 * true  - capsule is pending
 * false - no capsule pending
 */
bool efi_ram_capsules_pending(void);

#else

static inline void efi_parse_capsules(uintptr_t *base, size_t *size)
{
	*base = 0;
	*size = 0;
}

static inline void efi_add_capsules_to_bootmem(void) { }

static inline bool efi_ram_capsules_pending(void) { return false; }

#endif

#endif /* _EDK2_CAPSULES_H_ */
