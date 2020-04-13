/* SPDX-License-Identifier: GPL-2.0-only */
<<<<<<< HEAD
=======
/* This file is part of the coreboot project. */
>>>>>>> mb/dell/optiplex_9010: Add Dell OptiPlex 9010 SFF support

#include <bootblock_common.h>
#include <device/pci_ops.h>
#include <northbridge/intel/sandybridge/sandybridge.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <superio/smsc/sch5545/sch5545.h>
#include <superio/smsc/sch5545/sch5545_emi.h>

#include "sch5545_ec.h"

const struct southbridge_usb_port mainboard_usb_ports[] = {
	{ 1, 6, 0 },
	{ 1, 6, 0 },
	{ 1, 1, 1 },
	{ 1, 1, 1 },
	{ 1, 1, 2 },
	{ 1, 1, 2 },
	{ 1, 6, 3 },
	{ 1, 6, 3 },
	{ 1, 6, 4 },
	{ 1, 6, 4 },
	{ 1, 6, 5 },
	{ 1, 1, 5 },
	{ 1, 1, 6 },
	{ 1, 6, 6 },
};

void bootblock_mainboard_early_init(void)
{
	/*
	 * FIXME: the board gets stuck in reset loop in
	 * mainboard_romstage_entry. Avoid that by clearing SSKPD
	 */
	pci_write_config32(HOST_BRIDGE, MCHBAR, (uintptr_t)DEFAULT_MCHBAR | 1);
	pci_write_config32(HOST_BRIDGE, MCHBAR + 4, (0LL + (uintptr_t)DEFAULT_MCHBAR) >> 32);
	MCHBAR16(SSKPD_HI) = 0;

	sch5545_early_init(0x2e);
	/* Bare EC and SIO GPIO initialization which allows to enable serial port */
	sch5545_emi_init(0x2e);
	sch5545_emi_disable_interrupts();
	sch5545_ec_early_init();

	if (CONFIG(CONSOLE_SERIAL))
		sch5545_enable_uart(0x2e, 0);
}
