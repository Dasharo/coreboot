/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef VARIANT_PCIE_H
#define VARIANT_PCIE_H

#include <types.h>

/*
 * Split of the M.2 M-key slot set by the descriptor in flash, as one of
 * PCIE_BIFURCATION_OPTION_*. 1x4 if it can't be read.
 */
uint8_t variant_pcie_bifurcation(void);

#endif /* VARIANT_PCIE_H */
