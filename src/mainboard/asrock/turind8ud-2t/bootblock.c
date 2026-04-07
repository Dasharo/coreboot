/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/espi.h>
#include <amdblocks/lpc.h>
#include <amdblocks/spi.h>
#include <bootblock_common.h>
#include <device/pnp_type.h>
#include <gpio.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <superio/nuvoton/nct6791d/nct6791d.h>
#include <superio/aspeed/ast2400/ast2400.h>
#include <superio/aspeed/common/aspeed.h>
#include <device/pnp_ops.h>

void bootblock_mainboard_early_init(void)
{
	/* Configure appropriate physical port of SuperIO chip off BMC */
	const pnp_devfn_t serial1_dev = PNP_DEV(0x4e, AST2400_SUART1);
	const pnp_devfn_t serial2_dev = PNP_DEV(0x4e, AST2400_SUART2);

	/*
	 * APCBs are configured to enable 0x3f8 range already.
	 * Eable only post codes here.
	 */
	espi_open_io_window(0x80, 1);

	/*
	 * Disable the Nuvoton NCT6791D SuperIO UART1.  It is enabled by
	 * default, but the AST2600's is connected to the serial port.
	 */
	const pnp_devfn_t nvt_serial_dev = PNP_DEV(0x2E, NCT6791D_SP1);
	nuvoton_pnp_enter_conf_state(nvt_serial_dev);
	pnp_set_logical_device(nvt_serial_dev);
	pnp_set_enable(nvt_serial_dev, 0);
	nuvoton_pnp_exit_conf_state(nvt_serial_dev);

	/* Enable AST2600 SuperIO UART1 and UART2 */
	aspeed_enable_serial(serial1_dev, 0x3f8);
	aspeed_enable_serial(serial2_dev, 0x2f8);
	/* Enable UART function pin */
	aspeed_enable_uart_pin(serial1_dev);
}

static const struct soc_amd_gpio gpio_table[] = {
	PAD_GPO(GPIO_3, LOW),
	PAD_NFO(GPIO_4, SATA_ACT_L, LOW),
	PAD_GPI(GPIO_5, PULL_DOWN),
	PAD_GPI(GPIO_6, PULL_DOWN),
	PAD_NF(GPIO_76, SPI_TPM_CS_L, PULL_UP),
	PAD_GPI(GPIO_89, PULL_UP),
	PAD_NF(GPIO_89, PM_INTR_L, PULL_NONE),
	PAD_GPO(GPIO_115, LOW),
	PAD_GPO(GPIO_116, LOW),
};

void bootblock_mainboard_init(void)
{
	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
	lpc_tpm_decode_spi();
}
