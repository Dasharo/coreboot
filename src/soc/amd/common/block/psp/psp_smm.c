/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/mmio.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <region_file.h>
#include <console/console.h>
#include <amdblocks/psp.h>
#include <amdblocks/smi.h>
#include <dasharo/options.h>
#include <soc/iomap.h>
#include <spi_flash.h>
#include <string.h>

#include "psp_def.h"
#include "psp_rom_armor_apmc.h"

/*
 * When sending PSP mailbox commands to the PSP from the SMI handler after the boot done
 * command was sent, the corresponding data buffer needs to be placed in this core to PSP (C2P)
 * buffer.
 */
struct {
	uint8_t buffer[C2P_BUFFER_MAXSIZE];
} __aligned(32) c2p_buffer;

/*
 * When the PSP sends mailbox commands to the host, it will update the PSP to core (P2C) buffer
 * and then send an SMI to the host to process the request.
 */
struct {
	uint8_t buffer[P2C_BUFFER_MAXSIZE];
} __aligned(32) p2c_buffer;

/*
 * When sending PSP mailbox commands to the PSP from the SMI handler, the SMM flag needs to be
 * set for the PSP to accept it. Otherwise it should be cleared.
 */
static uint32_t smm_flag;

void psp_set_smm_flag(void)
{
	smm_flag = 1;
}

void psp_clear_smm_flag(void)
{
	smm_flag = 0;
}

__weak const struct psp_rom_armor1_whitelist *soc_get_psp_rom_armor_whitelist(void)
{
	return NULL;
}

/*
 * The MBOX_BIOS_CMD_SMM_INFO PSP mailbox command doesn't necessarily need be sent from SMM,
 * but doing so allows the linker to sort out the addresses of c2p_buffer, p2c_buffer and
 * smm_flag without us needing to pass this info between ramstage and smm. In the PSP gen2 case
 * this will also make sure that the PSP MMIO base will be cached in SMM before the OS takes
 * over so no SMN accesses will be needed during OS runtime.
 */
int psp_notify_smm(void)
{
	msr_t msr;
	int cmd_status;
	struct mbox_cmd_smm_info_buffer buffer = {
		.header = {
			.size = sizeof(buffer)
		},
		.req = {
			.psp_smm_data_region = (uintptr_t)p2c_buffer.buffer,
			.psp_smm_data_length = sizeof(p2c_buffer),
			.psp_mbox_smm_buffer_address = (uintptr_t)c2p_buffer.buffer,
			.psp_mbox_smm_flag_address = (uintptr_t)&smm_flag,
		}
	};

	msr = rdmsr(SMM_ADDR_MSR);
	buffer.req.smm_base = msr.raw;
	msr = rdmsr(SMM_MASK_MSR);
	msr.lo &= 0xffff0000; /* mask SMM_TSEG_VALID and reserved bits */
	buffer.req.smm_mask = msr.raw;

	soc_fill_smm_trig_info(&buffer.req.smm_trig_info);
#if (CONFIG(SOC_AMD_COMMON_BLOCK_PSP_GEN2))
	soc_fill_smm_reg_info(&buffer.req.smm_reg_info);
#endif

	if (CONFIG(SOC_AMD_COMMON_BLOCK_PSP_SMI)) {
		/*
		 * If ROM Armor 3 is compiled but disabled by variable, we
		 * have to configure PSP SMIs so that other things will work,
		 * like fTPM. PSP SMIs have to be configured in every other
		 * case too.
		 */
		if (!(CONFIG(SOC_AMD_COMMON_BLOCK_PSP_ROM_ARMOR3) &&
		      is_smm_bwp_permitted() && rom_armor_enforced)) {
			configure_psp_smi();
			enable_psp_smi();

			/* Probe for SPI flash now as it's likely not busy */
			assert(boot_device_spi_flash());
		}
	}

	printk(BIOS_DEBUG, "PSP: Notify SMM info... ");

	cmd_status = send_psp_command(MBOX_BIOS_CMD_SMM_INFO, &buffer);

	/* buffer's status shouldn't change but report it if it does */
	psp_print_cmd_status(cmd_status, &buffer.header);

	return cmd_status;
}

int psp_rom_armor_enter_smm_mode(void *param, size_t *flash_size)
{
	int cmd_status;
	struct rom_armor_params_init *params = param;
	struct mbox_buffer_header *header;
	*flash_size = 0;

	/* Initialize and send ROM Armor Enter SMM Mode command */
	struct mbox_rom_armor_enforce_buffer enforce_buffer3 = {
		.header.size = sizeof(enforce_buffer3),
		.capsule_update = params->capsule_update,
	};

	struct mbox_rom_armor1_buffer enforce_buffer1 = {
		.header.size = sizeof(enforce_buffer1),
		.tseg_addr = params->operation_buf,
		.chip_select = params->chip_select,
	};

	printk(BIOS_DEBUG, "PSP: Entering ROM Armor SMM-only mode...");

	if (CONFIG(SOC_AMD_COMMON_BLOCK_PSP_ROM_ARMOR1)) {
		cmd_status = send_psp_command(MBOX_BIOS_CMD_ARMOR_ENTER_SMM_MODE, &enforce_buffer1);
		header = &enforce_buffer1.header;
	} else {
		cmd_status = send_psp_command(MBOX_BIOS_CMD_ARMOR_ENTER_SMM_MODE2, &enforce_buffer3);
		header = &enforce_buffer3.header;
	}

	psp_print_cmd_status(cmd_status, header);

	if (cmd_status || header->status)
		return -1;

	if (CONFIG(SOC_AMD_COMMON_BLOCK_PSP_ROM_ARMOR3))
		*flash_size = enforce_buffer3.flash_size;
	else
		*flash_size = CONFIG_ROM_SIZE; /* For ROM Armor 3 API compatibility */

	return 0;
}

