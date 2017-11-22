/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arch/io.h>
#include <console/console.h>
#include <spi_flash.h>
#include <spi-generic.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>

#include <Proc/Fch/FchPlatform.h>

#define SPI_REG_OPCODE		0x0
#define SPI_REG_CNTRL02		0x2
 #define CNTRL02_FIFO_RESET	(1 << 4)
 #define CNTRL02_EXEC_OPCODE	(1 << 0)
#define SPI_REG_CNTRL03		0x3
 #define CNTRL03_SPIBUSY	(1 << 7)
#define SPI_REG_FIFO		0xc
#define SPI_REG_CNTRL11		0xd
 #define CNTRL11_FIFOPTR_MASK	0x07
#define SPI_EXT_REG_INDX	0x1e
 #define SPI_EXT_REG_TXCOUNT	0x5
 #define SPI_EXT_REG_RXCOUNT	0x6
#define SPI_EXT_REG_DATA	0x1f

#if IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_AGESA_YANGTZE)
#define AMD_SB_SPI_TX_LEN	64
#else
#define AMD_SB_SPI_TX_LEN	8
#endif

static uintptr_t spibar;

static inline uint8_t spi_read(uint8_t reg)
{
	return read8((void *)(spibar + reg));
}

static inline void spi_write(uint8_t reg, uint8_t val)
{
	write8((void *)(spibar + reg), val);
}

static void reset_internal_fifo_pointer(void)
{
	uint8_t reg8;

	do {
		reg8 = spi_read(SPI_REG_CNTRL02);
		reg8 |= CNTRL02_FIFO_RESET;
		spi_write(SPI_REG_CNTRL02, reg8);
	} while (spi_read(SPI_REG_CNTRL11) & CNTRL11_FIFOPTR_MASK);
}

static void execute_command(void)
{
	uint8_t reg8;

	reg8 = spi_read(SPI_REG_CNTRL02);
	reg8 |= CNTRL02_EXEC_OPCODE;
	spi_write(SPI_REG_CNTRL02, reg8);

	while ((spi_read(SPI_REG_CNTRL02) & CNTRL02_EXEC_OPCODE) &&
	       (spi_read(SPI_REG_CNTRL03) & CNTRL03_SPIBUSY));
}

void spi_init(void)
{
	device_t dev;

	dev = dev_find_slot(0, PCI_DEVFN(0x14, 3));
	spibar = pci_read_config32(dev, 0xA0) & ~0x1F;
}

unsigned int spi_crop_chunk(unsigned int cmd_len, unsigned int buf_len)
{
	return min(AMD_SB_SPI_TX_LEN - cmd_len, buf_len);
}

static int spi_ctrlr_xfer(const struct spi_slave *slave, const void *dout,
		size_t bytesout, void *din, size_t bytesin)
{
	/* First byte is cmd which can not being sent through FIFO. */
	u8 cmd = *(u8 *)dout++;
	size_t count;

	bytesout--;

	/*
	 * Check if this is a write command attempting to transfer more bytes
	 * than the controller can handle. Iterations for writes are not
	 * supported here because each SPI write command needs to be preceded
	 * and followed by other SPI commands, and this sequence is controlled
	 * by the SPI chip driver.
	 */
	if (bytesout > AMD_SB_SPI_TX_LEN) {
		printk(BIOS_WARNING, "FCH SPI: Too much to write. Does your SPI chip driver use"
		     " spi_crop_chunk()?\n");
		return -1;
	}

	spi_write(SPI_EXT_REG_INDX, SPI_EXT_REG_TXCOUNT);
	spi_write(SPI_EXT_REG_DATA, bytesout);
	spi_write(SPI_EXT_REG_INDX, SPI_EXT_REG_RXCOUNT);
	spi_write(SPI_EXT_REG_DATA, bytesin);

	spi_write(SPI_REG_OPCODE, cmd);

	reset_internal_fifo_pointer();
	for (count = 0; count < bytesout; count++, dout++) {
		spi_write(SPI_REG_FIFO, *(uint8_t *)dout);
	}

	reset_internal_fifo_pointer();
	execute_command();

	reset_internal_fifo_pointer();
	/* Skip the bytes we sent. */
	for (count = 0; count < bytesout; count++) {
		cmd = spi_read(SPI_REG_FIFO);
	}

	for (count = 0; count < bytesin; count++, din++) {
		*(uint8_t *)din = spi_read(SPI_REG_FIFO);
	}

	return 0;
}

int chipset_volatile_group_begin(const struct spi_flash *flash)
{
	if (!IS_ENABLED (CONFIG_HUDSON_IMC_FWM))
		return 0;

	ImcSleep(NULL);
	return 0;
}

int chipset_volatile_group_end(const struct spi_flash *flash)
{
	if (!IS_ENABLED (CONFIG_HUDSON_IMC_FWM))
		return 0;

	ImcWakeup(NULL);
	return 0;
}

static const struct spi_ctrlr spi_ctrlr = {
	.xfer = spi_ctrlr_xfer,
	.xfer_vector = spi_xfer_two_vectors,
};

int spi_setup_slave(unsigned int bus, unsigned int cs, struct spi_slave *slave)
{
	slave->bus = bus;
	slave->cs = cs;
	slave->ctrlr = &spi_ctrlr;
	return 0;
}
