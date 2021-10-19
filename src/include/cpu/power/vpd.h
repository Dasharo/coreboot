/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_VPD_H
#define CPU_PPC64_VPD_H

#include <stddef.h>
#include <stdint.h>

#define VPD_RECORD_NAME_LEN 4
#define VPD_KWD_NAME_LEN    2
#define VPD_RECORD_SIZE_LEN 2

void vpd_pnor_main(void);

/* Finds a keyword by its name. Retrieves its size too. Returns NULL on
 * failure. */
const uint8_t *vpd_find_kwd(const uint8_t *record, const char *record_name,
			    const char *kwd_name, size_t *size);

#endif /* CPU_PPC64_VPD_H */
