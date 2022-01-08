/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_XBUS_H
#define __SOC_IBM_POWER9_XBUS_H

#include <stdint.h>

/*
 * Define DEBUG_XBUS before including this header to get debug prints from this
 * unit
 */

#define XBUS_LANE_COUNT 17

#define XBUS_LINK_GROUP_OFFSET 0x2000000000

/*
 * The API below is meant to be used after SBE for the second CPU is up (so
 * after istep 8.4), but prior to XSCOM working for it, which covers range of
 * isteps that initialize XBus and SMP.
 *
 * The functions use XSCOM for the first CPU and SBE IO for the second one. When
 * SCOM address targets XBus chiplet, ring part of the address is updated to
 * XBus link #1 if necessary (addresses in code use link #0, which also matches
 * Hostboot logs).
 *
 * No need to use this interface once Powerbus is activated (after istep 10.1)
 * and XSCOM can access SCOMs on both CPUs.
 */

void put_scom(uint8_t chip, uint64_t addr, uint64_t data);
uint64_t get_scom(uint8_t chip, uint64_t addr);

#ifdef DEBUG_XBUS
#include <console/console.h>

#define put_scom(c, x, y)                                                      \
({                                                                             \
	uint8_t __cw = c;                                                      \
	uint64_t __xw = x;                                                     \
	uint64_t __yw = y;                                                     \
	printk(BIOS_EMERG, "PUTSCOM %d %016llX %016llX\n", __cw, __xw, __yw);  \
	put_scom(__cw, __xw, __yw);                                            \
})

#define get_scom(c, x)                                                         \
({                                                                             \
	uint8_t __cr  = c;                                                     \
	uint64_t __xr = x;                                                     \
	uint64_t __yr = get_scom(__cr, __xr);                                  \
	printk(BIOS_EMERG, "GETSCOM %d %016llX %016llX\n", __cr, __xr, __yr);  \
	__yr;                                                                  \
})

#endif

static inline void and_scom(uint8_t chip, uint64_t addr, uint64_t mask)
{
	put_scom(chip, addr, get_scom(chip, addr) & mask);
}

static inline void or_scom(uint8_t chip, uint64_t addr, uint64_t mask)
{
	put_scom(chip, addr, get_scom(chip, addr) | mask);
}

static inline void and_or_scom(uint8_t chip, uint64_t addr, uint64_t and, uint64_t or)
{
	uint64_t data = get_scom(chip, addr);
	data &= and;
	data |= or;
	put_scom(chip, addr, data);
}

#endif /* __SOC_IBM_POWER9_XBUS_H */
