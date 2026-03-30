/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <soc/amd/common/block/psp/psp_def.h>

#define MBOX_BIOS_CMD_SMM_LOCK			0x6C
#define MBOX_BIOS_CMD_LOCK_DF_REG		0x1B

#define CMD_CONFIG_ID_Z_STATE_ENABLEMENT_STATUS	0x02

/*
 * Send necessary commands after device intiialization so that
 * PSP Exit Boot Services command does not hang the platform.
 */
static void send_psp_commands(void *unused)
{
	uint32_t args[4] = { 0 };

	psp_command_set_config(CMD_CONFIG_ID_Z_STATE_ENABLEMENT_STATUS, args, "Z-State Enablement Status");
	psp_send_generic_command(MBOX_BIOS_CMD_SMM_LOCK, "Locking SMM");
	psp_send_generic_command(MBOX_BIOS_CMD_LOCK_DF_REG, "Locking DF registers");
}

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, send_psp_commands, NULL);
