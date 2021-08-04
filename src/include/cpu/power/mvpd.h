/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_MVPD_H
#define CPU_PPC64_MVPD_H

#include <stdbool.h>
#include <stdint.h>

struct region_device;

void mvpd_device_init(void);

void mvpd_device_unmount(void);

const struct region_device *mvpd_device_ro(void);

/* Finds a specific keyword in MVPD partition and extracts it. *size is updated
 * to reflect needed or used space in the buffer. */
bool mvpd_extract_keyword(const char *record_name, const char *kwd_name,
			  uint8_t *buf, uint32_t *size);

/* Finds a specific ring in MVPD partition and extracts it */
bool mvpd_extract_ring(const char *record_name, const char *kwd_name,
		       uint8_t chiplet_id, uint8_t even_odd, uint16_t ring_id,
		       uint8_t *buf, uint32_t buf_size);

#endif /* CPU_PPC64_MVPD_H */
