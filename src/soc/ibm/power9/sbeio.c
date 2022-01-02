/* SPDX-License-Identifier: GPL-2.0-only */

#include "sbeio.h"

#include <console/console.h>
#include <delay.h>
#include <stdbool.h>
#include <stddef.h>

#include "fsi.h"

enum fifo_regs {
	SBE_FIFO_UPFIFO_DATA_IN    = 0x00002400,
	SBE_FIFO_UPFIFO_STATUS     = 0x00002404,
	SBE_FIFO_UPFIFO_SIG_EOT    = 0x00002408,
	SBE_FIFO_UPFIFO_REQ_RESET  = 0x0000240C,
	SBE_FIFO_DNFIFO_DATA_OUT   = 0x00002440,
	SBE_FIFO_DNFIFO_STATUS     = 0x00002444,
	SBE_FIFO_DNFIFO_RESET      = 0x00002450,
	SBE_FIFO_DNFIFO_ACK_EOT    = 0x00002454,
	SBE_FIFO_DNFIFO_MAX_TSFR   = 0x00002458,
};

enum {
	SBE_FIFO_CLASS_SCOM_ACCESS = 0xA2,
	SBE_FIFO_CMD_GET_SCOM = 0x01,
	SBE_FIFO_CMD_PUT_SCOM = 0x02,

	FSB_FIFO_SIG_EOT = 0x80000000,
	MAX_FIFO_TIMEOUT_US = 2 * 1000 * 1000, // Hostboot waits up to 90s!

	FIFO_STATUS_MAGIC = 0xC0DE,
};

struct get_scom_request_t {
	uint32_t word_count;	// size in uint32_t (4)
	uint16_t reserved;	// 0
	uint8_t  cmd_class;	// SBE_FIFO_CLASS_SCOM_ACCESS
	uint8_t  cmd;		// SBE_FIFO_CMD_GET_SCOM
	uint64_t addr;
} __attribute__((packed));

struct put_scom_request_t {
	uint32_t word_count;	// size in uint32_t (6)
	uint16_t reserved;	// 0
	uint8_t  cmd_class;	// SBE_FIFO_CLASS_SCOM_ACCESS
	uint8_t  cmd;		// SBE_FIFO_CMD_PUT_SCOM
	uint64_t addr;
	uint64_t data;
} __attribute__((packed));

/* This structure is part of every response */
struct status_hdr_t {
	uint16_t  magic;	// FIFO_STATUS_MAGIC
	uint8_t   cmd_class;
	uint8_t   cmd;
	uint16_t  primary_status;
	uint16_t  secondary_status;
} __attribute__((packed));

static void fifo_push(uint8_t chip, uint32_t addr, uint32_t data)
{
	enum { UPFIFO_STATUS_FIFO_FULL = 0x00200000 };

	uint64_t elapsed_time_us = 0;

	while (true) {
		uint32_t status = read_fsi(chip, SBE_FIFO_UPFIFO_STATUS);
		if (!(status & UPFIFO_STATUS_FIFO_FULL))
			break;

		if (elapsed_time_us >= MAX_FIFO_TIMEOUT_US)
			die("Timeout waiting for upstream SBE FIFO to be not full");

		udelay(10);
		elapsed_time_us += 10;
	}

	write_fsi(chip, addr, data);
}

static void write_request(uint8_t chip, const void *request, uint32_t word_count)
{
	const uint32_t *words = request;

	/*
	 * Ensure Downstream Max Transfer Counter is 0 since we have no need for
	 * it and non-0 can cause protocol issues.
	 */
	write_fsi(chip, SBE_FIFO_DNFIFO_MAX_TSFR, 0x0);

	for (uint32_t i = 0; i < word_count; i++)
		fifo_push(chip, SBE_FIFO_UPFIFO_DATA_IN, words[i]);

	/* Notify SBE that last word has been sent */
	fifo_push(chip, SBE_FIFO_UPFIFO_SIG_EOT, FSB_FIFO_SIG_EOT);
}

