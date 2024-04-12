/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <arch/io.h>
#include <cbfs.h>
#include <console/system76_ec.h>
#include <pc80/mc146818rtc.h>
#include <security/vboot/misc.h>
#include <security/vboot/vboot_common.h>
#include <timer.h>
#include "acpi.h"
#include "commands.h"
#include "system76_ec.h"

// This is the command region for System76 EC firmware. It must be
// enabled for LPC in the mainboard.
#define SYSTEM76_EC_BASE 0x0E00
#define SYSTEM76_EC_SIZE 256

#define SPI_SECTOR_SIZE 1024

#define REG_CMD    0
#define REG_RESULT 1
#define REG_DATA   2 // Start of command data

// When command register is 0, command is complete
#define CMD_FINISHED 0

static inline uint8_t system76_ec_read(uint8_t addr)
{
	return inb(SYSTEM76_EC_BASE + (uint16_t)addr);
}

static inline void system76_ec_write(uint8_t addr, uint8_t data)
{
	outb(data, SYSTEM76_EC_BASE + (uint16_t)addr);
}

void system76_ec_init(void)
{
	// Clear entire command region
	for (int i = 0; i < SYSTEM76_EC_SIZE; i++)
		system76_ec_write((uint8_t)i, 0);
}

void system76_ec_flush(void)
{
	system76_ec_write(REG_CMD, CMD_PRINT);

	// Wait for command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(10000, system76_ec_read(REG_CMD) == CMD_FINISHED);

	system76_ec_write(CMD_PRINT_REG_LEN, 0);
}

void system76_ec_print(uint8_t byte)
{
	uint8_t len = system76_ec_read(CMD_PRINT_REG_LEN);
	system76_ec_write(CMD_PRINT_REG_DATA + len, byte);
	system76_ec_write(CMD_PRINT_REG_LEN, len + 1);

	// If we hit the end of the buffer, or were given a newline, flush
	if (byte == '\n' || len >= (SYSTEM76_EC_SIZE - CMD_PRINT_REG_DATA))
		system76_ec_flush();
}

uint8_t system76_ec_smfi_cmd(uint8_t cmd, uint8_t len, uint8_t *data)
{
	int i;

	if (len > SYSTEM76_EC_SIZE - 2)
		return -1;

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(10000, system76_ec_read(REG_CMD) == CMD_FINISHED);

	// Write data first
	for (i = 0; i < len; ++i)
		system76_ec_write(REG_CMD + 2 + i, data[i]);

	// Write command register, which starts command
	system76_ec_write(REG_CMD, cmd);

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(10000, system76_ec_read(REG_CMD) == CMD_FINISHED);

	return (system76_ec_read(REG_RESULT));
}

uint8_t system76_ec_read_version(uint8_t *data)
{
	int i;
	uint8_t result;

	if (!data)
		return -1;

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(10000, system76_ec_read(REG_CMD) == CMD_FINISHED);

	// Write command register, which starts command
	system76_ec_write(REG_CMD, CMD_VERSION);

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(10000, system76_ec_read(REG_CMD) == CMD_FINISHED);

	result = system76_ec_read(REG_RESULT);

	if (result != 0)
		return result;

	// Read data bytes, index should be valid due to length test above
	for (i = 0; i < (SYSTEM76_EC_SIZE - REG_DATA); i++) {
		data[i] = system76_ec_read(REG_DATA + i);
		if (data[i] == '\0')
			break;
	}

	return result;
}

uint8_t system76_ec_read_board(uint8_t *data)
{
	int i;
	uint8_t result;

	if (!data)
		return -1;

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(10000, system76_ec_read(REG_CMD) == CMD_FINISHED);

	// Write command register, which starts command
	system76_ec_write(REG_CMD, CMD_BOARD);

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(10000, system76_ec_read(REG_CMD) == CMD_FINISHED);

	result = system76_ec_read(REG_RESULT);

	if (result != 0)
		return result;

	// Read data bytes, index should be valid due to length test above
	for (i = 0; i < (SYSTEM76_EC_SIZE - REG_DATA); i++) {
		data[i] = system76_ec_read(REG_DATA + i);
		if (data[i] == '\0')
			break;
	}

	return result;
}

