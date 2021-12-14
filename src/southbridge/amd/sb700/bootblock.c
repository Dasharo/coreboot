/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <arch/mmio.h>
#include <arch/bootblock.h>
#include <console/console.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#if CONFIG(SOUTHBRIDGE_AMD_SR5650)
#include <southbridge/amd/sr5650/sr5650.h>
#endif
#include "sb700.h"

#define SPI_BASE_ADDRESS_REG		0xa0

#define SPI_CONTROL_1			0xc

static void sb7xx_51xx_pci_port80(void)
{
	u8 byte;
	pci_devfn_t dev;

	/* P2P Bridge */
	dev = PCI_DEV(0, 0x14, 4);

	/* Chip Control: Enable subtractive decoding */
	byte = pci_read_config8(dev, 0x40);
	byte |= 1 << 5;
	pci_write_config8(dev, 0x40, byte);

	/* Misc Control: Enable subtractive decoding if 0x40 bit 5 is set */
	byte = pci_read_config8(dev, 0x4B);
	byte |= 1 << 7;
	pci_write_config8(dev, 0x4B, byte);

	/* The same IO Base and IO Limit here is meaningful because we set the
	 * bridge to be subtractive. During early setup stage, we have to make
	 * sure that data can go through port 0x80.
	 */
	/* IO Base: 0xf000 */
	byte = pci_read_config8(dev, 0x1C);
	byte |= 0xF << 4;
	pci_write_config8(dev, 0x1C, byte);

	/* IO Limit: 0xf000 */
	byte = pci_read_config8(dev, 0x1D);
	byte |= 0xF << 4;
	pci_write_config8(dev, 0x1D, byte);

	/* PCI Command: Enable IO response */
	byte = pci_read_config8(dev, 0x04);
	byte |= 1 << 0;
	pci_write_config8(dev, 0x04, byte);

	/* LPC controller */
	dev = PCI_DEV(0, 0x14, 3);

	byte = pci_read_config8(dev, 0x4A);
	byte &= ~(1 << 5);	/* disable lpc port 80 */
	pci_write_config8(dev, 0x4A, byte);
}

static void sb7xx_51xx_lpc_port80(void)
{
	u8 byte;
	pci_devfn_t dev;

	/* Enable port 80 LPC decode in pci function 3 configuration space. */
	dev = PCI_DEV(0, 0x14, 3);
	byte = pci_read_config8(dev, 0x4a);
	byte |= 1 << 5;		/* enable port 80 */
	pci_write_config8(dev, 0x4a, byte);
}

/***************************************
* Legacy devices are mapped to LPC space.
*	Serial port 0
*	KBC Port
*	ACPI Micro-controller port
*	This function does not change port 0x80 decoding.
*	Console output through any port besides 0x3f8 is unsupported.
*	If you use FWH ROMs, you have to setup IDSEL.
***************************************/
static void sb7xx_51xx_lpc_init(void)
{
	u8 reg8;
	u32 reg32;
	pci_devfn_t dev;

	dev = PCI_DEV(0, 0x14, 0);	/* SMBUS controller */
	/* NOTE: Set BootTimerDisable, otherwise it would keep rebooting!!
	 * This bit has no meaning if debug strap is not enabled. So if the
	 * board keeps rebooting and the code fails to reach here, we could
	 * disable the debug strap first.
	 */
	reg32 = pci_read_config32(dev, 0x4C);
	reg32 |= 1 << 31;
	pci_write_config32(dev, 0x4C, reg32);

	/* Enable lpc controller */
	reg32 = pci_read_config32(dev, 0x64);
	reg32 |= 1 << 20;
	pci_write_config32(dev, 0x64, reg32);

	/* Enable SB I/O decodes*/
	pci_write_config8(dev, 0x78, 0xFF);
	//pci_write_config8(dev, 0x79, 0x45);

	if (CONFIG(SOUTHBRIDGE_AMD_SB700_DISABLE_ISA_DMA)) {
		/* Disable LPC ISA DMA Capability */
		reg8 = pci_read_config8(dev, 0x78);
		reg8 &= ~(1 << 0);
		pci_write_config8(dev, 0x78, reg8);
	}

	dev = PCI_DEV(0, 0x14, 3);
	if (CONFIG(SOUTHBRIDGE_AMD_SUBTYPE_SP5100)) {
		post_code(0x66);
		reg8 = pci_read_config8(dev, 0xBB);
		reg8 |= 1 << 2 | 1 << 3 | 1 << 6 | 1 << 7;
		reg8 &= ~(1 << 1);
		pci_write_config8(dev, 0xBB, reg8);
	}
	/* SB700 LPC Bridge 0x48.
	 * Turn on all LPC IO Port decode enables
	 */
	pci_write_config32(dev, 0x44, 0xffffffff);

	/* SB700 LPC Bridge 0x48.
	 * BIT0: Port Enable for SuperIO 0x2E-0x2F
	 * BIT1: Port Enable for SuperIO 0x4E-0x4F
	 * BIT6: Port Enable for RTC IO 0x70-0x73
	 */
	reg8 = pci_read_config8(dev, 0x48);
	reg8 |= (1 << 0) | (1 << 1) | (1 << 6);
	pci_write_config8(dev, 0x48, reg8);

	/* Super I/O, RTC */
	reg8 = pci_read_config8(dev, 0x48);
	/* Decode ports 0x2e-0x2f, 0x4e-0x4f (SuperI/O configuration) */
	reg8 |= (1 << 1) | (1 << 0);
	/* Decode port 0x70-0x73 (RTC) */
	reg8 |= (1 << 6);
	pci_write_config8(dev, 0x48, reg8);

	/* Enable RTC AltCentury if requested */
	if (CONFIG(USE_PC_CMOS_ALTCENTURY))
		pmio_write(0x7c, pmio_read(0x7c) | 0x10);
}

