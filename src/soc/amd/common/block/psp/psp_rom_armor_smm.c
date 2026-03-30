/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/psp.h>
#include <amdblocks/spi.h>
#include <boot_device.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include <spi_flash.h>
#include <spi-generic.h>
#include <stdint.h>
#include <string.h>
#include <types.h>
#include "psp_def.h"
#include "psp_rom_armor_apmc.h"

/* Save the SPI frequency, CS and page before entering SMM-only mode for ROM Armor 1 */
static uint8_t spi_cs;
static uint8_t spi_freq;
static uint8_t spi_page;

static uint8_t transfer_buffer[4 * KiB] __aligned(32);

static bool initialized;

bool rom_armor_enforced = false;

static void print_psp_spi_cmd_status(uint16_t result)
{
	printk(BIOS_ERR, "PSP RomArmor transaction result: 0x%04x ", result);

	switch (result & 0xf) {
	case SPI_CMD_NOT_PROCEDDED:
		printk(BIOS_ERR, "(command not procedded)\n");
		break;
	case SPI_CMD_COMPLETED:
		printk(BIOS_ERR, "(command completed)\n");
		break;
	case SPI_CMD_EXECUTION_ERROR:
		printk(BIOS_ERR, "(command execution error)\n");
		break;
	case SPI_CMD_NOT_ALLOWED:
		printk(BIOS_ERR, "(command not allowed)\n");
		break;
	case SPI_CMD_MALFORMED:
		printk(BIOS_ERR, "(command malformed)\n");
		break;
	default:
		printk(BIOS_ERR, "(unknown)\n");
		break;
	}
}

int psp_rom_armor1_spi_transaction(struct rom_armor_spi_cmd *cmd_buf)
{
	int cmd_status;
	struct rom_armor1_comm_buffer *buffer;
	struct mbox_rom_armor1_buffer spi_cmd_buffer = {
		.header.size = sizeof(spi_cmd_buffer),
		.tseg_addr = (uint64_t)&transfer_buffer[0],
		.chip_select = spi_cs,
	};
	uint8_t *buf;

	if (!initialized)
		return -1;

	/* PSP verifies that this buffer is at the address specified in enter SMM-only mode */
	buffer = (struct rom_armor1_comm_buffer *)&transfer_buffer[0];
	if (!cmd_buf) {
		printk(BIOS_ERR, "PSP RomArmor transaction: Invalid parameters\n");
		return -1;
	}

	/* Copy the command */
	memcpy(&buffer->spi_cmd, cmd_buf, sizeof(*cmd_buf));

	buffer->cmd_result = 0;
	buffer->ready_to_run = 1;
	buffer->cmd_count = 1;
	buffer->spi_cmd[0].freq = spi_freq;

	if (spi_cs != buffer->spi_cmd[0].cs) {
		printk(BIOS_ERR, "PSP ROM Armor: SPI CS mismatch\n");
		buffer->spi_cmd[0].cs = spi_cs;
	}

	if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_DEBUG)) {
		printk(BIOS_SPEW, "PSP: Sending transaction opcode=%u cs=%u freq=%u tx=%u rx=%u\n",
		       buffer->spi_cmd[0].opcode, buffer->spi_cmd[0].cs,
		       buffer->spi_cmd[0].freq, buffer->spi_cmd[0].tx_bytes,
		       buffer->spi_cmd[0].rx_bytes);

		printk(BIOS_SPEW, "PSP: Transaction buffer:");
		buf = &buffer->spi_cmd[0].buffer[0];
		for (size_t i = 0;
		     i < buffer->spi_cmd[0].tx_bytes && i < PSP_MAX_SPI_DATA_BUFFER_SIZE;
		     i++) {
			if (i % 16 == 0)
				printk(BIOS_SPEW, "\n");

			printk(BIOS_SPEW, "%02X", buf[i]);
		}
		printk(BIOS_SPEW, "\n");
	}

	asm volatile ("sfence");

	/* Send command to PSP */
	cmd_status = send_psp_command(MBOX_BIOS_CMD_ARMOR_EXECUTE_SPI_CMD, &spi_cmd_buffer);
	if (cmd_status || spi_cmd_buffer.header.status) {
		psp_print_cmd_status(cmd_status, &spi_cmd_buffer.header);
		return cmd_status ? cmd_status : spi_cmd_buffer.header.status;
	}

	if ((buffer->cmd_result & 0xf) != SPI_CMD_COMPLETED) {
		print_psp_spi_cmd_status(buffer->cmd_result);
		memset(transfer_buffer, 0, 4 * KiB);
		return -1;
	}

	if (cmd_buf->rx_bytes) {
		memcpy(&cmd_buf->buffer[cmd_buf->tx_bytes],
		       &buffer->spi_cmd[0].buffer[cmd_buf->tx_bytes],
		       cmd_buf->rx_bytes);
		memset(buffer->spi_cmd[0].buffer, 0, PSP_MAX_SPI_DATA_BUFFER_SIZE);

		if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_DEBUG)) {
			printk(BIOS_SPEW, "PSP: Received:");
			buf = &buffer->spi_cmd[0].buffer[cmd_buf->tx_bytes];
			for (size_t i = 0;
			     i < buffer->spi_cmd[0].rx_bytes && i < PSP_MAX_SPI_DATA_BUFFER_SIZE;
			     i++) {
				if (i % 16 == 0)
					printk(BIOS_SPEW, "\n");

				printk(BIOS_SPEW, "%02X", buf[i]);
			}
			printk(BIOS_SPEW, "\n");
		}
	}

	memset(transfer_buffer, 0, 4 * KiB);

	return 0;
}

