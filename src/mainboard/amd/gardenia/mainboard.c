/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/device.h>
#include <amdblocks/agesawrapper.h>
#include <amdblocks/amd_pci_util.h>
#include <soc/southbridge.h>

#include "gpio.h"

/***********************************************************
 * These arrays set up the FCH PCI_INTR registers 0xC00/0xC01.
 * This table is responsible for physically routing the PIC and
 * IOAPIC IRQs to the different PCI devices on the system.  It
 * is read and written via registers 0xC00/0xC01 as an
 * Index/Data pair.  These values are chipset and mainboard
 * dependent and should be updated accordingly.
 *
 * These values are used by the PCI configuration space,
 * MP Tables.  TODO: Make ACPI use these values too.
 */
static const u8 mainboard_picr_data[] = {
	[0x00] = 0x03, 0x04, 0x05, 0x07, 0x0B, 0x0A, 0x1F, 0x1F,
	[0x08] = 0xFA, 0xF1, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F,
	[0x10] = 0x1F, 0x1F, 0x1F, 0x03, 0x1F, 0x1F, 0x1F, 0x1F,
	[0x18] = 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x20] = 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00,
	[0x28] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x30] = 0x05, 0x04, 0x05, 0x04, 0x04, 0x05, 0x04, 0x05,
	[0x38] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x40] = 0x04, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x48] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x50] = 0x03, 0x04, 0x05, 0x07, 0x1F, 0x1F, 0x1F, 0x1F,
	[0x58] = 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	[0x60] = 0x1F, 0x1F, 0x07, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	[0x68] = 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	[0x70] = 0x03, 0x0F, 0x06, 0x0E, 0x0A, 0x0B, 0x1F, 0x1F,
	[0x78] = 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
};

static const u8 mainboard_intr_data[] = {
	[0x00] = 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	[0x08] = 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F,
	[0x10] = 0x09, 0x1F, 0x1F, 0x10, 0x1F, 0x1F, 0x1F, 0x10,
	[0x18] = 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x20] = 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00,
	[0x28] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x30] = 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x00,
	[0x38] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x40] = 0x11, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x48] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x50] = 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00,
	[0x58] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x60] = 0x1F, 0x1F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x68] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	[0x70] = 0x03, 0x0F, 0x06, 0x0E, 0x0A, 0x0B, 0x1F, 0x1F,
	[0x78] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* PIRQ Setup */
static void pirq_setup(void)
{
	intr_data_ptr = mainboard_intr_data;
	picr_data_ptr = mainboard_picr_data;
}

static void mainboard_init(void *chip_info)
{
	size_t num_gpios;
	const struct soc_amd_gpio *gpios;
	gpios = gpio_table(&num_gpios);
	program_gpios(gpios, num_gpios);
}

/*************************************************
 * enable the dedicated function in gardenia board.
 *************************************************/
static void gardenia_enable(struct device *dev)
{
	printk(BIOS_INFO, "Mainboard "
				CONFIG_MAINBOARD_PART_NUMBER " Enable.\n");

	/* Initialize the PIRQ data structures for consumption */
	pirq_setup();
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = gardenia_enable,
};
