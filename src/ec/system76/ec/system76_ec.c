/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <arch/io.h>
#include <console/system76_ec.h>
#include <pc80/mc146818rtc.h>
#include <timer.h>
#include "acpi.h"
#include "commands.h"

// This is the command region for System76 EC firmware. It must be
// enabled for LPC in the mainboard.
#define SYSTEM76_EC_BASE 0x0E00
#define SYSTEM76_EC_SIZE 256

#define REG_CMD 0
#define REG_RESULT 1
#define REG_DATA 2	// Start of command data

#define CMOS_KBD_BKL		0x80
#define CMOS_KBD_RGB_LED	0x81

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

static void system76_ec_restore_kbd_backlight(void *unused)
{
	uint32_t kbd_led;
	uint8_t cmd[4];

	/* Do not restore the backlight if lid is closed */
	if (system76_ec_get_lid_state() != 1)
		return;

	/* Set last keyboard LED brightness */
	cmd[0] = CMD_LED_INDEX_ALL;
	cmd[1] = cmos_read(CMOS_KBD_BKL);
	system76_ec_smfi_cmd(CMD_LED_SET_VALUE, 2, cmd);

	kbd_led = cmos_read32(CMOS_KBD_RGB_LED);

	cmd[0] = CMD_LED_INDEX_ALL;
	cmd[1] = (kbd_led & 0xff0000) >> 16; // R
	cmd[2] = (kbd_led & 0x00ff00) >> 8; // G
	cmd[3] = (kbd_led & 0x0000ff); // B
	system76_ec_smfi_cmd(CMD_LED_SET_COLOR, 4, cmd);
}

BOOT_STATE_INIT_ENTRY(BS_DEV_INIT, BS_ON_EXIT,
		      system76_ec_restore_kbd_backlight, NULL);
