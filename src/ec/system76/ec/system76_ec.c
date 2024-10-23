/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <arch/io.h>
#include <cbfs.h>
#include <console/system76_ec.h>
#include <delay.h>
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

#define SPI_WREN		0x06	/* Write Enable */
#define SPI_WRDI		0x04	/* Write Disable */
#define SPI_RDSR		0x05	/* Read Status Register */
#define SPI_FAST_READ		0x0b	/* Read Data Bytes at Higher Speed */
#define SPI_AAI_WP		0xad	/* Auto Address Increment Word Program */
#define SPI_BE			0xd7	/* 1K Block Erase */

#define SPI_SR_WIP		(1 << 0)	/* Write-in-Progress */
#define SPI_SR_WEL		(1 << 1)	/* Write enable */

#define SPI_TIMEOUT_10MS	10000

#define SPI_ACCESS_SIZE 252

#define MAX_WRITE_RETRY	3

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
	wait_us(SPI_TIMEOUT_10MS, system76_ec_read(REG_CMD) == CMD_FINISHED);

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

	if (len > SYSTEM76_EC_SIZE - 2) {
		printk(BIOS_ERR, "system76_ec: Invalid command length\n");
		return -1;
	}

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(SPI_TIMEOUT_10MS, system76_ec_read(REG_CMD) == CMD_FINISHED);

	// Write data first
	for (i = 0; i < len; ++i)
		system76_ec_write(REG_DATA + i, data[i]);

	// Write command register, which starts command
	system76_ec_write(REG_CMD, cmd);

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(SPI_TIMEOUT_10MS, system76_ec_read(REG_CMD) == CMD_FINISHED);

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
	wait_us(SPI_TIMEOUT_10MS, system76_ec_read(REG_CMD) == CMD_FINISHED);

	// Write command register, which starts command
	system76_ec_write(REG_CMD, CMD_VERSION);

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(SPI_TIMEOUT_10MS, system76_ec_read(REG_CMD) == CMD_FINISHED);

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
	wait_us(SPI_TIMEOUT_10MS, system76_ec_read(REG_CMD) == CMD_FINISHED);

	// Write command register, which starts command
	system76_ec_write(REG_CMD, CMD_BOARD);

	// Wait for previous command completion, for up to 10 milliseconds, with a
	// test period of 1 microsecond
	wait_us(SPI_TIMEOUT_10MS, system76_ec_read(REG_CMD) == CMD_FINISHED);

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

	return system76_ec_smfi_cmd(CMD_SPI, sizeof(reset_cmd), &reset_cmd);
}

/*
 * Read len bytes from EC SPI bus into dest.
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

		if ((rv = system76_ec_smfi_cmd(CMD_SPI, ARRAY_SIZE(read_cmd), read_cmd))) {
			printk(BIOS_ERR, "system76_ec: Failed to send read SPI bus command\n");
			return -rv;
		}

		if (system76_ec_read(REG_DATA + 1) != read_cmd[1]) {
			printk(BIOS_ERR, "system76_ec: SPI bus read insufficient bytes\n");
			return -1;
		}

		for (i = 0; i < read_cmd[1]; i++)
			dest[addr + i] = system76_ec_read(REG_DATA + 2 + i);
	}

	if (addr == len)
		return 0;

	read_cmd[1] = len % SPI_ACCESS_SIZE;

	if ((rv = system76_ec_smfi_cmd(CMD_SPI, ARRAY_SIZE(read_cmd), read_cmd))) {
		printk(BIOS_ERR, "system76_ec: Failed to send read SPI bus command (remainder)\n");
		return -rv;
	}

	if (system76_ec_read(REG_DATA + 1) != read_cmd[1]) {
		printk(BIOS_ERR, "system76_ec: SPI bus read remainder insufficient bytes\n");
		return -1;
	}

	for (i = 0; i < read_cmd[1]; i++)
		dest[addr + i] = system76_ec_read(REG_DATA + 2 + i);

	return 0;
}

/*
 * Write len bytes from data to EC SPI bus with flags.
 * Returns 0 on success, <0 on error
 */