static ssize_t psp_rom_armor_spi_readat(const struct region_device *rd, void *buf,
					size_t offset, size_t len)
{
	if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_DEBUG))
		printk(BIOS_DEBUG, "PSP RomArmor rdev_ops: read offset=0x%zx, len=0x%zx\n",
		       offset, len);

	if (!buf || !len) {
		printk(BIOS_ERR, "PSP RomArmor rdev_ops: Invalid read parameters\n");
		return -1;
	}

	/* Try to read from MMIO flash space first */
	const struct region_device *ro_dev = boot_device_ro();
	if (ro_dev) {
		printk(BIOS_ERR, "PSP RomArmor rdev_ops: Attempting RO rdev\n");
		if (len > region_device_sz(ro_dev) ||
		    offset > region_device_sz(ro_dev) ||
		    (offset + len) > region_device_sz(ro_dev)) {
			printk(BIOS_ERR, "PSP RomArmor rdev_ops: read range exceeds flash size\n");
			return -1;
		}

		return ro_dev->ops->readat(ro_dev, buf, offset, len);
	}

	if (!rd) {
		printk(BIOS_ERR, "PSP RomArmor rdev_ops: No boot device RW\n");
		return -1;
	}

	if (len > region_device_sz(rd) ||
	    offset > region_device_sz(rd) ||
	    (offset + len) > region_device_sz(rd)) {
		printk(BIOS_ERR, "PSP RomArmor rdev_ops: read range exceeds flash size\n");
		return -1;
	}

	/* ROM Armor 1 hooks into SPI controller */
	return rd->ops->readat(rd, buf, offset, len);
}

/*
 * Write data to the SPI flash via ROM Armor
 *
 * Chunks the write into 4KB blocks and communicates with PSP
 * to perform write operations through RomArmor3 protocol.
 */
static ssize_t psp_rom_armor_spi_writeat(const struct region_device *rd, const void *buf,
					 size_t offset, size_t len)
{
	if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_DEBUG))
		printk(BIOS_DEBUG, "ROM Armor rdev_ops: Write offset=0x%zx, len=0x%zx\n",
		       offset, len);

	if (!buf || len == 0) {
		printk(BIOS_ERR, "ROM Armor rdev_ops: Invalid write parameters\n");
		return -1;
	}

	if (!rd) {
		printk(BIOS_ERR, "PSP RomArmor rdev_ops: No boot device RW\n");
		return -1;
	}

	if (len > region_device_sz(rd) ||
	    offset > region_device_sz(rd) ||
	    (offset + len) > region_device_sz(rd)) {
		printk(BIOS_ERR, "ROM Armor rdev_ops: Write range exceeds flash size\n");
		return -1;
	}

	/* ROM Armor 1 hooks into SPI controller */
	return rd->ops->writeat(rd, buf, offset, len);
}

