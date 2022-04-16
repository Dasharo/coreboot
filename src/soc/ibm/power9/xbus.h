/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_XBUS_H
#define __SOC_IBM_POWER9_XBUS_H

#include <cpu/power/scom.h>
#include <stdint.h>

#define XBUS_LANE_COUNT 17

#define XBUS_LINK_GROUP_OFFSET 0x2000000000

/* Updates address that targets XBus chiplet to use a specific XBus link number.
 * Does nothing to non-XBus addresses. */
static inline uint64_t xbus_addr(uint64_t addr)
{
	enum {
		XBUS_COUNT = 0x3,		// number of XBus links
		XBUS_LINK = 0x1,		// hard-coded link number
		XB_IOX_0_RING_ID = 0x3,		// IOX_0
		XB_PBIOX_0_RING_ID = 0x6,	// PBIOX_0
	};

	uint8_t ring = (addr >> 10) & 0xF;
	uint8_t chiplet = (addr >> 24) & 0x3F;

	if (chiplet != XB_CHIPLET_ID)
		return addr;

	if (ring >= XB_IOX_0_RING_ID && ring < XB_IOX_0_RING_ID + XBUS_COUNT)
		PPC_INSERT(addr, XB_IOX_0_RING_ID + XBUS_LINK, 50, 4);
	else if (ring >= XB_PBIOX_0_RING_ID && ring < XB_PBIOX_0_RING_ID + XBUS_COUNT)
		PPC_INSERT(addr, XB_PBIOX_0_RING_ID + XBUS_LINK, 50, 4);

	return addr;
}

#endif /* __SOC_IBM_POWER9_XBUS_H */
