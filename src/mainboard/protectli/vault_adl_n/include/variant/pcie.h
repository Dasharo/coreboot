/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef VARIANT_PCIE_H
#define VARIANT_PCIE_H

#include <types.h>

/* True if the descriptor in flash splits the M.2 M-key slot as 2x2 */
bool variant_pcie_is_2x2(void);

#endif /* VARIANT_PCIE_H */