/* Returns true when there is no more data to be read */
static bool fifo_pop(uint8_t chip, uint32_t *data)
{
	enum {
		DNFIFO_STATUS_DEQUEUED_EOT_FLAG = 0x00800000,
		DNFIFO_STATUS_FIFO_EMPTY = 0x00100000,
	};

	uint64_t elapsed_time_us = 0;

	while (true) {
		uint32_t status = read_fsi(chip, SBE_FIFO_DNFIFO_STATUS);

		/* If we're done receiving response */
		if (status & DNFIFO_STATUS_DEQUEUED_EOT_FLAG)
			return false;

		/* If there is more data */
		if (!(status & DNFIFO_STATUS_FIFO_EMPTY))
			break;

		if (elapsed_time_us >= MAX_FIFO_TIMEOUT_US) {
			printk(BIOS_INFO, "Last downstream SBE status: 0x%08x\n", status);
			die("Timeout waiting for downstream SBE FIFO to be not empty\n");
		}

		udelay(10);
		elapsed_time_us += 10;
	}

	*data = read_fsi(chip, SBE_FIFO_DNFIFO_DATA_OUT);
	return true;
}

static void read_response(uint8_t chip, void *response, uint32_t word_count)
{
	enum {
		MSG_BUFFER_SIZE = 2048,

		STATUS_SIZE_WORDS = sizeof(struct status_hdr_t) / sizeof(uint32_t),

		SBE_PRI_OPERATION_SUCCESSFUL = 0x00,
		SBE_SEC_OPERATION_SUCCESSFUL = 0x00,
	};

	/* Large enough to receive FFDC */
	static uint32_t buffer[MSG_BUFFER_SIZE];

	uint32_t idx;
	uint32_t offset_idx;
	uint32_t status_idx;
	struct status_hdr_t *status_hdr;

	uint32_t *words = response;

	/*
	 * Message Schema:
	 *  |Return Data (optional)| Status Header | FFDC (optional)
	 *  |Offset to Status Header (starting from EOT) | EOT |
	 */

	for (idx = 0; idx < MSG_BUFFER_SIZE; ++idx) {
		if (!fifo_pop(chip, &buffer[idx]))
			break;

		if (idx < word_count)
			words[idx] = buffer[idx];
	}

	if (idx == MSG_BUFFER_SIZE)
		die("SBE IO response exceeded maximum allowed size\n");

	/* Notify SBE that EOT has been received */
	write_fsi(chip, SBE_FIFO_DNFIFO_ACK_EOT, FSB_FIFO_SIG_EOT);

	/*
	 * Final index for a minimum complete message (No return data and no FFDC):
	 *  Word Length of status header + Length of Offset (1) + Length of EOT (1)
	 */
	if (idx < STATUS_SIZE_WORDS + 2) {
		printk(BIOS_INFO, "Response length in words: 0x%08x\n", idx);
		die("SBE IO response is too short\n");
	}

	/*
	 * |offset to header| EOT marker | current insert pos | <- idx
	 * The offset is how far to move back from from the EOT position to
	 * to get the index of the Status Header.
	 */
	offset_idx = idx - 2;

	/* Validate the offset to the status header */
	if (buffer[offset_idx] - 1 > offset_idx)
		die("SBE response offset is too large\n");
	else if (buffer[offset_idx] < STATUS_SIZE_WORDS + 1)
		die("SBE response offset is too small\n");

	status_idx = offset_idx - (buffer[offset_idx] - 1);
	status_hdr = (struct status_hdr_t *)&buffer[status_idx];

	/* Check status for success */
	if (status_hdr->magic != FIFO_STATUS_MAGIC ||
	    status_hdr->primary_status != SBE_PRI_OPERATION_SUCCESSFUL ||
	    status_hdr->secondary_status != SBE_SEC_OPERATION_SUCCESSFUL)
		die("Invalid status in SBE IO response\n");
}

void write_sbe_scom(uint8_t chip, uint64_t addr, uint64_t data)
{
	struct put_scom_request_t request = {
		.word_count = sizeof(request) / sizeof(uint32_t),
		.cmd_class = SBE_FIFO_CLASS_SCOM_ACCESS,
		.cmd = SBE_FIFO_CMD_PUT_SCOM,
		.addr = addr,
		.data = data,
	};

	write_request(chip, &request, request.word_count);
	read_response(chip, NULL, 0);
}

uint64_t read_sbe_scom(uint8_t chip, uint64_t addr)
{
	uint64_t data;
	struct get_scom_request_t request = {
		.word_count = sizeof(request) / sizeof(uint32_t),
		.cmd_class = SBE_FIFO_CLASS_SCOM_ACCESS,
		.cmd = SBE_FIFO_CMD_GET_SCOM,
		.addr = addr,
	};

	write_request(chip, &request, request.word_count);
	read_response(chip, &data, sizeof(data) / sizeof(uint32_t));

	return data;
}
