/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/lpc.h>
#include <amdblocks/spi.h>

void soc_lpc_tpm_decode_spi(void)
{
	/* SoC-specific SPI TPM setting */
	spi_write32(SPI_CNTRL0, spi_read32(SPI_CNTRL0) | (1 << 13));
	spi_write32(SPI100_HOST_PREF_CONFIG, spi_read32(SPI100_HOST_PREF_CONFIG) | (1 << 25));
}
