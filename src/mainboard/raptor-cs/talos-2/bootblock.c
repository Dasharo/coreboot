/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <program_loading.h>
#include <superio/aspeed/common/aspeed.h>
#include <superio/aspeed/ast2400/ast2400.h>
#include <console/uart.h>
#include <bootblock_common.h>

void bootblock_mainboard_early_init(void)
{
	struct config_data vuart_init[] = {
		/* Activate VUART */
		[Step1] = {
			.type = SIO,
			.base = 0,
			.reg = ACT_REG,
			.and = 0,
			.or = ACTIVATE_VALUE
		},
		/* Configure IRQ level as low */
		[Step2] = {
			.type = MEM,
			.base = ASPEED_VUART_BASE,
			.reg = 0x20,
			.and = ~2,
			.or = 0
		},
		/* Set the SerIRQ IRQ 4 */
		[Step3] = {
			.type = MEM,
			.base = ASPEED_VUART_BASE,
			.reg = 0x24,
			.and = ~0xf0,
			.or = (4 << 4)
		},
		/* Configure base address low */
		[Step4] = {
			.type = MEM,
			.base = ASPEED_VUART_BASE,
			.reg = 0x28,
			.and = 0,
			.or = CONFIG_TTYS0_BASE & 0xff
		},
		/* Configure base address high */
		[Step5] = {
			.type = MEM,
			.base = ASPEED_VUART_BASE,
			.reg = 0x2c,
			.and = 0,
			.or = (CONFIG_TTYS0_BASE >> 8) & 0xff
		},
	};

	const pnp_devfn_t serial_dev = PNP_DEV(0x2e, AST2400_ILPC2AHB);
	aspeed_early_config(serial_dev, vuart_init, ARRAY_SIZE(vuart_init));
}
