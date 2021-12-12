/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_FSI_H
#define __SOC_IBM_POWER9_FSI_H

#include <stdbool.h>
#include <stdint.h>

void fsi_init(void);

/* This isn't meant to be used directly, see below for interface of this unit */
uint32_t fsi_op(uint8_t chip, uint32_t addr, uint32_t data, bool is_read, uint8_t size);

/* FSI-functions operate on byte addresses */

static inline uint32_t read_fsi(uint8_t chip, uint32_t addr)
{
	return fsi_op(chip, addr, /*data=*/0, /*is_read=*/true, /*size=*/4);
}

static inline void write_fsi(uint8_t chip, uint32_t addr, uint32_t data)
{
	(void)fsi_op(chip, addr, data, /*is_read=*/false, /*size=*/4);
}

/* CFAM-functions are FSI-functions that operate on 4-byte word addresses */

static inline uint32_t cfam_addr_to_fsi(uint32_t cfam)
{
	/*
	 * Such masks allow overlapping of two components after address
	 * translation (real engine mask is probably 0xF100), but let's be in
	 * sync with Hostboot on this to play it safe.
	 */
	const uint32_t CFAM_ADDRESS_MASK = 0x1FF;
	const uint32_t CFAM_ENGINE_OFFSET_MASK = 0xFE00;

	/*
	 * Address needs to be multiplied by 4 because CFAM register addresses
	 * are word offsets but FSI addresses are byte offsets. Address
	 * modification needs to preserve the engine's offset in the top byte.
	 */
	return ((cfam & CFAM_ADDRESS_MASK) * 4) | (cfam & CFAM_ENGINE_OFFSET_MASK);
}

static inline uint32_t read_cfam(uint8_t chip, uint32_t addr)
{
	return read_fsi(chip, cfam_addr_to_fsi(addr));
}

static inline void write_cfam(uint8_t chip, uint32_t addr, uint32_t data)
{
	write_fsi(chip, cfam_addr_to_fsi(addr), data);
}

#endif /* __SOC_IBM_POWER9_FSI_H */