/*
 * Erase SPI flash sectors via ROM Armor
 *
 * Supports both 4KB and 64KB erase block sizes for efficiency.
 * Uses PSP firmware to perform erase operations through RomArmor3 protocol.
 */
static ssize_t psp_rom_armor_spi_eraseat(const struct region_device *rd,
					 size_t offset, size_t len)
{
	if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_DEBUG))
		printk(BIOS_DEBUG, "ROM Armor rdev_ops: Erase offset=0x%zx, len=0x%zx\n",
		       offset, len);

	if (len == 0 || !IS_ALIGNED(len, 4 * KiB) || !IS_ALIGNED(offset, 4 * KiB)) {
		printk(BIOS_ERR, "ROM Armor rdev_ops: Invalid erase parameters\n");
		return -1;
	}

	if (!rd) {
		printk(BIOS_ERR, "PSP RomArmor rdev_ops: No boot device RW\n");
		return -1;
	}

	if (len > region_device_sz(rd) ||
	    offset > region_device_sz(rd) ||
	    (offset + len) > region_device_sz(rd)) {
		printk(BIOS_ERR, "ROM Armor rdev_ops: Write range exceeds flash size\n");
		return -1;
	}

	/* ROM Armor 1 hooks into SPI controller */
	return rd->ops->eraseat(rd, offset, len);
}

/* Rom Armor flash ops are only accessible in SMM. */
static const struct region_device_ops rom_armor_spi_flash_ops = {
	.mmap = NULL,
	.munmap = NULL,
	.readat = psp_rom_armor_spi_readat,
	.writeat = psp_rom_armor_spi_writeat,
	.eraseat = psp_rom_armor_spi_eraseat,
};

struct region_device rom_armor_smm_rw =
	REGION_DEV_INIT(&rom_armor_spi_flash_ops, 0, CONFIG_ROM_SIZE);

/*
 * SMM handler for ROM Armor operations
 * Called from APMC SMI handler with parameters passed via CPU registers
 */
static bool shutdown;

