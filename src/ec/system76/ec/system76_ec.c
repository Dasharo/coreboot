/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <arch/io.h>
#include <cbfs.h>
#include <console/system76_ec.h>
#include <pc80/mc146818rtc.h>
#include <timer.h>
#include "acpi.h"
#include "commands.h"

// This is the command region for System76 EC firmware. It must be
// enabled for LPC in the mainboard.
#define SYSTEM76_EC_BASE 0x0E00
#define SYSTEM76_EC_SIZE 256

#define SPI_FLASH_SIZE (128 * 1024)
#define SPI_SECTOR_SIZE 1024

#define REG_CMD 0
#define REG_RESULT 1
#define REG_DATA 2	// Start of command data

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

    switch (type){
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
    switch (type){
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

	/* Exit if not found */
	if (key_i < key_len)
		return 0;

	/* Copy the data */
	while (data_i < data_len && ret_i < dest_len - 1 && data[data_i] != 0)
		dest[ret_i++] = data[data_i++];

	/* Terminate the result */
	dest[ret_i] = '\0';

	return ret_i;
}

struct smfi_spi_cmd_hdr {
	uint8_t flags;
	uint8_t len;
} __packed;

static uint32_t flash_read(uint8_t *dest, uint32_t len) {
	struct smfi_spi_cmd_hdr read_cmd = {
		CMD_SPI_FLAG_READ,
		SYSTEM76_EC_SIZE - 4,
	};

	uint32_t addr = 0;
	uint32_t bytes_read = 0;
	int i;

	while (addr < len) {
		wait_us(10000, system76_ec_read(REG_CMD) == CMD_FINISHED);

		if (system76_ec_smfi_cmd(CMD_SPI, sizeof(read_cmd) / sizeof(uint8_t), (uint8_t *)&read_cmd)) {
			printk(BIOS_DEBUG, "EC flash read failed!\n");
			return addr + 1;
		}

		/* Check how many data bytes were actually read */
		bytes_read = system76_ec_read(REG_DATA + 1);

		/* Sanity check */
		if (bytes_read == 0 || bytes_read > SYSTEM76_EC_SIZE - 4)
			return 0;

		/* Read data bytes, index should be valid due to length test above */
		for (i = 0; i < bytes_read; i++)
			dest[addr + i] = system76_ec_read(REG_DATA + 2 + i);

		addr += bytes_read;

		/* Check if this was the last chunk */
		if (bytes_read < SYSTEM76_EC_SIZE - 4)
			break;
	}

	return addr + 1;
}

static void system76_ec_fw_sync(void *unused)
{
	size_t image_sz;
	char *image;
	char img_board_str[64];
	char img_version_str[64];
	char cur_board_str[64];
	char cur_version_str[64];

	/* EC update is loaded from verified CBFS */
	image = cbfs_map("ec.rom", &image_sz);

	if (image == NULL) {
		printk(BIOS_DEBUG, "No EC firmware update found.\n");
		return;
	}

	printk(BIOS_DEBUG, "EC update found (%ld bytes)\n", image_sz);

	/* Sanity checks */
	if (!firmware_str(image, image_sz, "76EC_BOARD=", img_board_str, ARRAY_SIZE(img_board_str))) {
		printk(BIOS_DEBUG, "Could not determine EC update target board.\n");
		return;
	}

	if (!firmware_str(image, image_sz, "76EC_VERSION=", img_version_str, ARRAY_SIZE(img_version_str))) {
		printk(BIOS_DEBUG, "Could not determine EC update version.\n");
		return;
	}

	printk(BIOS_DEBUG, "EC update target: %s\n", img_board_str);
	printk(BIOS_DEBUG, "EC update version: %s\n", img_version_str);

	system76_ec_read_board((uint8_t *)cur_board_str);
	system76_ec_read_version((uint8_t *)cur_version_str);

	if (strcmp(img_board_str, cur_board_str)) {
		printk(BIOS_DEBUG, "EC update mismatch detected! Found %s, expected %s\n",
			img_board_str, cur_board_str);
		return;
	}

	printk(BIOS_DEBUG, "EC current version: %s\n", cur_version_str);
	if (!strcmp(img_version_str, cur_version_str)) {
		printk(BIOS_DEBUG, "EC update not needed.\n");
		return;
	} else {
		printk(BIOS_DEBUG, "EC update required!\n");
	}

	/* Jump to Scratch ROM */
	struct smfi_spi_cmd_hdr smfi_cmd = {
		CMD_SPI_FLAG_SCRATCH,
		0,
	};

	/* If we failed to jump to scratch ROM, then we can probably continue booting. */
	if (system76_ec_smfi_cmd(CMD_SPI, sizeof(smfi_cmd) / sizeof(uint8_t), (uint8_t *)&smfi_cmd)) {
		printk(BIOS_ERR, "EC update failed!\n");
		return;
	}

	uint8_t current_rom[SPI_FLASH_SIZE] = { 0xFF };
	uint32_t current_size;

	current_size = flash_read(current_rom, ARRAY_SIZE(current_rom));
	printk(BIOS_ERR, "EC read %u bytes\n", current_size);

	//smfi_cmd.flags = CMD_SPI_FLAG_DISABLE;

	//if (system76_ec_smfi_cmd(CMD_SPI, sizeof(smfi_cmd) / sizeof(uint8_t), (uint8_t *)&smfi_cmd)) {
	//	printk(BIOS_ERR, "EC update failed!\n");
	//	return;
	//}
}

BOOT_STATE_INIT_ENTRY(BS_PRE_DEVICE, BS_ON_ENTRY, system76_ec_fw_sync, NULL);
