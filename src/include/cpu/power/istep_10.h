/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_ISTEP10_H
#define CPU_PPC64_ISTEP10_H

#include <stdint.h>

struct pci_info;

void istep_10_1(uint8_t chips);
void istep_10_6(uint8_t chips);
void istep_10_10(uint8_t chips, struct pci_info *pci_info);
void istep_10_12(uint8_t chips);
void istep_10_13(void);

#endif /* CPU_PPC64_ISTEP10_H */
