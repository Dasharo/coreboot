/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <device/pnp_ops.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <superio/nuvoton/nct6687d/nct6687d.h>

#define SERIAL_DEV PNP_DEV(0x4e, NCT6687D_SP1)

void bootblock_mainboard_early_init(void)
{
	/* Replicate vendor settings for multi-function pins in global config LDN */
	nuvoton_pnp_enter_conf_state(SERIAL_DEV);
	pnp_write_config(0x15, 0xaa);
	pnp_write_config(0x1a, 0x02);
	pnp_write_config(0x1b, 0x02);
	pnp_write_config(0x1d, 0x00);
	pnp_write_config(0x1e, 0xaa);
	pnp_write_config(0x1f, 0xb2);
	pnp_write_config(0x22, 0xbd);
	pnp_write_config(0x23, 0xdf);
	pnp_write_config(0x24, 0x39);
	pnp_write_config(0x25, 0xfe);
	pnp_write_config(0x26, 0x40);
	pnp_write_config(0x27, 0x77);
	pnp_write_config(0x28, 0x00);
	pnp_write_config(0x29, 0xfb);
	pnp_write_config(0x2a, 0x80);
	pnp_write_config(0x2b, 0x20);
	pnp_write_config(0x2c, 0x8a);
	pnp_write_config(0x2d, 0xaa);
	nuvoton_pnp_exit_conf_state(SERIAL_DEV);

	if (CONFIG(CONSOLE_SERIAL))
		nuvoton_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
}
