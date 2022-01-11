/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/mmio.h>
#include <console/console.h>
#include <spi-generic.h>
#include <spi_flash.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <types.h>
#include <delay.h>

#define AMD_SB_SPI_TX_LEN 8

#if CONFIG(DEBUG_SPI_FLASH)
#define SPI_DEBUG(x...) printk(BIOS_SPEW, x)
#else
#define SPI_DEBUG(x...)
#endif

static uintptr_t spibar;

static u32 get_spi_bar(void)
{
	if (spibar)
		return (u32)spibar;

#if ENV_PCI_SIMPLE_DEVICE
	pci_devfn_t dev = PCI_DEV(0, 0x14, 3);
#else
	struct device *dev = pcidev_on_root(0x14, 3);
	if (!dev)
		die("%s: LPC not dev found!\n", __func__);
#endif
	spibar = pci_read_config32(dev, 0xa0) & ~0x1f;
	return (u32)spibar;

}

void spi_init()
{
	spibar = get_spi_bar();
	printk(BIOS_DEBUG, "%s: SPI base %08x\n", __func__, (u32)spibar);
	/* Clear SpiArbEnable */
	write8((void *)(spibar + 2), read8((void *)(spibar + 2)) & 0xf6);
	/* Enable DropOneClkOnRd */
	// write8((void *)(spibar + 3), read8((void *)(spibar + 3)) | 0x10);
}

static int compare_internal_fifo_pointer(u8 want)
{
	u8 have = read8((void *)(spibar + 0xd)) & 0x07;
	want %= AMD_SB_SPI_TX_LEN;
	if (have != want) {
		printk(BIOS_ERR, "AMD SPI FIFO pointer corruption! Pointer is %d, wanted %d\n",
			have, want);
		return 1;
	} else {
		SPI_DEBUG("AMD SPI FIFO pointer is %d, wanted %d\n", have, want);
		return 0;
	}
}

static void reset_internal_fifo_pointer(void)
{
	write8((void *)(spibar + 2), read8((void *)(spibar + 2)) | 0x10);
	while (read8((void *)(spibar + 0xd)) & 0x7);
}

static void wait_spi_not_busy(void)
{
	while (read8((void *)(spibar + 2)) & 1);
}

static void execute_command(void)
{
	write8((void *)(spibar + 2), read8((void *)(spibar + 2)) | 1);

	wait_spi_not_busy();
}

static int spi_ctrlr_xfer(const struct spi_slave *slave, const void *dout,
		size_t bytesout, void *din, size_t bytesin)
{

	/* First byte is cmd which cannot be sent through the FIFO. */
	u8 cmd = *(u8 *)dout++;
	u8 readwrite;
	u8 readoffby1;
	size_t count;

	SPI_DEBUG("%s: cmd %02x bytesout %ld, bytesin %ld\n", __func__, cmd, bytesout, bytesin);

	bytesout--;

	/*
	 * Check if this is a write command attempting to transfer more bytes
	 * than the controller can handle. Iterations for writes are not
	 * supported here because each SPI write command needs to be preceded
	 * and followed by other SPI commands, and this sequence is controlled
	 * by the SPI chip driver.
	 */
	if (bytesout > AMD_SB_SPI_TX_LEN) {
		printk(BIOS_DEBUG, "AMD SPI: Too much to write. Does your SPI chip driver use"
		     " spi_crop_chunk()?\n");
		return -1;
	}

	wait_spi_not_busy();

	readoffby1 = (bytesout > 0) ? 0 : 1;

	readwrite = (bytesin + readoffby1) << 4 | bytesout;
	write8((void *)(spibar + 1), readwrite);
	write8((void *)(spibar + 0), cmd);

	reset_internal_fifo_pointer();
	if (compare_internal_fifo_pointer(0))
		return -1;
	SPI_DEBUG("Filling SPI FIFO:");
	for (count = 0; count < bytesout; count++, dout++) {
		SPI_DEBUG(" %02x", *(u8 *)dout);
		/* FIXME: Sometimes filling FIFO fails, try until succeed */
		while ((read8((void *)(spibar + 0xd)) & 0x7) != ((count + 1) % 8)) {
			write8((void *)(spibar + 0x0C), *(u8 *)dout);
			udelay(20);
		}
	}
	SPI_DEBUG("\n");

	if (compare_internal_fifo_pointer(bytesout)) {
		// write8((void *)(spibar + 1), readwrite);
		// write8((void *)(spibar + 0), cmd);

		// reset_internal_fifo_pointer();
		// SPI_DEBUG("Filling SPI FIFO 2nd try:");
		// for (count = 0; count < bytesout; count++, dout++) {
		// 	SPI_DEBUG(" %02x", *(u8 *)dout);
		// 	write8((void *)(spibar + 0x0C), *(u8 *)dout);
		// }
		// SPI_DEBUG("\n");
		// if (compare_internal_fifo_pointer(bytesout))
			return -1;
	}

	reset_internal_fifo_pointer();
	execute_command();
	if (compare_internal_fifo_pointer(bytesin + bytesout))
		return -1;

	reset_internal_fifo_pointer();
	/* Skip the bytes we sent. */
	SPI_DEBUG("Skipping:");
	for (count = 0; count < bytesout; count++) {
		cmd = read8((void *)(spibar + 0x0C));
		SPI_DEBUG(" %02x", cmd);
		while ((read8((void *)(spibar + 0xd)) & 0x7) != ((count + 1) % 8)) {
			cmd = read8((void *)(spibar + 0x0C));
			udelay(20);
		}
	}
	SPI_DEBUG("\n");
	if (compare_internal_fifo_pointer(bytesout))
		return -1;
	/* read response bytes */
	SPI_DEBUG("Reading SPI FIFO:");
	for (count = 0; count < bytesin; count++, din++) {
		*(u8 *)din = read8((void *)(spibar + 0x0C));
		SPI_DEBUG(" %02x", *(u8 *)din);
		while ((read8((void *)(spibar + 0xd)) & 0x7) != ((count + bytesout + 1) % 8)) {
			*(u8 *)din = read8((void *)(spibar + 0x0C));
			udelay(20);
		}
	}
	SPI_DEBUG("\n");
	if (compare_internal_fifo_pointer(bytesin + bytesout))
		return -1;

	if (read8((void *)(spibar + 1)) != readwrite) {
		printk(BIOS_ERR,"Unexpected change in AMD SPI read/write count!\n");
		return -1;
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
	.flags = SPI_CNTRLR_DEDUCT_CMD_LEN | SPI_CNTRLR_DEDUCT_OPCODE_LEN,
};

const struct spi_ctrlr_buses spi_ctrlr_bus_map[] = {
	{
		.ctrlr = &spi_ctrlr,
		.bus_start = 0,
		.bus_end = 0,
	},
};

const size_t spi_ctrlr_bus_map_count = ARRAY_SIZE(spi_ctrlr_bus_map);
