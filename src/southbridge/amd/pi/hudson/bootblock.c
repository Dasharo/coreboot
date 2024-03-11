/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <arch/bootblock.h>
#include <amdblocks/acpimmio.h>
#include <device/pci_ops.h>
#include <southbridge/amd/pi/hudson/hudson.h>

void bootblock_early_southbridge_init(void)
{
	u32 data;

	hudson_enable_lpc_rom();
	enable_acpimmio_decode_pm04();
	hudson_lpc_decode();

	if (CONFIG(POST_DEVICE_PCI_PCIE))
		hudson_pci_port80();
	else if (CONFIG(POST_DEVICE_LPC))
		hudson_lpc_port80();

	const pci_devfn_t dev = PCI_DEV(0, 0x14, 3);
	data = pci_read_config32(dev, LPC_IO_OR_MEM_DECODE_ENABLE);
	/* enable 0x2e/0x4e IO decoding for SuperIO */
	pci_write_config32(dev, LPC_IO_OR_MEM_DECODE_ENABLE, data | 3);

	/*
	 * Enable FCH to decode TPM associated Memory and IO regions for vboot
	 *
	 * Enable decoding of TPM cycles defined in TPM 1.2 spec
	 * Enable decoding of legacy TPM addresses: IO addresses 0x7f-
	 * 0x7e and 0xef-0xee.
	 */

	data = pci_read_config32(dev, LPC_TRUSTED_PLATFORM_MODULE);
	data |= TPM_12_EN | TPM_LEGACY_EN;
	pci_write_config32(dev, LPC_TRUSTED_PLATFORM_MODULE, data);

	/*
	 *  In Hudson RRG, PMIOxD2[5:4] is "Drive strength control for
	 *  LpcClk[1:0]".  This following register setting has been
	 *  replicated in every reference design since Parmer, so it is
	 *  believed to be required even though it is not documented in
	 *  the SoC BKDGs.  Without this setting, there is no serial
	 *  output.
	 */
	pm_write8(0xd2, 0);
}
