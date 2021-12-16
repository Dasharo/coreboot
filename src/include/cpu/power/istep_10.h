/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_ISTEP10_H
#define CPU_PPC64_ISTEP10_H

#include <stdint.h>

void istep_10_10(uint8_t *phb_active_mask, uint8_t *iovalid_enable);
void istep_10_12(void);
void istep_10_13(void);

#endif /* CPU_PPC64_ISTEP10_H */
