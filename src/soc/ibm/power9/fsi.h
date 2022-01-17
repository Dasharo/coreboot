/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_FSI_H
#define __SOC_IBM_POWER9_FSI_H

#include <stdbool.h>
#include <stdint.h>
#include <arch/byteorder.h>

#ifdef DEBUG_FSI
#include <console/console.h>
#endif

/*
 * Define DEBUG_FSI before including this header to get debug prints from this
 * unit (both FSI and CFAM)
 */

/* Base FSI address for registers of a FSI I2C master */
#define I2C_FSI_MASTER_BASE_ADDR 0x01800

/* Returns mask of available CPU chips (either 0x01 or 0x03) */
uint8_t fsi_init(void);

void fsi_i2c_init(uint8_t chips);

void fsi_reset_pib2opb(uint8_t chip);

/* This isn't meant to be used directly, see below for interface of this unit */
uint32_t fsi_op(uint8_t chip, uint32_t addr, uint32_t data, bool is_read, uint8_t size);

/* FSI-functions operate on byte addresses */

static inline uint32_t read_fsi(uint8_t chip, uint32_t addr)
{
	uint32_t data = fsi_op(chip, addr, /*data=*/0, /*is_read=*/true, /*size=*/4);
#ifdef DEBUG_FSI
	printk(BIOS_EMERG, "read_fsi(%d, 0x%08x) = 0x%08x\n", chip, addr, data);
#endif
	return data;
}

static inline void write_fsi(uint8_t chip, uint32_t addr, uint32_t data)
{
#ifdef DEBUG_FSI
	printk(BIOS_EMERG, "write_fsi(%d, 0x%08x) = 0x%08x\n", chip, addr, data);
#endif
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
	uint32_t data = read_fsi(chip, cfam_addr_to_fsi(addr));
#ifdef DEBUG_FSI
	printk(BIOS_EMERG, "read_cfam(%d, 0x%08x) = 0x%08x\n", chip, addr, data);
#endif
	return data;
}

static inline void write_cfam(uint8_t chip, uint32_t addr, uint32_t data)
{
#ifdef DEBUG_FSI
	printk(BIOS_EMERG, "write_cfam(%d, 0x%08x) = 0x%08x\n", chip, addr, data);
#endif
	write_fsi(chip, cfam_addr_to_fsi(addr), data);
}

/* Operations on FSI I2C registers */

static inline void write_fsi_i2c(uint8_t chip, uint8_t reg, uint32_t data, uint8_t size)
{
	uint32_t addr = I2C_FSI_MASTER_BASE_ADDR + reg * 4;
	fsi_op(chip, addr, data, /*is_read=*/false, size);
}

static inline uint32_t read_fsi_i2c(uint8_t chip, uint8_t reg, uint8_t size)
{
	uint32_t addr = I2C_FSI_MASTER_BASE_ADDR + reg * 4;
	return fsi_op(chip, addr, /*data=*/0, /*is_read=*/true, size);
}

#endif /* __SOC_IBM_POWER9_FSI_H */