/*
 * Enable 4MB (LPC) ROM access at 0xFFC00000 - 0xFFFFFFFF.
 *
 * Hardware should enable LPC ROM by pin straps. This function does not
 * handle the theoretically possible PCI ROM, FWH, or SPI ROM configurations.
 *
 * The SB700 power-on default is to map 512K ROM space.
 *
 * Details: AMD SB700/710/750 BIOS Developer's Guide (BDG), Rev. 1.00,
 *          PN 43366_sb7xx_bdg_pub_1.00, June 2009, section 3.1, page 14.
 */
static void sb700_enable_rom(void)
{
	u8 reg8;
	pci_devfn_t dev;

	dev = PCI_DEV(0, 0x14, 3);

	reg8 = pci_read_config8(dev, 0x48);
	/* Decode variable LPC ROM address ranges 1 and 2. */
	reg8 |= (1 << 3) | (1 << 4);
	pci_write_config8(dev, 0x48, reg8);

	/* LPC ROM address range 1: */
	/* Enable LPC ROM range mirroring start at 0x000e(0000). */
	pci_write_config16(dev, 0x68, 0x000e);
	/* Enable LPC ROM range mirroring end at 0x000f(ffff). */
	pci_write_config16(dev, 0x6a, 0x000f);

	/* LPC ROM address range 2: */
	/*
	 * Enable LPC ROM range start at:
	 * 0xfff8(0000): 512KB
	 * 0xfff0(0000): 1MB
	 * 0xffe0(0000): 2MB
	 * 0xffc0(0000): 4MB
	 * 0xff80(0000): 8MB
	 */
	pci_write_config16(dev, 0x6c, 0x10000 - (CONFIG_COREBOOT_ROMSIZE_KB >> 6));
	/* Enable LPC ROM range end at 0xffff(ffff). */
	pci_write_config16(dev, 0x6e, 0xffff);
}

static void sb700_configure_rom(void)
{
	pci_devfn_t dev;
	u32 dword;
	u8 reg8;

	dev = PCI_DEV(0, 0x14, 3);

	pci_read_config32(dev, SPI_BASE_ADDRESS_REG);
	dword = (uintptr_t)SPI_BASE_ADDRESS & ~0x1f;
	dword |= (0x1 << 1);	/* SpiRomEnable = 1 */
	pci_write_config32(dev, SPI_BASE_ADDRESS_REG, dword);

	if (CONFIG(SOUTHBRIDGE_AMD_SB700_33MHZ_SPI)) {
		dword = read32(SPI_BASE_ADDRESS + SPI_CONTROL_1);
		dword &= ~(0x3 << 12);	/* NormSpeed = 0x1 */
		dword |= (0x1 << 12);
		write32(SPI_BASE_ADDRESS + SPI_CONTROL_1, dword);
	}

	/* Enable PrefetchEnSPIFromHost to speed up SPI flash read (does not affect LPC) */
	reg8 = pci_read_config8(dev, 0xbb);
	reg8 |= 1 << 0;
	pci_write_config8(dev, 0xbb, reg8);
}

static void sb700_enable_tpm_decoding(void)
{
	u8 reg8;
	pci_devfn_t dev;

	dev = PCI_DEV(0, 0x14, 3);

	reg8 = pci_read_config8(dev, 0x7c);
	reg8 |= 0x81;	/* Tpm12_en and decode 0xfed4xxxx*/
	pci_write_config8(dev, 0x7c, reg8);
}

void bootblock_early_southbridge_init(void)
{
	sb700_enable_rom();
	sb700_configure_rom();
	sb7xx_51xx_lpc_init();

	if (CONFIG(POST_DEVICE_LPC))
		sb7xx_51xx_lpc_port80();
	if (CONFIG(POST_DEVICE_PCI_PCIE))
		sb7xx_51xx_pci_port80();

	sb700_enable_tpm_decoding();

#if CONFIG(SOUTHBRIDGE_AMD_SR5650)
	sr5650_disable_pcie_bridge();
	enable_sr5650_dev8();
#endif
}

void save_bios_ram_data(u32 dword, int size, int biosram_pos)
{
	int i;
	for (i = 0; i < size; i++) {
		outb(biosram_pos + i, BIOSRAM_INDEX);
		outb((dword >> (8 * i)) & 0xff, BIOSRAM_DATA);
	}
}

void load_bios_ram_data(u32 *dword, int size, int biosram_pos)
{
	u32 data = 0;
	int i;
	for (i = 0; i < size; i++) {
		outb(biosram_pos + i, BIOSRAM_INDEX);
		data &= ~(0xff << (i * 8));
		data |= inb(BIOSRAM_DATA) << (i *8);
	}

	*dword = data;
}
