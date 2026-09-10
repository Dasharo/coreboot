/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <commonlib/helpers.h>
#include <console/console.h>
#include <smbios.h>
#include <string.h>
#include <types.h>

#include <drivers/aspeed/common/ast_ahb.h>

#include "ipmi_if.h"
#include "smbios_mdrv2.h"

static uint32_t mdr2_checksum32(const uint8_t *buf, size_t len)
{
	uint32_t sum = 0;

	while (len--)
		sum += *buf++;

	return sum;
}

static enum cb_err mdr2_data_start(int port, uint32_t table_len, uint16_t *session)
{
	const struct mdr2_data_start_req req = {
		.agent_id = MDR2_SMBIOS_AGENT_ID,
		.data_set_info = { .data_info = MDR2_SMBIOS_DATA_ID },
		.data_length = table_len,
		.xfer_address = 0,
		.xfer_length = MDR2_SMBIOS_TABLE_MAX_SIZE,
		.timeout = 0,
	};
	struct mdr2_data_start_rsp rsp;
	int ret;

	ret = ipmi_message(port, IPMI_NETFN_MDR2, 0, IPMI_CMD_MDR2_DATA_START,
			    (const unsigned char *)&req, sizeof(req),
			    (unsigned char *)&rsp, sizeof(rsp));
	if (ret < (int)sizeof(struct ipmi_rsp) || rsp.resp.completion_code) {
		printk(BIOS_ERR, "IPMI: MDR2 data start failed (ret=%d resp=0x%x)\n",
		       ret, rsp.resp.completion_code);
		return CB_ERR;
	}
	if (ret != sizeof(rsp)) {
		printk(BIOS_ERR, "IPMI: MDR2 data start response truncated\n");
		return CB_ERR;
	}

	*session = rsp.session_handle;

	return CB_SUCCESS;
}

static enum cb_err mdr2_send_data_block(int port, uint16_t session, uint32_t offset,
					 uint32_t len, uint32_t checksum)
{
	const struct mdr2_send_data_block_req req = {
		.agent_id = MDR2_SMBIOS_AGENT_ID,
		.lock_handle = session,
		.xfer_offset = offset,
		.xfer_length = len,
		.checksum = checksum,
	};
	struct mdr2_send_data_block_rsp rsp;
	int ret;

	ret = ipmi_message(port, IPMI_NETFN_MDR2, 0, IPMI_CMD_MDR2_SEND_DATA_BLOCK,
			    (const unsigned char *)&req, sizeof(req),
			    (unsigned char *)&rsp, sizeof(rsp));
	if (ret < (int)sizeof(struct ipmi_rsp) || rsp.resp.completion_code) {
		printk(BIOS_ERR, "IPMI: MDR2 send data block failed (ret=%d resp=0x%x)\n",
		       ret, rsp.resp.completion_code);
		return CB_ERR;
	}

	return CB_SUCCESS;
}

static enum cb_err mdr2_data_done(int port, uint16_t session)
{
	const struct mdr2_data_done_req req = {
		.agent_id = MDR2_SMBIOS_AGENT_ID,
		.lock_handle = session,
	};
	struct ipmi_rsp rsp;
	int ret;

	ret = ipmi_message(port, IPMI_NETFN_MDR2, 0, IPMI_CMD_MDR2_DATA_DONE,
			    (const unsigned char *)&req, sizeof(req),
			    (unsigned char *)&rsp, sizeof(rsp));
	if (ret < (int)sizeof(struct ipmi_rsp) || rsp.completion_code) {
		printk(BIOS_ERR, "IPMI: MDR2 data done failed (ret=%d resp=0x%x)\n",
		       ret, rsp.completion_code);
		return CB_ERR;
	}

	return CB_SUCCESS;
}

enum cb_err ipmi_smbios_mdrv2_send_table(const int port)
{
	void *table;
	size_t table_len;
	size_t sent;
	uint16_t session;
	enum cb_err ret;

	table = smbios_get_table(&table_len);
	if (!table || !table_len) {
		printk(BIOS_ERR, "IPMI: MDR2: no SMBIOS table to forward to BMC\n");
		return CB_ERR;
	}

	if (table_len > MDR2_SMBIOS_TABLE_MAX_SIZE) {
		printk(BIOS_ERR,
		       "IPMI: MDR2: SMBIOS table (%zu bytes) exceeds BMC storage (%u bytes)\n",
		       table_len, MDR2_SMBIOS_TABLE_MAX_SIZE);
		return CB_ERR;
	}

	if (mdr2_data_start(port, table_len, &session) != CB_SUCCESS)
		return CB_ERR;

	ret = CB_SUCCESS;
	for (sent = 0; sent < table_len; sent += MDR2_SHARED_MEM_SIZE) {
		size_t chunk = MIN(table_len - sent, MDR2_SHARED_MEM_SIZE);
		const uint8_t *chunk_data = (const uint8_t *)table + sent;
		uint32_t checksum;

		if (ast_ahb_write_buffer(MDR2_SHARED_MEM_BASE, chunk_data, chunk) != CB_SUCCESS) {
			printk(BIOS_ERR,
			       "IPMI: MDR2: failed to write SMBIOS table to BMC shared memory\n");
			ret = CB_ERR;
			break;
		}

		checksum = mdr2_checksum32(chunk_data, chunk);

		if (mdr2_send_data_block(port, session, sent, chunk, checksum) != CB_SUCCESS) {
			ret = CB_ERR;
			break;
		}
	}

	/* Always close out the session, even if a block failed to send, so the
	   BMC-side entry doesn't stay locked. */
	if (mdr2_data_done(port, session) != CB_SUCCESS)
		ret = CB_ERR;

	if (ret == CB_SUCCESS)
		printk(BIOS_INFO, "IPMI: MDR2: forwarded %zu bytes of SMBIOS table to BMC\n",
		       table_len);

	return ret;
}

static void ipmi_smbios_mdrv2_boot_callback(void *unused)
{
	/* IPMI_BT depends on !IPMI_KCS, so exactly one of these is defined. */
#if CONFIG(IPMI_KCS)
	const int port = CONFIG_BMC_KCS_BASE;
#elif CONFIG(IPMI_BT)
	const int port = CONFIG_BMC_BT_BASE;
#endif

	ipmi_smbios_mdrv2_send_table(port);
}

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_ENTRY, ipmi_smbios_mdrv2_boot_callback, NULL);