static int ec_spi_bus_write_with_flags(uint8_t *data, uint8_t len, uint8_t flags)
{
	uint32_t addr, i, rv;
	uint8_t write_cmd[2] = {
		[0] = CMD_SPI_FLAG_SCRATCH | flags,
		[1] = len,
	};

	for (addr = 0; addr + SPI_ACCESS_SIZE < len; addr += SPI_ACCESS_SIZE) {
		write_cmd[1] = SPI_ACCESS_SIZE;

		for (i = 0; i < write_cmd[1]; ++i)
			system76_ec_write(REG_DATA + ARRAY_SIZE(write_cmd) + i, data[addr + i]);

		rv = system76_ec_smfi_cmd(CMD_SPI, ARRAY_SIZE(write_cmd), write_cmd);
		if (rv) {
			printk(BIOS_ERR, "system76_ec: Failed to send write SPI bus command\n");
			return -rv;
		}

		if (system76_ec_read(REG_DATA + 1) != write_cmd[1]) {
			printk(BIOS_ERR, "system76_ec: SPI bus write insufficient bytes\n");
			return -1;
		}
	}

	if (addr == len)
		return 0;

	write_cmd[1] = len % SPI_ACCESS_SIZE;

	for (i = 0; i < write_cmd[1]; i++)
		system76_ec_write(REG_DATA + ARRAY_SIZE(write_cmd) + i, data[addr + i]);

	rv = system76_ec_smfi_cmd(CMD_SPI, ARRAY_SIZE(write_cmd), write_cmd);
	if (rv) {
		printk(BIOS_ERR, "system76_ec: Failed to send write SPI bus command (remainder)\n");
		return -rv;
	}

	if (system76_ec_read(REG_DATA + 1) != write_cmd[1]) {
		printk(BIOS_ERR, "system76_ec: SPI bus write remainder insufficient bytes\n");
		return -1;
	}

	return 0;
}

/*
 * Write len bytes from data to EC SPI bus
 * Returns 0 on success, <0 on error
 */
static int ec_spi_bus_write(uint8_t *data, uint8_t len)
{
	return ec_spi_bus_write_with_flags(data, len, 0);
}

static int ec_spi_wait_status(uint8_t mask, uint8_t value, const char *func,
			      const unsigned int line)
{
	struct stopwatch sw;
	uint8_t status;
	int rv;
	uint8_t cmd = SPI_RDSR;

	rv = ec_spi_reset();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_reset failed\n", __func__);
		return rv;
	}

	rv = ec_spi_bus_write(&cmd, sizeof(cmd));
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_bus_write failed\n", __func__);
		return rv;
	}

	stopwatch_init_usecs_expire(&sw, SPI_TIMEOUT_10MS);

	rv = ec_spi_bus_read(&status, sizeof(status));
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_bus_read failed\n", __func__);
		return rv;
	}

	while ((status & mask) != value) {
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR, "%s: Timeout at %s:%u\n", __func__, func, line);
			ec_spi_reset();
			return -1;
		}

		/*
		 * Status Register can be constantly read from SPI bus.
		 * No need to start new RDSR command.
		 */
		rv = ec_spi_bus_read(&status, sizeof(status));
		if (rv) {
			printk(BIOS_ERR, "%s: ec_spi_bus_read failed\n", __func__);
			ec_spi_reset();
			return rv;
		}
	}

	rv = ec_spi_reset();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_reset failed\n", __func__);
		return rv;
	}

	return 0;
}

static int ec_spi_cmd_write_enable(void)
{
	int rv;
	uint8_t cmd = SPI_WREN;;

	rv = ec_spi_wait_status(SPI_SR_WIP, 0, __func__, __LINE__);
	if (rv) {
		printk(BIOS_ERR, "%s: flash not ready\n", __func__);
		return rv;
	}

	rv = ec_spi_bus_write(&cmd, sizeof(cmd));
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_bus_write failed\n", __func__);
		return rv;
	}

	return ec_spi_wait_status(SPI_SR_WIP | SPI_SR_WEL, SPI_SR_WEL, __func__, __LINE__);
}

static int ec_spi_cmd_write_disable(void)
{
	int rv;
	uint8_t cmd = SPI_WRDI;

	rv = ec_spi_reset();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_reset failed\n", __func__);
		return rv;
	}

	rv = ec_spi_bus_write(&cmd, sizeof(cmd));
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_bus_write failed\n", __func__);
		return rv;
	}
	return ec_spi_wait_status(SPI_SR_WIP | SPI_SR_WEL, 0, __func__, __LINE__);
}