int system76_ec_get_bat_threshold(enum bat_threshold_type type)
{
	int ret = -1;

	switch (type) {
	case BAT_THRESHOLD_START:
		ret = ec_read(SYSTEM76_EC_REG_BATTERY_START_THRESHOLD);
		break;
	case BAT_THRESHOLD_STOP:
		ret = ec_read(SYSTEM76_EC_REG_BATTERY_STOP_THRESHOLD);
		break;
	default:
		break;
	}

	return ret;
}

void system76_ec_set_bat_threshold(enum bat_threshold_type type, uint8_t value)
{
	switch (type) {
	case BAT_THRESHOLD_START:
		ec_write(SYSTEM76_EC_REG_BATTERY_START_THRESHOLD, value);
		break;
	case BAT_THRESHOLD_STOP:
		ec_write(SYSTEM76_EC_REG_BATTERY_STOP_THRESHOLD, value);
		break;
	default:
		break;
	}
}

/* Ported from system76_ectool */
static int firmware_str(char *data, int data_len, const char *key, char *dest, int dest_len)
{
	int data_i, key_i, ret_i;
	int key_len = strlen(key);

	data_i = key_i = ret_i = 0;

	/* Locate the key */
	while (data_i < data_len && key_i < key_len) {
		if (data[data_i] == key[key_i])
			key_i += 1;
		else
			key_i = 0;
		data_i += 1;
	}

	if (key_i < key_len)
		return 0;

	while (data_i < data_len && ret_i < dest_len - 1 && data[data_i] != 0)
		dest[ret_i++] = data[data_i++];

	dest[ret_i] = '\0';

	return ret_i;
}

/* Reset the EC SPI bus */
static uint8_t ec_spi_reset(void)
{
	uint8_t reset_cmd = CMD_SPI_FLAG_DISABLE | CMD_SPI_FLAG_SCRATCH;

	return system76_ec_smfi_cmd(CMD_SPI, 1, &reset_cmd);
}

#define SPI_ACCESS_SIZE 252

/*
 * Read len bytes from EC SPI bus into dest
 * Returns 0 on success, <0 on error
 */
static int ec_spi_bus_read(uint8_t *dest, uint32_t len)
{
	uint32_t addr, i, rv;

	uint8_t read_cmd[2] = {
		[0] = CMD_SPI_FLAG_READ | CMD_SPI_FLAG_SCRATCH,
		[1] = 0,
	};

	for (addr = 0; addr + SPI_ACCESS_SIZE < len; addr += SPI_ACCESS_SIZE) {
		read_cmd[1] = SPI_ACCESS_SIZE;

		if ((rv = system76_ec_smfi_cmd(CMD_SPI, ARRAY_SIZE(read_cmd), read_cmd)))
			return -rv;

		if (system76_ec_read(REG_DATA + 1) != read_cmd[1])
			return -1;

		for (i = 0; i < read_cmd[1]; i++)
			dest[addr + i] = system76_ec_read(REG_DATA + 2 + i);
	}

	if (addr == len)
		return 0;

	read_cmd[1] = len % SPI_ACCESS_SIZE;

	if ((rv = system76_ec_smfi_cmd(CMD_SPI, ARRAY_SIZE(read_cmd), read_cmd)))
		return -rv;

	if (system76_ec_read(REG_DATA + 1) != read_cmd[1])
		return -1;

	for (i = 0; i < read_cmd[1]; i++)
		dest[addr + i] = system76_ec_read(REG_DATA + 2 + i);

	return 0;
}

/*
 * Write len bytes from data to EC SPI bus
 * Returns 0 on success, <0 on error
 */
