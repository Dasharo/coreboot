/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/spi.h>
#include <amdblocks/lpc.h>
#include <bootblock_common.h>
#include <gpio.h>

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
