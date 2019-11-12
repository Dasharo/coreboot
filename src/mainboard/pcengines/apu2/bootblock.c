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
#include <device/pnp_type.h>
#include <southbridge/amd/pi/hudson/hudson.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <superio/nuvoton/nct5104d/nct5104d.h>

#include "bios_knobs.h"

#define SIO_PORT 0x2e
#define SERIAL1_DEV PNP_DEV(SIO_PORT, NCT5104D_SP1)
#define SERIAL2_DEV PNP_DEV(SIO_PORT, NCT5104D_SP2)

void bootblock_mainboard_early_init(void)
{
	hudson_lpc_port80();
	hudson_clk_output_48Mhz();

	/* Enable UARTB for LPC MCU */
	if (CONFIG(BOARD_PCENGINES_APU5))
		nuvoton_enable_serial(SERIAL2_DEV, 0x2f8);

	if ((check_com2() || (CONFIG_UART_FOR_CONSOLE == 1)))
		nuvoton_enable_serial(SERIAL2_DEV, 0x2f8);
}
