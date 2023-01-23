/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <console/system76_ec.h>
#include <timer.h>
#include "commands.h"

// This is the command region for System76 EC firmware. It must be
// enabled for LPC in the mainboard.
#define SYSTEM76_EC_BASE 0x0E00
#define SYSTEM76_EC_SIZE 256

#define REG_CMD 0
#define REG_RESULT 1

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
