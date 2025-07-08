/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/acpi.h>
#include <amdblocks/amd_pci_util.h>
#include <cbmem.h>
#include <drivers/amd/opensil/opensil.h>
#include <smbios.h>
#include <soc/amd_pci_int_defs.h>
#include <stdio.h>
#include <types.h>

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
	{ PIRQ_A,	10,		0x10 },
	{ PIRQ_B,	11,		0x11 },
	{ PIRQ_C,	11,		0x12 },
	{ PIRQ_D,	10,		0x13 },
	{ PIRQ_E,	10,		0x14 },
	{ PIRQ_F,	11,		0x15 },
	{ PIRQ_G,	11,		0x16 },
	{ PIRQ_H,	10,		0x17 },
	{ PIRQ_SCI,	ACPI_SCI_IRQ,	ACPI_SCI_IRQ },
	{ PIRQ_SD,	PIRQ_NC,	0x10 },
	{ PIRQ_SDIO,	PIRQ_NC,	0x10 },
	{ PIRQ_GPIO,	PIRQ_NC,	0x07 },
	{ PIRQ_I2C0,	PIRQ_NC,	0x0a },
	{ PIRQ_I2C1,	PIRQ_NC,	0x0b },
	{ PIRQ_I2C2,	PIRQ_NC,	0x04 },
	{ PIRQ_I2C3,	PIRQ_NC,	0x06 },
	{ PIRQ_UART0,	4,		0x03 },
	{ PIRQ_UART1,	3,		0x04 },
	{ PIRQ_I2C4,	PIRQ_NC,	0x0e },
	{ PIRQ_I2C5,	PIRQ_NC,	0x0f },
	{ PIRQ_UART2,	4,		0x03 },
	{ PIRQ_UART3,	3,		0x04 },
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

void smbios_fill_dimm_locator(const struct dimm_info *dimm, struct smbios_type17 *t)
{
	char locator[40];

	/*
	 * DIMM slots are anemd by channel number and dimm number:
	 * Channels: A, B, C, D, etc.
	 * DIMMs: A0, A1, B0, B1, etc.
	 */
	snprintf(locator, sizeof(locator), "DIMM %c%u",
		'A' + dimm->channel_num, dimm->dimm_num);
	t->device_locator = smbios_add_string(t->eos, locator);

	snprintf(locator, sizeof(locator), "BANK %d", dimm->bank_locator);
	t->bank_locator = smbios_add_string(t->eos, locator);
}

static void mainboard_init(void *chip_info)
{
	struct memory_info *mem_info;

	/*
	 * Update maximum memory capacity for SMBIOS type 16 for this board.
	 * Turin can support up to 6TB of memory on 24 DIMMS per socket. This
	 * board populates all 12 channels with 2 DIMMs per channel on one
	 * socket.
	 */
	mem_info = cbmem_find(CBMEM_ID_MEMINFO);
	if (mem_info)
		mem_info->max_capacity_mib = 6 * 1024 * (GiB / MiB);
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
};
