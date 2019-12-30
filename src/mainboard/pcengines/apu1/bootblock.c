/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <bootblock_common.h>
#include <device/pci_ops.h>
#include <device/pnp_type.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <superio/nuvoton/nct5104d/nct5104d.h>

#include "bios_knobs.h"

#define SIO_PORT 0x2e
#define SERIAL2_DEV PNP_DEV(SIO_PORT, NCT5104D_SP2)

void bootblock_mainboard_early_init(void)
{
	pci_devfn_t dev;
	u32 data;

	dev = PCI_DEV(0, 0x14, 3);
	data = pci_read_config32(dev, 0x48);
	/* enable 0x2e/0x4e IO decoding before configuring SuperIO */
	pci_write_config32(dev, 0x48, data | 3);

	if (check_com2() || (CONFIG_UART_FOR_CONSOLE == 1))
		nuvoton_enable_serial(SERIAL2_DEV, 0x2f8);

}
