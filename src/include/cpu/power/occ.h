/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_OCC_H
#define CPU_PPC64_OCC_H

#include <stdint.h>

struct homer_st;

void clear_occ_special_wakeups(uint64_t cores);
void special_occ_wakeup_disable(uint64_t cores);
void occ_start_from_mem(void);
/* Moves OCC to active state */
void activate_occ(struct homer_st *homer);

void pm_occ_fir_init(void);
void pm_pba_fir_init(void);

#endif /* CPU_PPC64_OCC_H */