uint32_t rom_armor_exec(uint8_t command, void *param)
{
	ssize_t ret;
	size_t flash_size = 0;

	/* After shutdown don't respond to requests */
	if (shutdown)
		return ROM_ARMOR_RET_SHUTDOWN;

	/* Shutdown command doesn't need param */
	if (!param && command != ROM_ARMOR_APM_CMD_SHUTDOWN)
		return ROM_ARMOR_RET_FAILURE;

	/* Ensure param does not point to SMM space */
	if (param && smm_points_to_smram(param, sizeof(uintptr_t)))
		return ROM_ARMOR_RET_FAILURE;

	switch (command) {
	case ROM_ARMOR_APM_CMD_INIT: {
		struct rom_armor_params_init *params = param;
		if (initialized)
			return ROM_ARMOR_RET_FAILURE;

		memset(transfer_buffer, 0, 4 * KiB);

		spi_cs = boot_device_spi_cs();
		spi_cs = spi_cs < 2 ? spi_cs + 1 : 1; /* SPI CS is 1-based for ROM Armor */
		spi_freq = DECODE_SPI_NORMAL_SPEED(spi_read16(SPI100_SPEED_CONFIG));
		spi_page = spi_read8(SPI_ROM_PAGE) & SPI_ROM_PAGE_SEL;

		const struct spi_flash *flash = boot_device_spi_flash();
		if (flash)
			spi_release_bus(&flash->spi);

		/* For ROM Armor 1 we have to pass the transfer buffer used to talk to PSP */
		params->operation_buf = (uint64_t)&transfer_buffer[0];
		params->chip_select = spi_cs;

		if (psp_rom_armor_enter_smm_mode(params, &flash_size) != 0) {
			printk(BIOS_ERR, "%s: Failed to enter SMM mode\n", __func__);
			return ROM_ARMOR_RET_FAILURE;
		}

		rom_armor_enforced = true;

		printk(BIOS_INFO, "%s: Initialized with flash size 0x%zx\n", __func__, flash_size);
		if (region_device_sz(&rom_armor_smm_rw) != flash_size) {
			printk(BIOS_ERR, "%s: Flash size 0x%zx doesn't match CONFIG_ROM_SIZE!\n",
			       __func__, flash_size);
			rom_armor_smm_rw.region.size = flash_size;
		}

		if (psp_rom_armor_enforce_whitelist(params, spi_freq) != 0) {
			printk(BIOS_ERR, "%s: Failed to enforce SPI whitelist\n", __func__);
			return ROM_ARMOR_RET_FAILURE;
		}
		/*
		 * Do not call QueryHSTI as it seems to fail on Turin after
		 * entering SMM-only mode in the very same SMI handler.
		 * However, it unblocks in normal, non-SMM world for some reason.
		 *
		 * if (!psp_get_hsti_state_rom_armor_enforced())
		 *	return ROM_ARMOR_RET_FAILURE;
		 */

		initialized = true;

		return ROM_ARMOR_RET_SUCCESS;
	}
	case ROM_ARMOR_APM_CMD_READ: {
		struct rom_armor_params_read *params = param;

		if (!initialized)
			return ROM_ARMOR_RET_NOT_INITIALIZED;

		/* Validate parameters */
		if (smm_points_to_smram(params->buf, params->size))
			return ROM_ARMOR_RET_FAILURE;

		if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_DEBUG))
			printk(BIOS_DEBUG, "%s: Read offset=0x%zx size=0x%zx\n",
			       __func__, params->offset, params->size);

		ret = psp_rom_armor_spi_readat(boot_device_rw(), params->buf,
					       params->offset, params->size);
		return (ret == (ssize_t)params->size) ? ROM_ARMOR_RET_SUCCESS :
							ROM_ARMOR_RET_FAILURE;
	}

	case ROM_ARMOR_APM_CMD_WRITE: {
		struct rom_armor_params_write *params = param;

		if (!initialized)
			return ROM_ARMOR_RET_NOT_INITIALIZED;

		/* Validate parameters */
		if (smm_points_to_smram(params->buf, params->size))
			return ROM_ARMOR_RET_FAILURE;

		if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_DEBUG))
			printk(BIOS_DEBUG, "%s: Write src=%p offset=0x%zx size=0x%zx\n",
			       __func__, params->buf, params->offset, params->size);

		ret = psp_rom_armor_spi_writeat(boot_device_rw(), params->buf,
						params->offset, params->size);
		return (ret == (ssize_t)params->size) ? ROM_ARMOR_RET_SUCCESS :
							ROM_ARMOR_RET_FAILURE;
	}

	case ROM_ARMOR_APM_CMD_ERASE: {
		struct rom_armor_params_erase *params = param;

		if (!initialized)
			return ROM_ARMOR_RET_NOT_INITIALIZED;

		if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_DEBUG))
			printk(BIOS_DEBUG, "%s: Erase offset=0x%zx size=0x%zx\n",
			       __func__, params->offset, params->size);

		ret = psp_rom_armor_spi_eraseat(boot_device_rw(),
						params->offset, params->size);
		return (ret == (ssize_t)params->size) ? ROM_ARMOR_RET_SUCCESS :
							ROM_ARMOR_RET_FAILURE;
	}
	case ROM_ARMOR_APM_CMD_SHUTDOWN:
		shutdown = true;
		printk(BIOS_DEBUG, "%s: Disabled\n", __func__);
		return ROM_ARMOR_RET_SUCCESS;
	default:
		printk(BIOS_ERR, "%s: Unknown command: 0x%02x\n", __func__, command);
	}
	return ROM_ARMOR_RET_UNSUPPORTED;
}
