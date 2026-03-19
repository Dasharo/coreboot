/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef AMD_BLOCK_PCI_MMCONF_H
#define AMD_BLOCK_PCI_MMCONF_H

#include <types.h>

void enable_pci_mmconf(void);
uint32_t soc_get_df_func0_smn_base(void);

#endif /* AMD_BLOCK_PCI_MMCONF_H */