static int ec_spi_bus_write(uint8_t *data, uint8_t len)
{
	uint32_t addr, i, rv;
	uint8_t write_cmd[2] = {
		[0] = CMD_SPI_FLAG_SCRATCH,
		[1] = len,
	};

	for (addr = 0; addr + SPI_ACCESS_SIZE < len; addr += SPI_ACCESS_SIZE) {
		write_cmd[1] = SPI_ACCESS_SIZE;

		for (i = 0; i < write_cmd[1]; ++i)
			system76_ec_write(REG_DATA + 2 + i, data[addr + i]);

		if ((rv = system76_ec_smfi_cmd(CMD_SPI, ARRAY_SIZE(write_cmd), write_cmd)))
			return -rv;

		if (system76_ec_read(REG_DATA + 1) != write_cmd[1])
			return -1;
	}

	if (addr == len)
		return 0;

	write_cmd[1] = len % SPI_ACCESS_SIZE;

	for (i = 0; i < write_cmd[1]; i++)
		system76_ec_write(REG_DATA + 2 + i, data[addr + i]);

	if ((rv = system76_ec_smfi_cmd(CMD_SPI, ARRAY_SIZE(write_cmd), write_cmd)))
		return -rv;

	if (system76_ec_read(REG_DATA + 1) != write_cmd[1])
		return -1;

	return 0;
}

static int ec_spi_cmd_status(void)
{
	int rv;
	uint8_t cmd;

	if ((rv = ec_spi_reset()))
		return rv;

	cmd = 0x05; /* SPI Read Status */

	if ((rv = ec_spi_bus_write(&cmd, 1)))
		return rv;

	if ((rv = ec_spi_bus_read(&cmd, 1)))
		return rv;

	return cmd;
}

static int ec_spi_cmd_write_enable(void)
{
	int status, rv;
	uint8_t cmd;

	if ((rv = ec_spi_reset()))
		return rv;

	cmd = 0x06; /* SPI Write Enable */

	if ((rv = ec_spi_bus_write(&cmd, 1)))
		return rv;

	do {
		status = ec_spi_cmd_status();
	} while (status > 0 && (status & 3) != 2);

	return 0;
}

static int ec_spi_cmd_write_disable(void)
{
	int status, rv;
	uint8_t cmd;

	if ((rv = ec_spi_reset()))
		return rv;

	cmd = 0x04; /* SPI Write Disable */

	if ((rv = ec_spi_bus_write(&cmd, 1)))
		return rv;

	do {
		status = ec_spi_cmd_status();
	} while (status > 0 && (status & 3) != 0);

	return 0;
}

/* Erase a sector at addr. Returns 0 on success <0 on error. */
static int ec_spi_erase_sector(uint32_t addr)
{
	int status, rv;
	uint8_t buf[4] = {
		[0] = 0xD7, /* SPI Sector Erase */
		[1] = (addr >> 16) & 0xFF,
		[2] = (addr >> 8) & 0xFF,
		[3] = addr & 0xFF,
	};

	if ((rv = ec_spi_cmd_write_enable()))
		return rv;

	if ((rv = ec_spi_reset()))
		return rv;

	if ((rv = ec_spi_bus_write(buf, 4)))
		return rv;

	do {
		status = ec_spi_cmd_status();
	} while (status > 0 && (status & 1) != 0);

	if ((rv = ec_spi_cmd_write_disable()))
		return rv;

	return 0;
}

/*
 * Erase the entire chip sector by sector.
 * Chip / bulk erase doesn't work on this hardware.
 * Returns erased byte count on success and <0 on error.
 */
static int ec_spi_erase_chip(void)
{
	int rv;
	uint32_t addr;

	for (addr = 0; addr < CONFIG_EC_SYSTEM76_EC_FLASH_SIZE; addr += SPI_SECTOR_SIZE)
		if ((rv = ec_spi_erase_sector(addr)))
			return rv;

	return addr;
}

/*
 * Program an entire chip with a given image.
 * Returns written byte count on success and <0 on error.
 */
