/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/mmio.h>
#include <console/console.h>
#include <spi-generic.h>
#include <spi_flash.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <types.h>

#define AMD_SB_SPI_TX_LEN 8

static uintptr_t spibar;

static uint32_t get_spi_bar(void)
{
	struct device *dev;

	if (spibar)
		return (uint32_t)spibar;

	dev = pcidev_on_root(0x14, 3);
	if (!dev)
		printk(BIOS_ERR, "%s: LPC not dev found!\n", __func__);
	spibar = pci_read_config32(dev, 0xa0) & ~0x1f;
	return (uint32_t)spibar;
}

void spi_init()
{
	spibar = get_spi_bar();
	printk(BIOS_DEBUG, "%s: SPI base %08x\n", __func__, (uint32_t)spibar);
}

static void reset_internal_fifo_pointer(void)
{
	do {
		write8((void *)(spibar + 2),
		read8((void *)(spibar + 2)) | 0x10);
	} while (read8((void *)(spibar + 0xd)) & 0x7);
}

static void execute_command(void)
{
	write8((void *)(spibar + 2), read8((void *)(spibar + 2)) | 1);

	while ((read8((void *)(spibar + 2)) & 1) &&
		(read8((void *)(spibar+3)) & 0x80));
}

static int spi_ctrlr_xfer(const struct spi_slave *slave, const void *dout,
		size_t bytesout, void *din, size_t bytesin)
{
	/* First byte is cmd which cannot be sent through the FIFO. */
	u8 cmd = *(u8 *)dout++;
	u8 readoffby1;
	u8 readwrite;
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
		printk(BIOS_DEBUG, "FCH SPI: Too much to write. Does your SPI chip driver use"
		     " spi_crop_chunk()?\n");
		return -1;
	}

	readoffby1 = bytesout ? 0 : 1;

	readwrite = (bytesin + readoffby1) << 4 | bytesout;
	write8((void *)(spibar + 1), readwrite);
	write8((void *)(spibar + 0), cmd);

	reset_internal_fifo_pointer();
	for (count = 0; count < bytesout; count++, dout++) {
		write8((void *)(spibar + 0x0C), *(u8 *)dout);
	}

	reset_internal_fifo_pointer();
	execute_command();

	reset_internal_fifo_pointer();
	/* Skip the bytes we sent. */
	for (count = 0; count < bytesout; count++) {
		cmd = read8((void *)(spibar + 0x0C));
	}
	/* read response bytes */
	for (count = 0; count < bytesin; count++, din++) {
		*(u8 *)din = read8((void *)(spibar + 0x0C));
	}

	return 0;
}

static int xfer_vectors(const struct spi_slave *slave,
			struct spi_op vectors[], size_t count)
{
	return spi_flash_vector_helper(slave, vectors, count, spi_ctrlr_xfer);
}

static const struct spi_ctrlr spi_ctrlr = {
	.xfer_vector = xfer_vectors,
	.max_xfer_size = AMD_SB_SPI_TX_LEN,
	.flags = SPI_CNTRLR_DEDUCT_CMD_LEN,
};

const struct spi_ctrlr_buses spi_ctrlr_bus_map[] = {
	{
		.ctrlr = &spi_ctrlr,
		.bus_start = 0,
		.bus_end = 0,
	},
};

const size_t spi_ctrlr_bus_map_count = ARRAY_SIZE(spi_ctrlr_bus_map);