/* Erase a sector at addr. Returns 0 on success <0 on error. */
static int ec_spi_erase_sector(uint32_t addr)
{
	int rv;
	uint8_t buf[4] = {
		[0] = SPI_BE,
		[1] = (addr >> 16) & 0xFF,
		[2] = (addr >> 8) & 0xFF,
		[3] = addr & 0xFF,
	};

	rv = ec_spi_cmd_write_enable();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_cmd_write_enable failed\n", __func__);
		return rv;
	}

	rv = ec_spi_reset();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_reset failed\n", __func__);
		return rv;
	}

	rv = ec_spi_bus_write(buf, ARRAY_SIZE(buf));
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_bus_write failed\n", __func__);
		return rv;
	}

	rv = ec_spi_wait_status(SPI_SR_WIP, 0, __func__, __LINE__);
	if (rv)
		return rv;

	rv = ec_spi_cmd_write_disable();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_cmd_write_disable failed\n", __func__);
		return rv;
	}

	return 0;
}

/* Read a sector into dest. Returns 0 on success and <0 on error. */
static int ec_spi_read_sector(uint8_t *dest, uint32_t addr)
{
	int rv;
	uint8_t buf[5] = {
		[0] = SPI_FAST_READ,
		[1] = (addr >> 16) & 0xFF,
		[2] = (addr >> 8) & 0xFF,
		[3] = addr & 0xFF,
		[4] = 0,
	};

	rv = ec_spi_wait_status(SPI_SR_WIP, 0, __func__, __LINE__);
	if (rv)
		return rv;

	rv = ec_spi_bus_write(buf, ARRAY_SIZE(buf));
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_bus_write failed\n", __func__);
		return rv;
	}

	rv = ec_spi_bus_read(dest, SPI_SECTOR_SIZE);
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_bus_read failed\n", __func__);
		return rv;
	}

	rv = ec_spi_reset();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_reset failed\n", __func__);
		return rv;
	}

	return 0;
}

/* Read a sector and verify if it has been erased. Returns 0 on success and <0 on error. */
static int ec_spi_verify_erased_sector(uint32_t addr)
{
	int rv, i;
	uint8_t buf[5] = {
		[0] = SPI_FAST_READ, /* SPI Read */
		[1] = (addr >> 16) & 0xFF,
		[2] = (addr >> 8) & 0xFF,
		[3] = addr & 0xFF,
		[4] = 0,
	};
	uint8_t data;

	rv = ec_spi_wait_status(SPI_SR_WIP, 0, __func__, __LINE__);
	if (rv)
		return rv;

	rv = ec_spi_bus_write(buf, ARRAY_SIZE(buf));
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_bus_write failed\n", __func__);
		return rv;
	}

	for (i = 0; i < SPI_SECTOR_SIZE; i++) {
		/* Read the sector byte after byte and compare. */
		rv = ec_spi_bus_read(&data, sizeof(data));
		if (rv) {
			printk(BIOS_ERR, "%s: ec_spi_bus_read failed\n", __func__);
			return rv;
		}

		if (data != 0xff) {
			printk(BIOS_ERR, "%s: sector at %x is not erased\n", __func__, addr);
			return -1;
		}
	}

	rv = ec_spi_reset();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_reset failed\n", __func__);
		return rv;
	}

	return 0;
}

/*
 * Program a chip with a given image at given address and size of data.
 * Returns written byte count on success and <0 on error.
 */
