/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
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

#include <arch/mmio.h>
#include <device/pci_ops.h>
#include <soc/gpio.h>
#include <soc/iomap.h>
#include <soc/lpc.h>
#include <soc/pci_devs.h>
#include <soc/pmc.h>
#include <soc/romstage.h>

#define PCH_ILB_IRQE 0x88

static unsigned int get_pch_stepping(void)
{
	uint8_t rev_id;

	rev_id = pci_read_config8 (PCI_DEV(0, LPC_DEV, 0), REVID);

	if (rev_id >= PCH_LPC_RID_13)
		return STEP_D1;
	else if (rev_id >= PCH_LPC_RID_11)
		return STEP_D0;
	else if (rev_id >= PCH_LPC_RID_11)
		return STEP_C0;
	else if (rev_id >= PCH_LPC_RID_11)
		return STEP_B3;
	else if (rev_id >= PCH_LPC_RID_11)
		return STEP_B2;
	else if (rev_id >= PCH_LPC_RID_11)
		return STEP_B1;
	else if (rev_id >= PCH_LPC_RID_11)
		return STEP_B0;
	else if (rev_id >= PCH_LPC_RID_11)
		return STEP_A1;
	else if (rev_id <= PCH_LPC_RID_2)
		return STEP_A0;

	return STEP_D0;
}

void byt_config_com1_and_enable(void)
{
	uint32_t reg32;
	uint8_t reg8;

	/* Enable COM1 for debug message output. */
	reg32 = read32((const volatile void *)(PMC_BASE_ADDRESS + GEN_PMCON1));
	reg32 &= ~(SUS_PWR_FLR | PWR_FLR);
	reg32 |= UART_EN;
	write32 ((volatile void *)(PMC_BASE_ADDRESS + GEN_PMCON1), reg32);

	reg8 = read8((const volatile void *)(ILB_BASE_ADDRESS + PCH_ILB_IRQE));

	if (get_pch_stepping()>= STEP_B0)
		reg8 |= BIT(4); // Enable IRQ4
	else
		reg8 |= BIT(3); // Enable IRQ3

	write8((volatile void *)(ILB_BASE_ADDRESS + PCH_ILB_IRQE), reg8);

	/* Set up the pads to select the UART function */
	score_set_internal_pull(UART_RXD_PAD, PAD_PU_2K | PAD_PULL_UP);
	score_select_func(UART_RXD_PAD, 1);
	score_select_func(UART_TXD_PAD, 1);

	/* Enable the legacy UART hardware. */
	pci_write_config32(PCI_DEV(0, LPC_DEV, 0), UART_CONT, 1);
}
