/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/acpi.h>
#include <amdblocks/amd_pci_util.h>
#include <commonlib/helpers.h>
#include <device/device.h>
#include <types.h>
#include "gpio.h"
#include "update_devicetree.h"

/* The IRQ mapping in fch_irq_map ends up getting written to the indirect address space that is
   accessed via I/O ports 0xc00/0xc01. */

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
	{ PIRQ_SDIO,	PIRQ_NC,	0x10 },
	{ PIRQ_GPIO,	0x07,		0x07 },
	{ PIRQ_I2C0,	0x0a,		0x0a },
	{ PIRQ_I2C1,	0x0b,		0x0b },
	{ PIRQ_I2C2,	0x0e,		0x0e },
	{ PIRQ_I2C3,	0x0f,		0x06 },
	{ PIRQ_UART0,	4,		0x03 },
	{ PIRQ_UART1,	3,		0x0e },
	{ PIRQ_UART2,	4,		0x05 },
	{ PIRQ_UART3,	3,		0x0f },
	{ PIRQ_UART4,	4,		0x10 },
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

static void mainboard_init(void *chip_info)
{
	mainboard_program_gpios();

	mainboard_update_devicetree_opensil();
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
};
