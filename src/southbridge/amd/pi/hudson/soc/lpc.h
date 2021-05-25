/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_LPC_H
#define SOC_LPC_H

#define SPIROM_BASE_ADDRESS_REGISTER	0xa0
#define   SPI_BASE_ALIGNMENT		BIT(6)
#define   SPI_BASE_RESERVED		(BIT(0) | BIT(2)  | BIT(4) | BIT(5))
#define   ROUTE_TPM_2_SPI		BIT(3)
#define   SPI_ROM_ENABLE		BIT(1)
#define   SPI_PRESERVE_BITS		(BIT(1) | BIT(3))

#endif /* SOC_LPC_H */