static int ec_spi_write_at(uint32_t start, uint8_t *image, size_t size)
{
	int rv;
	uint32_t addr;
	uint8_t buf[6] = {0};

	buf[0] = SPI_AAI_WP;
	buf[1] = (start >> 16) & 0xff;
	buf[2] = (start >> 8) & 0xff;
	buf[3] = start & 0xff;

	rv = ec_spi_cmd_write_enable();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_cmd_write_enable failed\n", __func__);
		return rv;
	}

	for (addr = start; addr < start + size; addr += 2) {

		/*
		 * ec_spi_cmd_write_enable ends with ec_spi_wait_status which always calls
		 * ec_spi_reset. No need to do it here again. We also reset the SPI
		 * (make the CS# go high) after AAI WP command* by passing the
		 * CMD_SPI_FLAG_DISABLE to ec_spi_bus_write_with_flags.
		 */

		if (addr == start) {
			/* 1st cmd bytes 1,2,3 are the address */
			buf[4] = image[addr];
			buf[5] = image[addr + 1];

			rv = ec_spi_bus_write_with_flags(buf, 6, CMD_SPI_FLAG_DISABLE);
		} else {
			buf[1] = image[addr];
			buf[2] = image[addr + 1];

			rv = ec_spi_bus_write_with_flags(buf, 3, CMD_SPI_FLAG_DISABLE);
		}

		if (rv) {
			printk(BIOS_ERR, "%s: ec_spi_bus_write failed, addr 0x%06x\n",
			       __func__, addr);
			return rv;
		}

		/*
		 * From the experiments it looked like the busy bit is never set.
		 * It is still dangerous to not probe the bit, however, AAI programming
		 * is quite picky and stops after first 2 bytes if we interrupt the
		 * process with different command than AAI word program.
		 *
		 * rv = ec_spi_wait_status(SPI_SR_WIP, 0, __func__, __LINE__);
		 * if (rv) return rv;
		 */
	}

	rv = ec_spi_cmd_write_disable();
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_cmd_write_disable failed\n", __func__);
		return rv;
	}

	return addr - start;
}

/*
 * Program an entire chip with a given image.
 * Returns written byte count on success and <0 on error.
 */
static int ec_spi_image_write(uint8_t *image, size_t size)
{
	uint8_t *sector;
	uint32_t addr;
	uint32_t erase_addr = CONFIG_EC_SYSTEM76_EC_FLASH_SIZE;
	size_t length;
	int rv;

	sector = malloc(SPI_SECTOR_SIZE);
	if (!sector)
		return -1;

	rv = 0;
	addr = 0;

	while ((addr < CONFIG_EC_SYSTEM76_EC_FLASH_SIZE) &&
	       ((addr + SPI_SECTOR_SIZE) < size)) {
		rv = ec_spi_read_sector(sector, addr);
		if (rv) {
			printk(BIOS_ERR, "%s: ec_spi_read_sector failed, addr 0x%06x\n",
			       __func__, addr);
			goto cleanup;
		}

		if (!memcmp(sector, image + addr, SPI_SECTOR_SIZE)) {
			printk(BIOS_SPEW, "%s: Skipping identical sector, addr 0x%06x\n",
			       __func__, addr);
			addr += SPI_SECTOR_SIZE;
			continue;
		}

		rv = ec_spi_erase_sector(addr);
		if (rv) {
			printk(BIOS_ERR, "%s: ec_spi_erase_sector failed, addr 0x%06x\n",
			       __func__, addr);
			if (CONFIG(VBOOT))
				vboot_fail_and_reboot(vboot_get_context(),
						      VB2_RECOVERY_EC_SOFTWARE_SYNC,
						      EC_UPDATE_ERR_ERASE);
			goto cleanup;
		}

		rv = ec_spi_verify_erased_sector(addr);
		if (rv) {
			printk(BIOS_ERR, "%s: ec_spi_verify_erased_sector failed, addr 0x%06x\n",
			       __func__, addr);
			if (CONFIG(VBOOT))
				vboot_fail_and_reboot(vboot_get_context(),
						      VB2_RECOVERY_EC_SOFTWARE_SYNC,
						      EC_UPDATE_ERR_ERASE);
			goto cleanup;
		}

		rv = ec_spi_write_at(addr, image, SPI_SECTOR_SIZE);
		if (rv < 0) {
			printk(BIOS_ERR, "%s: ec_spi_write_at failed, addr 0x%06x (%d)\n",
			       __func__, addr, rv);
			if (CONFIG(VBOOT))
				vboot_fail_and_reboot(vboot_get_context(),
						      VB2_RECOVERY_EC_SOFTWARE_SYNC,
						      EC_UPDATE_ERR_PROGRAM);
			goto cleanup;
		}

		addr += SPI_SECTOR_SIZE;
	}

	/* Update any remainder bytes */
	rv = ec_spi_read_sector(sector, addr);
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_read_sector failed, last addr 0x%06x\n",
		       __func__, addr);
		goto cleanup;
	}

	erase_addr = addr + SPI_SECTOR_SIZE;

	if (size % SPI_SECTOR_SIZE == 0 && addr < CONFIG_EC_SYSTEM76_EC_FLASH_SIZE)
		length = SPI_SECTOR_SIZE;
	else
		length = size % SPI_SECTOR_SIZE;

	if (memcmp(sector, image + addr, length)) {
		rv = ec_spi_erase_sector(addr);
		if (rv) {
			printk(BIOS_ERR, "%s: ec_spi_erase_sector failed, addr 0x%06x\n",
			       __func__, addr);
			if (CONFIG(VBOOT))
				vboot_fail_and_reboot(vboot_get_context(),
						      VB2_RECOVERY_EC_SOFTWARE_SYNC,
						      EC_UPDATE_ERR_ERASE);
			goto cleanup;
		}

		rv = ec_spi_write_at(addr, image, length);
		if (rv < 0) {
			printk(BIOS_ERR, "%s: ec_spi_write_at failed, addr 0x%06x (%d)\n",
			       __func__, addr, rv);
			if (CONFIG(VBOOT))
				vboot_fail_and_reboot(vboot_get_context(),
						      VB2_RECOVERY_EC_SOFTWARE_SYNC,
						      EC_UPDATE_ERR_PROGRAM);
			goto cleanup;
		}

		addr += length;
	} else {
		printk(BIOS_SPEW, "%s: Skipping identical sector, addr 0x%06x\n",
				  __func__, addr);
		addr += length;
	}

	/* Erase remaining sectors if any */
	while (erase_addr < CONFIG_EC_SYSTEM76_EC_FLASH_SIZE) {
		rv = ec_spi_erase_sector(erase_addr);
		if (rv) {
			printk(BIOS_ERR, "%s: ec_spi_erase_sector failed, addr 0x%06x\n",
			       __func__, erase_addr);
			if (CONFIG(VBOOT))
				vboot_fail_and_reboot(vboot_get_context(),
						      VB2_RECOVERY_EC_SOFTWARE_SYNC,
						      EC_UPDATE_ERR_ERASE);
			goto cleanup;
		}
		erase_addr += SPI_SECTOR_SIZE;
	}

