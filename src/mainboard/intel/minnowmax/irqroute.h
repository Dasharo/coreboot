/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/irq.h>
#include <soc/pci_devs.h>
#include <soc/pm.h>

/*
 *IR02h GFX      INT(A)     - PIRQ A
 *IR10h EMMC     INT(ABCD)  - PIRQ DEFG
 *IR11h SDIO     INT(A)     - PIRQ B
 *IR12h SD       INT(A)     - PIRQ C
 *IR13h SATA     INT(A)     - PIRQ D
 *IR14h XHCI     INT(A)     - PIRQ E
 *IR15h LP Audio INT(A)     - PIRQ F
 *IR17h MMC      INT(A)     - PIRQ F
 *IR18h SIO      INT(ABCD)  - PIRQ BADC
 *IR1Ah TXE      INT(A)     - PIRQ F
 *IR1Bh HD Audio INT(A)     - PIRQ G
 *IR1Ch PCIe     INT(ABCD)  - PIRQ EFGH
 *IR1Dh EHCI     INT(A)     - PIRQ D
 *IR1Eh SIO      INT(ABCD)  - PIRQ BDEF
 *IR1Fh LPC      INT(ABCD)  - PIRQ HGBC
 */

/* Devices set as A, A, A, A evaluate as 0, and don't get set */
#define PCI_DEV_PIRQ_ROUTES \
	PCI_DEV_PIRQ_ROUTE(GFX_DEV,     A, A, A, B), \
	PCI_DEV_PIRQ_ROUTE(MMC_DEV,     D, E, F, G), \
	PCI_DEV_PIRQ_ROUTE(SDIO_DEV,    B, A, A, A), \
	PCI_DEV_PIRQ_ROUTE(SD_DEV,      C, A, A, A), \
	PCI_DEV_PIRQ_ROUTE(SATA_DEV,    D, A, A, A), \
	PCI_DEV_PIRQ_ROUTE(XHCI_DEV,    E, A, A, A), \
	PCI_DEV_PIRQ_ROUTE(LPE_DEV,     F, A, A, A), \
	PCI_DEV_PIRQ_ROUTE(MMC45_DEV,   F, A, A, A), \
	PCI_DEV_PIRQ_ROUTE(SIO1_DEV,    B, A, D, C), \
	PCI_DEV_PIRQ_ROUTE(TXE_DEV,     F, A, A, A), \
	PCI_DEV_PIRQ_ROUTE(HDA_DEV,     G, A, A, A), \
	PCI_DEV_PIRQ_ROUTE(PCIE_DEV,    E, F, G, H), \
	PCI_DEV_PIRQ_ROUTE(EHCI_DEV,    D, A, A, A), \
	PCI_DEV_PIRQ_ROUTE(SIO2_DEV,    B, D, E, F), \
	PCI_DEV_PIRQ_ROUTE(PCU_DEV,     H, G, B, C)

/*
 * Route each PIRQ[A-H] to a PIC IRQ[0-15]
 * Reserved: 0, 1, 2, 8, 13
 * PS2 keyboard: 12
 * ACPI/SCI: 9
 * Floppy: 6
 */
#define PIRQ_PIC_ROUTES \
	PIRQ_PIC(A,  3), \
	PIRQ_PIC(B,  5), \
	PIRQ_PIC(C,  7), \
	PIRQ_PIC(D, 10), \
	PIRQ_PIC(E, 11), \
	PIRQ_PIC(F, 12), \
	PIRQ_PIC(G, 14), \
	PIRQ_PIC(H, 15)

