/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * IPMI "MDR2" (Managed Data Region v2) push-agent commands.
 *
 * These forward the platform's SMBIOS table to the BMC over the "VGA shared
 * memory" window exposed by ASPEED BMC SoCs, instead of shipping every byte
 * through the (narrow, slow) IPMI KCS/BT transport: the host writes the
 * table into BMC memory directly through the ASPEED P2A bridge
 * (see ast_ahb.h), and only uses small IPMI commands to tell the BMC where
 * the data is and how large/valid it is.
 *
 * This mirrors the OEM commands implemented by OpenBMC's intel-ipmi-oem
 * (src/smbiosmdrv2handler.cpp, include/smbiosmdrv2.hpp) and consumed by its
 * smbios-mdr daemon (include/mdrv2.hpp), which together expose the
 * resulting table over D-Bus/Redfish/the web UI. Constants below are named
 * after their counterparts in those files so the protocol can be cross
 * checked against the BMC-side implementation.
 */

#ifndef __DRIVERS_IPMI_SMBIOS_MDRV2_H__
#define __DRIVERS_IPMI_SMBIOS_MDRV2_H__

#include <types.h>

#include "ipmi_if.h"

/* netFnApp == netFnOemEight in phosphor-ipmi-host's ipmid/api.hpp NetFn enum */
#define IPMI_NETFN_MDR2			0x3e

/* cmdMdrII{DataStart,SendDataBlock,DataDone} in oemcommands.hpp */
#define IPMI_CMD_MDR2_DATA_START	0x3b
#define IPMI_CMD_MDR2_SEND_DATA_BLOCK	0x3d
#define IPMI_CMD_MDR2_DATA_DONE		0x3c

/* smbiosAgentId in smbiosmdrv2.hpp */
#define MDR2_SMBIOS_AGENT_ID		0x0101

/*
 * Fixed 16-byte "Data Set ID" both the BMC's smbios-mdr daemon
 * (smbiosTableId in smbios-mdr/include/mdrv2.hpp) and intel-ipmi-oem's
 * static directory entry (smbiosmdrv2handler.hpp) use to identify the
 * SMBIOS table entry.
 */
#define MDR2_SMBIOS_DATA_ID \
	{ 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 0x42 }

/*
 * Base physical (BMC AHB) address of the VGA shared-memory window used to
 * transfer MDR2 data (mdr2SMBaseAddress in smbiosmdrv2.hpp). The BMC always
 * reads a data block back from the start of the window it mapped for the
 * current session, so the host always writes each block there, regardless
 * of that block's final offset in the assembled table.
 */
#define MDR2_SHARED_MEM_BASE		0x9ff00000
#define MDR2_SHARED_MEM_SIZE		0x00100000

/* smbiosTableStorageSize in smbiosmdrv2.hpp: max size of the assembled
   SMBIOS table the BMC will accept/store. */
#define MDR2_SMBIOS_TABLE_MAX_SIZE	(64 * 1024)

struct mdr2_data_id {
	uint8_t data_info[16];
} __packed;

struct mdr2_data_start_req {
	uint16_t agent_id;
	struct mdr2_data_id data_set_info;
	uint32_t data_length;
	uint32_t xfer_address;
	uint32_t xfer_length;
	uint16_t timeout;
} __packed;

struct mdr2_data_start_rsp {
	struct ipmi_rsp resp;
	uint8_t xfer_start_ack;
	uint16_t session_handle;
} __packed;

struct mdr2_send_data_block_req {
	uint16_t agent_id;
	uint16_t lock_handle;
	uint32_t xfer_offset;
	uint32_t xfer_length;
	uint32_t checksum;
} __packed;

struct mdr2_send_data_block_rsp {
	struct ipmi_rsp resp;
	uint8_t terminate;
} __packed;

struct mdr2_data_done_req {
	uint16_t agent_id;
	uint16_t lock_handle;
} __packed;

/*
 * Sends the SMBIOS table currently stored in CBMEM (see smbios_get_table())
 * to the BMC over the ASPEED VGA shared-memory window, using the MDR2
 * "push agent" IPMI commands. Requires an ASPEED BMC graphics device with
 * its P2A bridge enabled (see ast_ahb.h).
 *
 * Returns CB_SUCCESS on success.
 */
enum cb_err ipmi_smbios_mdrv2_send_table(const int port);

#endif /* __DRIVERS_IPMI_SMBIOS_MDRV2_H__ */
