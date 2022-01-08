/* SPDX-License-Identifier: GPL-2.0-only */

#include "xbus.h"

#include <cpu/power/scom.h>
#include <stdint.h>

#include "sbeio.h"

/* Updates address that targets XBus chiplet to use specified XBus link number.
 * Does nothing to non-XBus addresses. */
static uint64_t xbus_addr(uint8_t xbus, uint64_t addr)
{
	enum {
		XBUS_COUNT = 0x3,		// number of XBus links
		XB_IOX_0_RING_ID = 0x3,		// IOX_0
		XB_PBIOX_0_RING_ID = 0x6,	// PBIOX_0
	};

	uint8_t ring = (addr >> 10) & 0xF;
	uint8_t chiplet = (addr >> 24) & 0x3F;

	if (chiplet != XB_CHIPLET_ID)
		return addr;

	if (ring >= XB_IOX_0_RING_ID && ring < XB_IOX_0_RING_ID + XBUS_COUNT)
		ring = XB_IOX_0_RING_ID + xbus;
	else if (ring >= XB_PBIOX_0_RING_ID && ring < XB_PBIOX_0_RING_ID + XBUS_COUNT)
		ring = XB_PBIOX_0_RING_ID + xbus;

	addr &= ~PPC_BITMASK(50, 53);
	addr |= PPC_PLACE(ring, 50, 4);

	return addr;
}

void put_scom(uint8_t chip, uint64_t addr, uint64_t data)
{
	addr = xbus_addr(/*xbus=*/1, addr);

	if (chip == 0)
		write_scom(addr, data);
	else
		write_sbe_scom(chip, addr, data);
}

uint64_t get_scom(uint8_t chip, uint64_t addr)
{
	addr = xbus_addr(/*xbus=*/1, addr);

	if (chip == 0)
		return read_scom(addr);
	else
		return read_sbe_scom(chip, addr);
}
