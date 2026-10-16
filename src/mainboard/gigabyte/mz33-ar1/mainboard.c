/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/acpi.h>
#include <amdblocks/amd_pci_util.h>
#include <cbmem.h>
#include <drivers/amd/opensil/opensil.h>
#include <drivers/ipmi/ipmi_ops.h>
#include <smbios.h>
#include <soc/amd_pci_int_defs.h>
#include <stdio.h>
#include <types.h>

#include "smbios_oem.h"

/*
 * The BMC holds the board FRU as FRU device 0. Its board area has the
 * unique board serial number and the board part number, its product area
 * the system serial number. The stock BIOS fills SMBIOS type 2 (serial
 * number, and version from the part number) and type 1 from these. FRU
 * device 1 only has default values. Read it once, when the SMBIOS tables
 * are written.
 */
#define MZ33_FRU_DEVICE_ID	0

static const struct fru_info_str *board_fru(void)
{
	static struct fru_info_str fru;
	static bool done;

	if (!done) {
		done = true;
		read_fru_areas(CONFIG_BMC_KCS_BASE, MZ33_FRU_DEVICE_ID, 0, &fru);
	}

	return &fru;
}

static const char *fru_string_or_default(const char *str, const char *fallback)
{
	return (str && *str) ? str : fallback;
}

const char *smbios_mainboard_serial_number(void)
{
	return fru_string_or_default(board_fru()->board_info.serial_number,
				     CONFIG_MAINBOARD_SERIAL_NUMBER);
}

const char *smbios_system_version(void)
{
	return fru_string_or_default(board_fru()->prod_info.product_version, "0100");
}

const char *smbios_system_serial_number(void)
{
	return fru_string_or_default(board_fru()->prod_info.serial_number,
				     CONFIG_MAINBOARD_SERIAL_NUMBER);
}

/* The IRQ mapping in fch_irq_map ends up getting written to the indirect
   address space that is accessed via I/O ports 0xc00/0xc01. */

/*
 * This controls the device -> IRQ routing.
 *
 * Hardcoded IRQs:
 *  0: timer < soc/amd/common/acpi/lpc.asl
 *  1: i8042 - Keyboard
 *  2: cascade
 *  8: rtc0 <- soc/amd/common/acpi/lpc.asl
 *  9: acpi <- soc/amd/common/acpi/lpc.asl
 */
static const struct fch_irq_routing fch_irq_map[] = {
	{ PIRQ_A,	0x0a,		0x10 },
	{ PIRQ_B,	0x0b,		0x11 },
	{ PIRQ_C,	0x0b,		0x12 },
	{ PIRQ_D,	0x0a,		0x13 },
	{ PIRQ_E,	0x0a,		0x14 },
	{ PIRQ_F,	0x0b,		0x15 },
	{ PIRQ_G,	0x0b,		0x16 },
	{ PIRQ_H,	0x0a,		0x17 },
	{ PIRQ_SCI,	ACPI_SCI_IRQ,	ACPI_SCI_IRQ },
	{ PIRQ_GPIO,	0x07,		0x07 },
	{ PIRQ_I2C0,	0x0a,		0x0a },
	{ PIRQ_I2C1,	0x0b,		0x0b },
	{ PIRQ_I2C2,	0x0e,		0x0c },
	{ PIRQ_I2C3,	0x0f,		0x0d },
	{ PIRQ_UART0,	0x04,		0x03 },
	{ PIRQ_UART1,	0x03,		0x0e },
	{ PIRQ_I2C4,	0x05,		0x16 },
	{ PIRQ_I2C5,	0x06,		0x17 },
	{ PIRQ_UART2,	0x04,		0x05 },
	{ PIRQ_UART3,	0x03,		0x0f },
	/* The MISC registers are not interrupt numbers */
	{ PIRQ_MISC,	0xfa,		0x00 },
	{ PIRQ_MISC0,	0x91,		0x00 },
	{ PIRQ_HPET_L,	0x00,		0x00 },
	{ PIRQ_HPET_H,	0x00,		0x00 },
};

const struct fch_irq_routing *mb_get_fch_irq_mapping(size_t *length)
{
	*length = ARRAY_SIZE(fch_irq_map);
	return fch_irq_map;
}

bool mainboard_dimm_slot_exists(uint8_t socket, uint8_t channel, uint8_t slot)
{
	/* All DIMM slots exist on socket 0. This is single socket platform */
	return (socket == 0);
}

void smbios_fill_dimm_locator(const struct dimm_info *dimm, struct smbios_type17 *t)
{
	char locator[40];

	/*
	 * Same names as the vendor BIOS, DIMM_P<socket>_<channel><dimm>:
	 * DIMM_P0_A0, DIMM_P0_A1, DIMM_P0_B0, ... on this single socket board.
	 * The BMC parses the locator to map each DIMM to its status sensor and
	 * only accepts "DIMM_P%d_%c%d", "DIMM %c%d" with a digit as channel,
	 * or "CPU%d_HBMIO%d". Anything else fails the whole DIMM inventory
	 * ("Error in Parse Dimm Status: Out Of Spec Define").
	 */
	snprintf(locator, sizeof(locator), "DIMM_P0_%c%u",
		'A' + dimm->channel_num, dimm->dimm_num);
	t->device_locator = smbios_add_string(t->eos, locator);

	snprintf(locator, sizeof(locator), "BANK %u", dimm->dimm_num);
	t->bank_locator = smbios_add_string(t->eos, locator);
}

static void mainboard_init(void *chip_info)
{
	struct memory_info *mem_info;

	/*
	 * Update maximum memory capacity and number of DIMM slots for SMBIOS
	 * type 16 for this board. Turin can support up to 6TB of memory on 24
	 * DIMMS per socket. This board populates all 12 channels with 2 DIMMs
	 * per channel on one socket. Both values must be set, otherwise
	 * smbios_write_type16() replaces them with the populated DIMMs. The
	 * edk2 payload derives the Gigabyte DIMM record (type 0x85) from them.
	 */
	mem_info = cbmem_find(CBMEM_ID_MEMINFO);
	if (mem_info) {
		mem_info->max_capacity_mib = MZ33_MAX_MEMORY_GIB * (GiB / MiB);
		mem_info->number_of_devices = CONFIG_DIMM_MAX;
	}
}

static void mainboard_enable(struct device *dev)
{
	if (CONFIG(GENERATE_SMBIOS_TABLES))
		dev->ops->get_smbios_data = mainboard_smbios_oem_data;
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = mainboard_enable,
};
