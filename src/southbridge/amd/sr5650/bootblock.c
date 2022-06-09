/* SPDX-License-Identifier: GPL-2.0-only */

#include "cmn.h"
#include "sr5650.h"

/*****************************************************************
* The sr5650 uses NBCONFIG:0x1c (BAR3) to map the PCIE Extended Configuration
* Space to a 256MB range within the first 4GB of addressable memory.
*****************************************************************/
void enable_pcie_bar3(void)
{
	pci_devfn_t nb_dev = PCI_DEV(0, 0, 0);

	/* Enable writes to the BAR3 register */
	set_nbcfg_enable_bits(nb_dev, 0x7C, 1 << 30, 1 << 30);

	/* Set number of valid BAR3 address bits (implies number of buses) */
	set_nbcfg_enable_bits(nb_dev, 0x84, 7 << 16, 0 << 16);

	/* Set BAR3 */
	pci_write_config32(nb_dev, 0x1C, CONFIG_MMCONF_BASE_ADDRESS);
	pci_write_config32(nb_dev, 0x20, 0x00000000);

	/* Enable decoding of BAR3 */
	set_htiu_enable_bits(nb_dev, 0x32, 1 << 28, 1 << 28);

	/* Disable writes to the BAR3. */
	set_nbcfg_enable_bits(nb_dev, 0x7C, 1 << 30, 0 << 30);

	/* Hide BAR3 so resource allocator won't complain about read-only BAR */
	set_nbmisc_enable_bits(nb_dev, 0x0, 1 << 3, 1 << 3);
}

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
	enable_pcie_bar3();
}

/* enable CFG access to Dev8, which is the SB P2P Bridge */
void enable_sr5650_dev8(void)
{
	set_nbmisc_enable_bits(PCI_DEV(0, 0, 0), 0x00, 1 << 6, 1 << 6);
}
