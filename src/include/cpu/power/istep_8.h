/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_ISTEP8_H
#define CPU_PPC64_ISTEP8_H

#include <stdint.h>

void istep_8_1(uint8_t chips);
void istep_8_2(uint8_t chips);
void istep_8_3(uint8_t chips);
void istep_8_4(uint8_t chips);
void istep_8_9(uint8_t chips);
void istep_8_10(uint8_t chips);

/* These functions access SCOM of the second CPU using SBE IO, thus they can be
 * used only in isteps that come after 8.4 */
void put_scom(uint8_t chip, uint64_t addr, uint64_t data);
uint64_t get_scom(uint8_t chip, uint64_t addr);

#endif /* CPU_PPC64_ISTEP8_H */
