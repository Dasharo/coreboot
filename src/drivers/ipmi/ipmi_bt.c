/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * IPMI specification:
 * https://www.intel.com/content/www/us/en/servers/ipmi/ipmi-intelligent-platform-mgt-interface-spec-2nd-gen-v2-0-spec-update.html
 *
 * LUN seems to be always zero.
 */

#include "ipmi_bt.h"

#include <arch/io.h>
#include <console/console.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <timer.h>

#include "ipmi_if.h"

#define MAX_SIZE         255
#define MAX_PAYLOAD_SIZE (MAX_SIZE - 4)

#define BT_CTRL_INC  0 // Typical address of BT_CTRL is 0xE4
#define HOST2BMC_INC 1 // Typical address of HOST2BMC is 0xE5
#define BMC2HOST_INC 1 // Typical address of BMC2HOST is 0xE5

/* Bits of BT_CTRL */
#define B_BUSY     (1 << 7)
#define H_BUSY     (1 << 6)
#define OEM0       (1 << 5)
#define EVT_ATN    (1 << 4)
#define B2H_ATN    (1 << 3)
#define H2B_ATN    (1 << 2)
#define CLR_RD_PTR (1 << 1)
#define CLR_WR_PTR (1 << 0)

static int wait_for_control_bit(uint16_t port, uint8_t bit, int set)
{
	uint16_t bt_ctrl_port = port + BT_CTRL_INC;
	if (!wait_ms(CONFIG_IPMI_BT_TIMEOUT_MS, ((inb(bt_ctrl_port) & bit) != 0) == set)) {
		printk(BIOS_ERR, "%s(0x%04x, 0x%02x, %d) timeout!\n",
		       __func__, port, bit, set);
		return 1;
	}

	return 0;
}

int ipmi_bt_clear(uint16_t port)
{
	uint8_t bt_ctrl;

	/*
	 * First, set H_BUSY (if not set already) so BMC won't try to write new
	 * commands while we're resetting pointers.
	 */
	if ((inb(port + BT_CTRL_INC) & H_BUSY) == 0)
		outb(H_BUSY, port + BT_CTRL_INC);

	/* If BMC is already in the process of writing, wait until it's done */
	if (wait_for_control_bit(port, B_BUSY, 0))
		return 1;

	bt_ctrl = inb(port + BT_CTRL_INC);

	printk(BIOS_SPEW, "BT_CTRL = %#2.2x\n", bt_ctrl);

	/*
	 * Clear all bits which are already set (they are either toggle bits or
	 * write-1-to-clear) and reset buffer pointers. This also clears H_BUSY.
	 */
	outb(bt_ctrl | CLR_RD_PTR | CLR_WR_PTR, port + BT_CTRL_INC);

	return 0;
}

static int ipmi_bt_send(uint16_t port, uint8_t addr, uint8_t cmd,
			const uint8_t *payload, uint8_t payload_len,
			uint8_t seq_num)
{
	uint16_t i;
	uint16_t len;
	uint8_t buf[MAX_SIZE];

	len = 3 + payload_len;

	buf[0] = len;
	buf[1] = addr;
	buf[2] = seq_num;
	buf[3] = cmd;
	memcpy(&buf[4], payload, payload_len);

	/* Wait for BMC to be available */
	if (wait_for_control_bit(port, B_BUSY, 0))
		return 1;

	/* Clear write pointer */
	outb(CLR_WR_PTR, port + BT_CTRL_INC);

	/* Send our message */
	for (i = 0; i < len + 1; ++i)
		outb(buf[i], port + HOST2BMC_INC);

	/* Tell BMC to process the data */
	outb(H2B_ATN, port + BT_CTRL_INC);

	return 0;
}

static int ipmi_bt_recv(uint16_t port, uint8_t addr, uint8_t cmd,
			uint8_t *response, uint8_t response_len,
			uint8_t seq_num)
{
	uint16_t i;
	uint16_t len;
	uint8_t buf[MAX_SIZE];

	/* Wait for BMC's response */
	if (wait_for_control_bit(port, B2H_ATN, 1))
		return -1;

	/* Tell BMC that host is busy */
	outb(H_BUSY, port + BT_CTRL_INC);

	/* Acknowledge that response is being processed */
	outb(B2H_ATN, port + BT_CTRL_INC);

	/* Clear read pointer */
	outb(CLR_RD_PTR, port + BT_CTRL_INC);

	/* Receive response */
	len = inb(port + BMC2HOST_INC);
	for (i = 0; i < len; ++i)
		buf[i] = inb(port + BMC2HOST_INC);

	/* Indicate that the host is done working with the buffer */
	outb(H_BUSY, port + BT_CTRL_INC);

	if (buf[0] != addr) {
		printk(BIOS_ERR,
		       "Invalid NETFN/LUN field in IPMI BT response: 0x%02x instead of 0x%02x\n",
		       buf[0], addr);
		goto error;
	}
	if (buf[1] != seq_num) {
		printk(BIOS_ERR,
		       "Invalid SEQ field in IPMI BT response: 0x%02x instead of 0x%02x\n",
		       buf[1], seq_num);
		goto error;
	}
	if (buf[2] != cmd) {
		printk(BIOS_ERR,
		       "Invalid CMD field in IPMI BT response: 0x%02x instead of 0x%02x\n",
		       buf[2], cmd);
		goto error;
	}

	if (response_len < len)
		len = response_len;

	/*
	 * Copy response skipping sequence number to match KCS messages.
	 * Sequence number is really an implementation detail anyway.
	 */
	if (response_len != 0)
		response[0] = buf[0];
	memcpy(&response[1], &buf[2], len - 1);

	return len;

error:
	printk(BIOS_ERR, "  IPMI response length field: 0x%02x\n", len);
	printk(BIOS_ERR, "  IPMI netfn/lun: 0x%02x\n", addr);
	printk(BIOS_ERR, "  IPMI SEQ: 0x%02x\n", seq_num);
	printk(BIOS_ERR, "  IPMI command: 0x%02x\n", cmd);
	return -1;
}

int ipmi_message(int port, int netfn, int lun, int cmd,
		 const uint8_t *payload, int payload_len,
		 uint8_t *response, int response_len)
{
	static uint8_t seq_num = 0xff;

	uint8_t addr;

	assert(payload_len >= 0 && payload_len < MAX_PAYLOAD_SIZE);
	assert(netfn >= 0 && netfn <= 0x3f);
	assert(lun >= 0 && lun <= 0x3);

	addr = (netfn << 2) | (lun & 0x3);
	if (ipmi_bt_send(port, addr, cmd, payload, payload_len, ++seq_num)) {
		printk(BIOS_ERR, "Failed to send IPMI BT command 0x%02x\n", cmd);
		return -1;
	}

	addr = ((netfn + 1) << 2) | (lun & 0x3);
	return ipmi_bt_recv(port, addr, cmd, response, response_len, seq_num);
}