static int ec_spi_image_write(uint8_t *image, ssize_t size)
{
	int status, rv;
	uint32_t addr;
	uint8_t buf[6] = {0};

	if ((rv = ec_spi_cmd_write_enable()))
		return rv;

	/* SPI AAI Word Program */
	buf[0] = 0xAD;

	for (addr = 0; addr < size; addr += 2) {
		if ((rv = ec_spi_reset()))
			return rv;

		if (addr == 0) {
			/* 1st cmd bytes 1,2,3 are the address */
			buf[4] = image[0];
			buf[5] = image[1];

			rv = ec_spi_bus_write(buf, 6);
		} else {
			buf[1] = image[addr];
			buf[2] = image[addr + 1];

			rv = ec_spi_bus_write(buf, 3);
		}

		if (rv)
			return rv;

		do {
			status = ec_spi_cmd_status();
		} while (status > 0 && (status & 1) != 0);
	}

	if ((rv = ec_spi_cmd_write_disable()))
		return rv;

	return addr;
}

/* Read a sector into dest. Returns 0 on success and <0 on error. */
static int ec_spi_read_sector(uint8_t *dest, uint32_t addr)
{
	int rv;
	uint8_t buf[5] = {
		[0] = 0x0B, /* SPI Read */
		[1] = (addr >> 16) & 0xFF,
		[2] = (addr >> 8) & 0xFF,
		[3] = addr & 0xFF,
		[4] = 0,
	};

	if ((rv = ec_spi_reset()))
		return rv;

	if ((rv = ec_spi_bus_write(buf, 5)))
		return rv;

	if ((rv = ec_spi_bus_read(dest, SPI_SECTOR_SIZE)))
		return rv;

	return 0;
}

/* Verify an image sector by sector. Returns 0 on success and <0 on error. */
static int ec_spi_image_verify(uint8_t *image, ssize_t image_sz)
{
	uint8_t *sector;
	uint32_t addr;
	int rv;

	sector = malloc(SPI_SECTOR_SIZE);
	if (!sector)
		return -1;
	rv = 0;

	for (addr = 0;
	     addr < CONFIG_EC_SYSTEM76_EC_FLASH_SIZE && addr + SPI_SECTOR_SIZE < image_sz;
	     addr += SPI_SECTOR_SIZE) {
		if ((rv = ec_spi_read_sector(sector, addr)))
			goto exit;

		if ((rv = -memcmp(sector, image + addr, SPI_SECTOR_SIZE)))
			goto exit;
	}

	if (addr == image_sz)
		goto exit;

	if ((rv = ec_spi_read_sector(sector, addr)))
		goto exit;

	if ((rv = -memcmp(sector, image + addr, image_sz % SPI_SECTOR_SIZE)))
		goto exit;

exit:
	free(sector);
	return rv;
}

