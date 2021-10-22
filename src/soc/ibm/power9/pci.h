/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_HOMER_H
#define __SOC_IBM_POWER9_HOMER_H

#define MAX_PEC_PER_PROC 3
#define MAX_PHB_PER_PROC 6

/* Enum giving bitmask values for enabled PHBs */
enum phb_active_mask {
	PHB_MASK_NA = 0x00, // Sentinel mask (loop terminations)
	PHB0_MASK   = 0x80, // PHB0 enabled
	PHB1_MASK   = 0x40, // PHB1 enabled
	PHB2_MASK   = 0x20, // PHB2 enabled
	PHB3_MASK   = 0x10, // PHB3 enabled
	PHB4_MASK   = 0x08, // PHB4 enabled
	PHB5_MASK   = 0x04, // PHB5 enabled
};

void pci_init(void);

#endif /* __SOC_IBM_POWER9_HOMER_H  */
