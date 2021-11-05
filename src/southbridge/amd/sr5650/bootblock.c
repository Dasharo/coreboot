/* SPDX-License-Identifier: GPL-2.0-only */

#include "cmn.h"
#include "sr5650.h"

/**
 * @brief disable GPP1 Port0,1, GPP2, GPP3a Port0,1,2,3,4,5, GPP3b
 *
 */
void sr5650_disable_pcie_bridge(void)
{
	u32 mask;
	u32 reg;
	pci_devfn_t nb_dev = PCI_DEV(0, 0, 0);

	mask = (1 << 2) | (1 << 3); /*GPP1*/
	mask |= (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 16) | (1 << 17); /*GPP3a*/
	mask |= (1 << 18) | (1 << 19); /*GPP2*/
	mask |= (1 << 20); /*GPP3b*/
	reg = mask;
	set_nbmisc_enable_bits(nb_dev, 0x0c, mask, reg);
}

/* enable CFG access to Dev8, which is the SB P2P Bridge */
void enable_sr5650_dev8(void)
{
	set_nbmisc_enable_bits(PCI_DEV(0, 0, 0), 0x00, 1 << 6, 1 << 6);
}
