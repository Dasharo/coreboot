/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_ISTEP14_H
#define CPU_PPC64_ISTEP14_H

#include <stdint.h>

void istep_14_2(void);
void istep_14_3(uint8_t phb_active_mask, const uint8_t *iovalid_enable);

#endif /* CPU_PPC64_ISTEP14_H */