cleanup:
	free(sector);

	return rv ? rv : addr;
}

/* Verify an image sector by sector. Returns 0 on success and <0 on error. */
static int ec_spi_image_verify(uint8_t *image, ssize_t image_sz)
{
	uint8_t *sector;
	uint32_t addr;
	int rv;
	int error = 0;

	sector = malloc(SPI_SECTOR_SIZE);
	if (!sector)
		return -1;

	rv = 0;
	addr = 0;

	while ((addr < CONFIG_EC_SYSTEM76_EC_FLASH_SIZE) &&
	       ((addr + SPI_SECTOR_SIZE) < image_sz)) {
		rv = ec_spi_read_sector(sector, addr);
		if (rv) {
			printk(BIOS_ERR, "%s: ec_spi_read_sector failed, addr 0x%06x\n",
			       __func__, addr);
			return rv;
		}

		rv = -memcmp(sector, image + addr, SPI_SECTOR_SIZE);
		if (rv) {
			printk(BIOS_ERR, "%s: failed to verify sector, addr 0x%06x\n",
			       __func__, addr);
			error = rv;
		}

		addr += SPI_SECTOR_SIZE;
	}

	if (addr == image_sz)
		goto exit;

	rv = ec_spi_read_sector(sector, addr);
	if (rv) {
		printk(BIOS_ERR, "%s: ec_spi_read_sector failed, last addr 0x%06x\n",
		       __func__, addr);
		goto exit;
	}

	rv = -memcmp(sector, image + addr, image_sz % SPI_SECTOR_SIZE);
	if (rv) {
		printk(BIOS_ERR, "%s: failed to verify last sector, addr 0x%06x size 0x%lx\n",
		       __func__, addr, image_sz % SPI_SECTOR_SIZE);
		goto exit;
	}

exit:
	free(sector);

	if (error)
		return error;

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
		printk(BIOS_INFO, "EC: skipping update in recovery mode.\n");
		return;
	}

	image = malloc(CONFIG_EC_SYSTEM76_EC_FLASH_SIZE);
	if (image == NULL) {
		printk(BIOS_ERR, "EC: failed to allocate memory for update.");
		return;
	}

	image_sz = cbfs_load("ec.rom", image, CONFIG_EC_SYSTEM76_EC_FLASH_SIZE);
	if (!image_sz) {
		printk(BIOS_ERR, "EC: failed to load update from CBFS.");
		goto cleanup;
	}

	printk(BIOS_INFO, "EC: update found (%ld bytes)\n", image_sz);

	/* Sanity checks */
	if (image_sz % 2 || image_sz > CONFIG_EC_SYSTEM76_EC_FLASH_SIZE) {
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

	printk(BIOS_INFO, "EC: update target: %s\n", img_board_str);
	printk(BIOS_INFO, "EC: update version: %s\n", img_version_str);

	system76_ec_read_board((uint8_t *)cur_board_str);
	system76_ec_read_version((uint8_t *)cur_version_str);

	if (strcmp(img_board_str, cur_board_str)) {
		printk(BIOS_ERR, "EC: update target mismatch detected! Found %s, expected %s\n",
		       img_board_str, cur_board_str);
		goto cleanup;
	}

	printk(BIOS_INFO, "EC: current version: %s\n", cur_version_str);
	if (!strcmp(img_version_str, cur_version_str)) {
		printk(BIOS_INFO, "EC: update not needed.\n");
		goto cleanup;
	} else {
		printk(BIOS_INFO, "EC: update required!\n");
	}

	if ((ec_read(0x10) & 0x01) != 0x01) {
		printk(BIOS_WARNING, "EC: AC adapter not connected, skipping update.\n");
		if (CONFIG(VBOOT))
			vboot_fail_and_reboot(vboot_get_context(),
					      VB2_RECOVERY_EC_SOFTWARE_SYNC,
					      EC_UPDATE_ERR_NO_AC);
		goto cleanup;
	}

	/* Jump to Scratch ROM */
	smfi_cmd = CMD_SPI_FLAG_SCRATCH;
	if (system76_ec_smfi_cmd(CMD_SPI, sizeof(smfi_cmd), &smfi_cmd)) {
		/* If we failed to jump to scratch ROM, then we can probably continue booting. */
		printk(BIOS_ERR, "EC: failed to jump to scratch ROM!\n");
		if (CONFIG(VBOOT))
			vboot_fail_and_reboot(vboot_get_context(),
					      VB2_RECOVERY_EC_SOFTWARE_SYNC,
					      EC_UPDATE_ERR_SCRATCH);
		goto cleanup;
	}

	for (int i = 0; i < MAX_WRITE_RETRY; i++) {
		rv = ec_spi_image_write((uint8_t *)image, image_sz);
		if (rv < 0) {
			/* EC is now in an unknown state. It may still boot from backup. */
			printk(BIOS_ALERT, "EC: update failed!\n");
		} else {
			printk(BIOS_INFO, "EC: wrote %x bytes\n", rv);
		}

		rv = ec_spi_image_verify((uint8_t *)image, image_sz);
		if (rv < 0) {
			printk(BIOS_ALERT, "EC: update verification failed! (try: %d)\n",
			       i + 1);
		} else {
			printk(BIOS_INFO, "EC: update verified.\n");
			rv = 0;
			break;
		}
	}

	if (rv < 0) {
		/* EC is now in an unknown state. It may still boot from backup. */
		printk(BIOS_ALERT, "EC: update failed!\n");
		if (CONFIG(VBOOT))
			vboot_fail_and_reboot(vboot_get_context(),
					VB2_RECOVERY_EC_SOFTWARE_SYNC,
					EC_UPDATE_ERR_PROGRAM);
		goto cleanup;
	}

	smfi_cmd = CMD_SPI_FLAG_DISABLE;
	if (system76_ec_smfi_cmd(CMD_SPI, sizeof(smfi_cmd), &smfi_cmd)) {
		printk(BIOS_ERR, "EC: failed to disable SPI bus!\n");
		goto cleanup;
	}

	smfi_cmd = 0;
	if (system76_ec_smfi_cmd(CMD_RESET, sizeof(smfi_cmd), &smfi_cmd))
		printk(BIOS_ERR, "EC: failed to trigger reset!\n");

cleanup:
	free(image);
	return;
}

BOOT_STATE_INIT_ENTRY(BS_POST_DEVICE, BS_ON_ENTRY, system76_ec_fw_sync, NULL);
