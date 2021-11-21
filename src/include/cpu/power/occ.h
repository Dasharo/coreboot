/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_OCC_H
#define CPU_PPC64_OCC_H

#include <stdint.h>

void writeOCCSRAM(uint8_t chip, uint32_t address, uint64_t *buffer, size_t data_length);
void readOCCSRAM(uint8_t chip, uint32_t address, uint64_t *buffer, size_t data_length);
void write_occ_command(uint8_t chip, uint64_t write_data);
void clear_occ_special_wakeups(uint8_t chip, uint64_t cores);
void special_occ_wakeup_disable(uint8_t chip, uint64_t cores);
void occ_start_from_mem(uint8_t chip);

void pm_occ_fir_init(uint8_t chip);
void pm_pba_fir_init(uint8_t chip);

#endif /* CPU_PPC64_OCC_H */
