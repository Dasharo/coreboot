/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/gpio.h>
#include <gpio.h>
#include "gpio.h"

/* GPIO pins used by coreboot should be initialized in bootblock */

static const struct soc_amd_gpio gpio_set_stage_reset[] = {
	/* TPM CS */
	PAD_NF(GPIO_29, SPI_TPM_CS_L, PULL_UP),
	/* ESPI_CS_L */
	PAD_NF(GPIO_30, ESPI_CS_L, PULL_UP),
	/* ESPI_SOC_CLK */
	PAD_NF(GPIO_77, SPI1_CLK, PULL_DOWN),
	/* ESPI_DATA0 */
	PAD_NF(GPIO_81, SPI1_DAT0, PULL_UP),
	/* ESPI_DATA1 */
	PAD_NF(GPIO_80, SPI1_DAT1, PULL_UP),
	/* ESPI_DATA2 */
	PAD_NF(GPIO_68, SPI1_DAT2, PULL_UP),
	/* ESPI_DATA3 */
	PAD_NF(GPIO_69, SPI1_DAT3, PULL_UP),
	/* ESPI_ALERT_L */
	PAD_NF(GPIO_22, ESPI_ALERT_D1, PULL_UP),
	/* ESPI_RESET_L */
	PAD_NF(GPIO_21, ESPI_RESET_L, PULL_UP),
	/* SPI_ROM_REQ */
	PAD_NF(GPIO_67, SPI_ROM_REQ, PULL_UP),
	/* SPI_ROM_GNT */
	PAD_NF(GPIO_76, SPI_ROM_GNT, PULL_DOWN),
	/* SPI1_CS3_L */
	PAD_NF(GPIO_79, SPI1_CS3_L, PULL_UP),

	/* Deassert PCIe Reset lines */
	/* PCIE_RST0_L */
	PAD_NFO(GPIO_26, PCIE_RST0_L, HIGH),

	/* I2C0 SCL */
	PAD_NF(GPIO_145, I2C0_SCL, PULL_NONE),
	/* I2C0 SDA */
	PAD_NF(GPIO_146, I2C0_SDA, PULL_NONE),
	/* I2C1 SCL */
	PAD_NF(GPIO_147, I2C1_SCL, PULL_NONE),
	/* I2C1 SDA */
	PAD_NF(GPIO_148, I2C1_SDA, PULL_NONE),
	/* SMBUS0_SCL */
	PAD_NF(GPIO_113, SMBUS0_SCL, PULL_NONE),
	/* SMBUS0_SDA */
	PAD_NF(GPIO_114, SMBUS0_SDA, PULL_NONE),
	/* SMBUS1_SCL */
	PAD_NF(GPIO_19, SMBUS1_SCL, PULL_NONE),
	/* SMBUS1_SDA */
	PAD_NF(GPIO_20, SMBUS1_SDA, PULL_NONE),
};

void mainboard_program_early_gpios(void)
{
	gpio_configure_pads(gpio_set_stage_reset, ARRAY_SIZE(gpio_set_stage_reset));
}