int psp_rom_armor_enforce_whitelist(void *param, uint8_t spi_freq)
{
	int cmd_status;
	struct rom_armor_params_init *params = param;
	struct psp_rom_armor1_whitelist *whitelist_ptr;
	struct mbox_rom_armor1_buffer whitelist_buffer = {
		.header.size = sizeof(whitelist_buffer),
		.tseg_addr = params->operation_buf,
		.chip_select = params->chip_select,
	};
	const struct psp_rom_armor1_whitelist *whitelist = soc_get_psp_rom_armor_whitelist();

	if (!whitelist) {
		printk(BIOS_DEBUG, "PSP: ROM Armor SPI whitelist not found\n");
		return 0;
	}

	memset((void *)whitelist_buffer.tseg_addr, 0, 4 * KiB);
	memcpy((void *)whitelist_buffer.tseg_addr, whitelist, sizeof(*whitelist));
	whitelist_ptr = (struct psp_rom_armor1_whitelist *)whitelist_buffer.tseg_addr;

	/* Update command frequency used by the controller */
	for (size_t i = 0;
	     i < whitelist_ptr->allowed_cmd_count && i < PSP_MAX_WHITE_LIST_CMD_NUM;
	     i++) {
		whitelist_ptr->allowed_cmds[i].freq = spi_freq;
	}

	printk(BIOS_DEBUG, "PSP: Enforcing ROM Armor whitelist...");

	cmd_status = send_psp_command(MBOX_BIOS_CMD_ARMOR_ENFORCE_WHITELIST, &whitelist_buffer);
	psp_print_cmd_status(cmd_status, &whitelist_buffer.header);

	if (cmd_status || whitelist_buffer.header.status)
		return -1;

	return 0;
}

int psp_rom_armor3_spi_transaction(const struct mbox_rom_armor_flash_command *cmd_buf)
{
	int cmd_status;
	struct mbox_rom_armor_flash_command_buffer *buffer;

	/* PSP verifies that this buffer is at the address specified in psp_notify_smm() */
	buffer = (struct mbox_rom_armor_flash_command_buffer *)c2p_buffer.buffer;
	assert(buffer);
	assert(cmd_buf);

	buffer->header.size = sizeof(*buffer);
	buffer->header.status = 0; /* Clear status before sending command */
	memcpy(&buffer->cmd, cmd_buf, sizeof(*cmd_buf));

	/* Sanity checks */
	assert(buffer->cmd.transaction);
	assert(buffer->cmd.buffer_ptr);
	assert(buffer->cmd.size);

	if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_DEBUG))
		printk(BIOS_SPEW, "PSP: Sending transaction type=%u offset=0x%x size=0x%x buffer_ptr=0x%llx read_back=0x%x\n",
		       buffer->cmd.transaction, buffer->cmd.offset, buffer->cmd.size,
		       buffer->cmd.buffer_ptr, buffer->cmd.read_back);

	asm volatile ("sfence");

	/* Send command to PSP */
	cmd_status = send_psp_command(MBOX_BIOS_CMD_ARMOR_SPI_TRANSACTION, buffer);
	if (cmd_status || buffer->header.status) {
		psp_print_cmd_status(cmd_status, &buffer->header);
		return cmd_status ? cmd_status : buffer->header.status;
	}

	return 0;
}

/* Notify PSP the system is going to a sleep state. */
void psp_notify_sx_info(uint8_t sleep_type)
{
	int cmd_status;
	struct mbox_cmd_sx_info_buffer *buffer;

	/* PSP verifies that this buffer is at the address specified in psp_notify_smm() */
	buffer = (struct mbox_cmd_sx_info_buffer *)c2p_buffer.buffer;
	memset(buffer, 0, sizeof(*buffer));
	buffer->header.size = sizeof(*buffer);

	if (sleep_type > MBOX_BIOS_CMD_SX_INFO_SLEEP_TYPE_MAX) {
		printk(BIOS_ERR, "PSP: BUG: invalid sleep type 0x%x requested\n", sleep_type);
		return;
	}

	printk(BIOS_DEBUG, "PSP: Prepare to enter sleep state %d... ", sleep_type);

	buffer->sleep_type = sleep_type;

	cmd_status = send_psp_command(MBOX_BIOS_CMD_SX_INFO, buffer);

	/* buffer's status shouldn't change but report it if it does */
	psp_print_cmd_status(cmd_status, &buffer->header);
}
