/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_OCC_H
#define CPU_PPC64_OCC_H

#include <stdbool.h>
#include <stdint.h>

struct homer_st;

void clear_occ_special_wakeups(uint8_t chip, uint64_t cores);
void special_occ_wakeup_disable(uint8_t chip, uint64_t cores);
void occ_start_from_mem(uint8_t chip);
/* Moves OCC to active state */
void activate_occ(uint8_t chip, struct homer_st *homer, bool is_master);

void pm_occ_fir_init(uint8_t chip);
void pm_pba_fir_init(uint8_t chip);

#endif /* CPU_PPC64_OCC_H */
