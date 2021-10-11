/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootblock_common.h>
#include <device/pci_ops.h>
#include <superio/winbond/common/winbond.h>
#include <superio/winbond/w83667hg-a/w83667hg-a.h>

#define SERIAL_0_DEV PNP_DEV(0x2e, W83667HG_A_SP1)
#define SERIAL_1_DEV PNP_DEV(0x2e, W83667HG_A_SP2)

void bootblock_mainboard_early_init(void)
{
	u8 reg8;

	/* Configure secondary serial port pin mux */
	winbond_set_pinmux(SERIAL_1_DEV, 0x2a, W83667HG_SPI_PINMUX_GPIO4_SERIAL_B_MASK, W83667HG_SPI_PINMUX_SERIAL_B);

	/* Initialize early serial */
	winbond_enable_serial(SERIAL_0_DEV, CONFIG_TTYS0_BASE);

	/* Disable LPC legacy DMA support to prevent lockup */
	reg8 = pci_read_config8(PCI_DEV(0, 0x14, 3), 0x78);
	reg8 &= ~(1 << 0);
	pci_write_config8(PCI_DEV(0, 0x14, 3), 0x78, reg8);
}