/* Sync the EC firmware with an image contained in CBFS. */
static void system76_ec_fw_sync(void *unused)
{
	size_t image_sz;
	char *image;
	char img_board_str[64];
	char img_version_str[64];
	char cur_board_str[64];
	char cur_version_str[64];
	uint8_t smfi_cmd;
	int rv;

	if (!CONFIG(EC_SYSTEM76_EC_UPDATE))
		return;

	if (vboot_recovery_mode_enabled()) {
		printk(BIOS_DEBUG, "EC: skipping update in recovery mode.\n");
		return;
	}

	image = malloc(CONFIG_EC_SYSTEM76_EC_FLASH_SIZE);
	if (image == NULL) {
		printk(BIOS_ERR, "EC: failed to allocate memory for update.");
		return;
	}

	if (!(image_sz = cbfs_load("ec.rom", image, CONFIG_EC_SYSTEM76_EC_FLASH_SIZE))) {
		printk(BIOS_ERR, "EC: failed to load update from CBFS.");
		goto cleanup;
	}

	printk(BIOS_DEBUG, "EC: update found (%ld bytes)\n", image_sz);

	/* Sanity checks */
	if (!image_sz || image_sz % 2 || image_sz > CONFIG_EC_SYSTEM76_EC_FLASH_SIZE) {
		printk(BIOS_ERR, "EC: incorrect update size.\n");
		goto cleanup;
	}

	if (!firmware_str(image, image_sz, "76EC_BOARD=", img_board_str,
			  ARRAY_SIZE(img_board_str))) {
		printk(BIOS_ERR, "EC: could not determine update target board.\n");
		goto cleanup;
	}

	if (!firmware_str(image, image_sz, "76EC_VERSION=", img_version_str,
			  ARRAY_SIZE(img_version_str))) {
		printk(BIOS_ERR, "EC: could not determine update version.\n");
		goto cleanup;
	}

	printk(BIOS_DEBUG, "EC: update target: %s\n", img_board_str);
	printk(BIOS_DEBUG, "EC: update version: %s\n", img_version_str);

	system76_ec_read_board((uint8_t *)cur_board_str);
	system76_ec_read_version((uint8_t *)cur_version_str);

	if (strcmp(img_board_str, cur_board_str)) {
		printk(BIOS_ERR, "EC: update target mismatch detected! Found %s, expected %s\n",
		       img_board_str, cur_board_str);
		goto cleanup;
	}

	printk(BIOS_DEBUG, "EC: current version: %s\n", cur_version_str);
	if (!strcmp(img_version_str, cur_version_str)) {
		printk(BIOS_DEBUG, "EC: update not needed.\n");
		goto cleanup;
	} else {
		printk(BIOS_INFO, "EC: update required!\n");
	}

	if ((ec_read(0x10) & 0x01) != 0x01) {
		printk(BIOS_WARNING, "EC: AC adapter not connected, skipping update.\n");
		if (CONFIG(VBOOT))
			vboot_fail_and_reboot(vboot_get_context(),
					      VB2_RECOVERY_EC_SOFTWARE_SYNC, EC_UPDATE_NO_AC);
		goto cleanup;
	}

	/* Jump to Scratch ROM */
	smfi_cmd = CMD_SPI_FLAG_SCRATCH;
	if (system76_ec_smfi_cmd(CMD_SPI, 1, &smfi_cmd)) {
		/* If we failed to jump to scratch ROM, then we can probably continue booting. */
		printk(BIOS_ERR, "EC: failed to jump to scratch ROM!\n");
		goto cleanup;
	}

	rv = ec_spi_erase_chip();
	if (rv < 0) {
		/*
		 * Best case, nothing was erased.
		 * Worst case, everything is erased and EC will boot from backup.
		 */
		printk(BIOS_CRIT, "EC: erase failed!\n");
		goto cleanup;
	}
	printk(BIOS_DEBUG, "EC: erased %d bytes\n", rv);

	rv = ec_spi_image_write((uint8_t *)image, image_sz);
	if (rv < 0) {
		/* EC is now in an unknown state. It may still boot from backup. */
		printk(BIOS_ALERT, "EC: update failed!\n");
		goto cleanup;
	}
	printk(BIOS_DEBUG, "EC: wrote %d bytes\n", rv);

	rv = ec_spi_image_verify((uint8_t *)image, image_sz);
	if (rv < 0) {
		printk(BIOS_ALERT, "EC: update verification failed!\n");
	} else {
		printk(BIOS_DEBUG, "EC: update verified.\n");
	}

	smfi_cmd = CMD_SPI_FLAG_DISABLE;
	if (system76_ec_smfi_cmd(CMD_SPI, 1, &smfi_cmd)) {
		printk(BIOS_ERR, "EC: failed to disable SPI bus!\n");
		goto cleanup;
	}

	smfi_cmd = 0;
	if (system76_ec_smfi_cmd(CMD_RESET, 1, &smfi_cmd)) {
		printk(BIOS_ERR, "EC: failed to trigger reset!\n");
		goto cleanup;
	}

cleanup:
	free(image);
	return;
}

BOOT_STATE_INIT_ENTRY(BS_PRE_DEVICE, BS_ON_ENTRY, system76_ec_fw_sync, NULL);
