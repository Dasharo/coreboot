/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/espi.h>
#include <amdblocks/lpc.h>
#include <bootblock_common.h>
#include <device/mmio.h>
#include <device/pnp_ops.h>
#include <soc/espi.h>
#include <soc/amd/common/block/lpc/espi_def.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <superio/nuvoton/nct6687d/nct6687d.h>
#include <superio/nuvoton/nct6687d/nct6687d_ec.h>
#include "gpio.h"

#define GPIO_DEV PNP_DEV(0x4e, NCT6687D_GPIO_0_7)
#define SERIAL_DEV PNP_DEV(0x4e, NCT6687D_SP1)
#define POWER_DEV PNP_DEV(0x4e, NCT6687D_SLEEP_PWR)
#define P80_UART_DEV PNP_DEV(0x4e, NCT6687D_P80_UART)
#define EC_DEV PNP_DEV(0x4e, NCT6687D_EC)

#define EC_IO_BASE 0xa20

void mb_set_up_early_espi(void)
{
	espi_switch_to_spi1_pads();
	mainboard_program_early_gpios();
}

static void post_espi_init(void)
{
	volatile void *espi_base = (void *)lpc_get_spibase() + ESPI_OFFSET_FROM_BAR;
	uint32_t value;

	/* Initialzie eSPI watchdog timer */
	value = read32(espi_base + ESPI_GLOBAL_CONTROL_0);
	value &= ~(ESPI_WDG_CNT_MASK | ESPI_AL_IDLE_TIMER_MASK);
	value |= (ESPI_AL_IDLE_TIMER_MASK | (0x1400 << ESPI_WDG_CNT_SHIFT));
	write32(espi_base + ESPI_GLOBAL_CONTROL_0, value);

	write32(espi_base + ESPI_GLOBAL_CONTROL_0,
		read32(espi_base + ESPI_GLOBAL_CONTROL_0) | ESPI_WDG_EN);
	write32(espi_base + ESPI_GLOBAL_CONTROL_0,
		read32(espi_base + ESPI_GLOBAL_CONTROL_0) | ESPI_WAIT_CHKEN);

	value = read32(espi_base + ESPI_GLOBAL_CONTROL_0);
	value &= ~ESPI_WAIT_CNT_MASK;
	value |= ESPI_WAIT_CNT_MASK;
	value |= (1 << 31); // set reserved bit as vendor BIOS does
	write32(espi_base + ESPI_GLOBAL_CONTROL_0, value);

	/* Program RX Virtual Wires */
	write32(espi_base + ESPI_RXVW_INDEX, 0x00040506);
	write32(espi_base + ESPI_RXVW_MISC_CNTL, 0x00000007);

	value = read32(espi_base + ESPI_GLOBAL_CONTROL_1);
	value |= ESPI_REQ_NOTWITH_VW_REQ;
	write32(espi_base + ESPI_GLOBAL_CONTROL_1, value);

	/* Clear and enable interrupts */
	value = read32(espi_base + ESPI_SLAVE0_INT_STS);
	if (value)
		write32(espi_base + ESPI_SLAVE0_INT_STS, value);

	write32(espi_base + ESPI_SLAVE0_INT_EN, UINT32_MAX);
}

void bootblock_mainboard_early_init(void)
{
	post_espi_init();

	/* Replicate vendor settings for multi-function pins in global config LDN */
	nuvoton_pnp_enter_conf_state(GPIO_DEV);
	pnp_write_config(GPIO_DEV, 0x10, 0xff); // IRQ8-15 level triggered, low
	pnp_write_config(GPIO_DEV, 0x11, 0xff); // IRQ0-7 level triggered, low
	pnp_write_config(GPIO_DEV, 0x13, 0xff); // IRQ8-15 level triggered, low
	pnp_write_config(GPIO_DEV, 0x14, 0xff); // IRQ0-7 level triggered, low

	/* Below are multi-pin function */
	pnp_write_config(GPIO_DEV, 0x15, 0x0a);
	pnp_write_config(GPIO_DEV, 0x1a, 0x82);
	pnp_write_config(GPIO_DEV, 0x1b, 0x02);
	pnp_write_config(GPIO_DEV, 0x1d, 0x00);
	pnp_write_config(GPIO_DEV, 0x1e, 0xa2);
	pnp_write_config(GPIO_DEV, 0x1f, 0x02);
	pnp_write_config(GPIO_DEV, 0x22, 0xbc);
	pnp_write_config(GPIO_DEV, 0x23, 0xdf);
	pnp_write_config(GPIO_DEV, 0x24, 0x39);
	pnp_write_config(GPIO_DEV, 0x25, 0xfe);
	pnp_write_config(GPIO_DEV, 0x26, 0xc0);
	pnp_write_config(GPIO_DEV, 0x27, 0x12);
	pnp_write_config(GPIO_DEV, 0x28, 0x40);
	pnp_write_config(GPIO_DEV, 0x29, 0xff);
	pnp_write_config(GPIO_DEV, 0x2a, 0x8f);
	pnp_write_config(GPIO_DEV, 0x2b, 0x28);
	pnp_write_config(GPIO_DEV, 0x2c, 0x80);
	pnp_write_config(GPIO_DEV, 0x2d, 0x00);

	pnp_set_logical_device(POWER_DEV);
	/* Configure pins for AMD SB-TSI, pin 70 as GPIO66 */
	pnp_write_config(POWER_DEV, 0xf3, 0x99);

	/* Configure V_COMPn detection */
	pnp_unset_and_set_config(POWER_DEV, 0xf0, 0xf2, 0x70);

	/* Configure yellow and green LED */
	pnp_write_config(POWER_DEV, 0xe7, 0xa0);
	pnp_write_config(POWER_DEV, 0xe8, 0x07);
	pnp_set_logical_device(P80_UART_DEV);
	pnp_write_config(P80_UART_DEV, 0xe5, 0x0f);

	/* Configure EC */
	pnp_set_logical_device(EC_DEV);
	pnp_set_iobase(EC_DEV, PNP_IDX_IO0, EC_IO_BASE);
	pnp_set_enable(EC_DEV, 1);

	nct6687d_ec_and_or_page(EC_IO_BASE, 0, 0x34, 0xc0, 0x00);
	nct6687d_ec_and_or_page_ff(EC_IO_BASE, 8, 0x00, 0xff, 0x01);

	nuvoton_pnp_exit_conf_state(EC_DEV);

	if (CONFIG(CONSOLE_SERIAL))
		nuvoton_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
}

void bootblock_mainboard_init(void)
{
}
