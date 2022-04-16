/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_9.h>

#include <console/console.h>
#include <cpu/power/scom.h>
#include <timer.h>

#include "xbus.h"

static void tx_serializer_sync_power_on(uint8_t master_chip, uint8_t slave_chip, int group)
{
	const uint64_t offset = group * XBUS_LINK_GROUP_OFFSET;

	/*
	 * EDIP_TX_CLK_UNLOAD_CLK_DISABLE
	 * (set to 0 to clock off sync logic on the clock slice and save power;
	 *  it should not be necessary to use the sync logic on the clock slice
	 *  since it has no fifo but control is available just in case)
	 */
	scom_and(master_chip, xbus_addr(0x800C1C0006010C3F + offset), ~PPC_BIT(50));
	scom_and(slave_chip, xbus_addr(0x800C1C0006010C3F + offset), ~PPC_BIT(50));

	/*
	 * EDIP_TX_CLK_RUN_COUNT
	 * (set to 1 to enable the tx clock slice serializer; this should be
	 *  enabled at all times but control is available just in case)
	 */
	scom_and(master_chip, xbus_addr(0x800C1C0006010C3F + offset), ~PPC_BIT(51));
	scom_and(slave_chip, xbus_addr(0x800C1C0006010C3F + offset), ~PPC_BIT(51));

	/* EDIP_TX_CLK_RUN_COUNT (see above) */
	scom_or(master_chip, xbus_addr(0x800C1C0006010C3F + offset), PPC_BIT(51));
	scom_or(slave_chip, xbus_addr(0x800C1C0006010C3F + offset), PPC_BIT(51));

	/* EDIP_TX_CLK_UNLOAD_CLK_DISABLE (see above) */
	scom_or(master_chip, xbus_addr(0x800C1C0006010C3F + offset), PPC_BIT(50));
	scom_or(slave_chip, xbus_addr(0x800C1C0006010C3F + offset), PPC_BIT(50));

	for (int i = 0; i < XBUS_LANE_COUNT; ++i) {
		uint64_t lane_offset = PPC_PLACE(i, 27, 5);
		/*
		 * EDIP_TX_UNLOAD_CLK_DISABLE
		 * (set to 0 to enable sync of tx custom serializer via tx_fifo_init register,
		 *  set to 1 to clock off sync logic and save power)
		 */
		scom_and(master_chip, xbus_addr(0x80040C0006010C3F | offset | lane_offset),
			 ~PPC_BIT(56));
		scom_and(slave_chip, xbus_addr(0x80040C0006010C3F | offset | lane_offset),
			 ~PPC_BIT(56));
	}
}

static void xbus_linktrain(uint8_t master_chip, uint8_t slave_chip, int group)
{
	enum {
		/* I/O EDI+ Training Substeps */
		NONE       = 0x00000000,
		WIRETEST   = 0x00000001,
		DESKEW     = 0x00000002,
		EYEOPT     = 0x00000004,
		REPAIR     = 0x00000008,
		FUNCTIONAL = 0x00000010,
		WDERF      = 0x0000001F, // all of the above
	};

	const uint64_t offset = group * XBUS_LINK_GROUP_OFFSET;

	uint64_t tmp;

	/* Hostboot collects bad lane information here, we don't */

	/*
	 * Clock Serializer Init
	 * Isn't strictly necessary but does line up the clock serializer
	 * counter with the data slices.
	 */
	tx_serializer_sync_power_on(master_chip, slave_chip, group);

	/* Start Slave/Master Target Link Training */

	/*
	 * EDIP_RX_START_WDERF_ALIAS (alias for rx_start_* bits)
	 * Slave training must start first.
	 */
	scom_and_or(slave_chip, xbus_addr(0x8009F00006010C3F + offset),
		    ~PPC_BITMASK(48, 52), PPC_PLACE(WDERF, 48, 5));
	scom_and_or(master_chip, xbus_addr(0x8009F00006010C3F + offset),
		    ~PPC_BITMASK(48, 52), PPC_PLACE(WDERF, 48, 5));

	/*
	 * 48-52  EDIP_RX_WDERF_DONE_ALIAS (alias for rx_*_done bits)
	 * 56-60  EDIP_RX_WDERF_FAILED_ALIAS (alias for rx_*_failed bits)
	 */
	wait_ms(100 * 1,
		(tmp = read_scom(master_chip, xbus_addr(0x800A380006010C3F + offset)),
		 (((tmp >> 11) & 0x1F) == WDERF || ((tmp >> 3) & 0x1F) != 0)));
	if (((tmp >> 3) & 0x1F) != 0)
		die("I/O EDI+ Xbus link training failed.\n");
	if (((tmp >> 11) & 0x1F) != WDERF)
		die("I/O EDI+ Xbus link training timeout.\n");

}

void istep_9_4(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 9.4\n");
	report_istep(9,4);

	if (chips != 0x01) {
		xbus_linktrain(/*master_chip=*/0, /*slave_chip=*/1, /*group=*/0);
		xbus_linktrain(/*master_chip=*/0, /*slave_chip=*/1, /*group=*/1);
	}

	printk(BIOS_EMERG, "ending istep 9.4\n");
}
