/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __AMD_PSP_DEF_H__
#define __AMD_PSP_DEF_H__

#include <types.h>
#include <commonlib/helpers.h>
#include <amdblocks/psp.h>

#define CORE_2_PSP_MSG_38_OFFSET	0x10998 /* 4 byte */
#define   CORE_2_PSP_MSG_38_FUSE_SPL		BIT(12)
#define   CORE_2_PSP_MSG_38_SPL_FUSE_ERROR	BIT(13)
#define   CORE_2_PSP_MSG_38_SPL_ENTRY_ERROR	BIT(14)
#define   CORE_2_PSP_MSG_38_SPL_ENTRY_MISSING	BIT(15)

#define CORE_2_PSP_MSG_63_OFFSET	0x109fc /* 4 byte */

/* x86 to PSP commands */
#define MBOX_BIOS_CMD_SMM_INFO			0x02
#define MBOX_BIOS_CMD_SX_INFO			0x03
#define   MBOX_BIOS_CMD_SX_INFO_SLEEP_TYPE_MAX	0x07
#define MBOX_BIOS_CMD_RSM_INFO			0x04
#define MBOX_BIOS_CMD_PSP_FTPM_QUERY		0x05
#define   MBOX_FTPM_CAP_TPM_SUPPORTED		(1 << 0)
#define   MBOX_FTPM_CAP_TPM_REQ_FACTORY_RESET	(1 << 1)
#define   MBOX_FTPM_CAP_FTPM_NEED_RECOVER	(1 << 2)
#define MBOX_BIOS_CMD_BOOT_DONE			0x06
#define MBOX_BIOS_CMD_CLEAR_S3_STS		0x07
#define MBOX_BIOS_CMD_S3_DATA_INFO		0x08
#define MBOX_BIOS_CMD_NOP			0x09
#define MBOX_BIOS_CMD_HSTI_QUERY		0x14
#define  HSTI_STATE_ROM_ARMOR_ENFORCED		BIT(11)
#define MBOX_BIOS_CMD_PSB_AUTO_FUSING		0x21
#define MBOX_BIOS_CMD_PSP_CAPS_QUERY		0x27
#define MBOX_BIOS_CMD_ARMOR_ENTER_SMM_MODE	0x28
#define MBOX_BIOS_CMD_ARMOR_ENFORCE_WHITELIST	0x29
#define MBOX_BIOS_CMD_ARMOR_EXECUTE_SPI_CMD	0x2a
#define MBOX_BIOS_CMD_ARMOR_SWITCH_CS_MODE	0x2b
#define MBOX_BIOS_CMD_SET_SPL_FUSE		0x2d
#define MBOX_BIOS_CMD_LOCK_FCH_REG		0x30
#define MBOX_BIOS_CMD_SET_RPMC_ADDRESS		0x39
#define MBOX_BIOS_CMD_LOCK_FCH_GPIO		0x3A
#define MBOX_BIOS_CMD_SEND_IVRS_ACPI_TABLE	0x3F
#define MBOX_BIOS_CMD_QUERY_SPL_FUSE		0x47
#define MBOX_BIOS_CMD_ARMOR_ENTER_SMM_MODE2	0x50
#define MBOX_BIOS_CMD_ARMOR_SPI_TRANSACTION	0x51
#define MBOX_BIOS_CMD_SET_CONFIG		0x5d
#define MBOX_BIOS_CMD_I2C_TPM_ARBITRATION	0x64
#define MBOX_BIOS_CMD_ABORT			0xfe

/* x86 to PSP commands, v1-only */
#define MBOX_BIOS_CMD_DRAM_INFO			0x01
#define MBOX_BIOS_CMD_SMU_FW			0x19
#define MBOX_BIOS_CMD_SMU_FW2			0x1a

#define SMN_PSP_PUBLIC_BASE			0x3800000

/* PSP ROM Armor 1 whitelist defines */
#define PSP_MAX_SPI_CMD_SUPPORT		4	/* Max number of SPI command supported */
#define PSP_MAX_SPI_DATA_BUFFER_SIZE	72	/* Max SPI Command Data Buffer Size */
#define PSP_MAX_WHITE_LIST_CMD_NUM	32	/* Max White list allowed command */
#define PSP_MAX_WHITE_LIST_REGION_NUM	16	/* Max White list allowed region */

/* command/response format, BIOS builds this in memory
 *   mbox_buffer_header: generic header
 *   mbox_buffer:        command-specific buffer format
 *
 * AMD reference code aligns and pads all buffers to 32 bytes.
 */
struct mbox_buffer_header {
	uint32_t size;		/* total size of buffer */
	uint32_t status;	/* command status, filled by PSP if applicable */
} __packed;

/*
 * x86 to PSP mailbox commands that don't take any parameter or return any data, use the
 * mbox_default_buffer, while x86 to PSP commands that either pass data to the PSP or get data
 * returned from the PSP use command-specific buffer definitions. For details on the specific
 * buffer definitions for the various commands, see NDA document #54267 for the generations
 * before family 17h and NDA document #55758 for the generations from family 17h on.
 */

struct mbox_default_buffer {	/* command-response buffer unused by command */
	struct mbox_buffer_header header;
} __packed __aligned(32);

struct smm_req_buffer {
	uint64_t smm_base;		/* TSEG base */
	uint64_t smm_mask;		/* TSEG mask */
	uint64_t psp_smm_data_region;	/* PSP region in SMM space */
	uint64_t psp_smm_data_length;	/* PSP region length in SMM space */
	struct smm_trigger_info smm_trig_info;
#if CONFIG(SOC_AMD_COMMON_BLOCK_PSP_GEN2)
	struct smm_register_info smm_reg_info;
#endif
	uint64_t psp_mbox_smm_buffer_address;
	uint64_t psp_mbox_smm_flag_address;
} __packed;

/* MBOX_BIOS_CMD_SMM_INFO */
struct mbox_cmd_smm_info_buffer {
	struct mbox_buffer_header header;
	struct smm_req_buffer req;
} __packed __aligned(32);

/* MBOX_BIOS_CMD_SX_INFO */
struct mbox_cmd_sx_info_buffer {
	struct mbox_buffer_header header;
	uint8_t sleep_type;
} __packed __aligned(32);

/* MBOX_BIOS_CMD_PSP_FTPM_QUERY, MBOX_BIOS_CMD_PSP_CAPS_QUERY */
struct mbox_cmd_capability_query_buffer {
	struct mbox_buffer_header header;
	uint32_t capabilities;
} __packed __aligned(32);

/* MBOX_BIOS_CMD_HSTI_QUERY */
struct mbox_cmd_hsti_query_buffer {
	struct mbox_buffer_header header;
	uint32_t state;
} __packed __aligned(32);

/* MBOX_BIOS_CMD_SET_RPMC_ADDRESS */
struct mbox_cmd_set_rpmc_address_buffer {
	struct mbox_buffer_header header;
	uint32_t address;
} __packed __aligned(32);

/* MBOX_BIOS_CMD_SET_SPL_FUSE */
struct mbox_cmd_late_spl_buffer {
	struct mbox_buffer_header header;
	uint32_t	spl_value;
} __packed __aligned(32);

struct dtpm_config {
	uint32_t gpio;
} __packed;

enum dtpm_request_type {
	DTPM_REQUEST_ACQUIRE,	/* Acquire I2C bus */
	DTPM_REQUEST_RELEASE,	/* Release I2C bus */
	DTPM_REQUEST_CONFIG,	/* Provide DTPM info */
	DTPM_REQUEST_MAX,
};

/* MBOX_BIOS_CMD_I2C_TPM_ARBITRATION */
struct mbox_cmd_dtpm_config_buffer {
	struct mbox_buffer_header header;
	uint32_t request_type;
	struct dtpm_config config;
} __packed __aligned(32);


struct generic_config {
	uint32_t config_id;
	uint32_t arg0;
	uint32_t arg1;
	uint32_t arg2;
	uint32_t arg3;
} __packed;

/* MBOX_BIOS_CMD_SET_CONFIG */
struct mbox_cmd_set_config_buffer {
	struct mbox_buffer_header header;
	struct generic_config config;
} __packed __aligned(32);

struct ivrs_acpi_table_info {
	uint64_t ivrs_table_buffer;
	uint32_t ivrs_table_size;
} __packed;

/* MBOX_BIOS_CMD_SET_IVRS_TABLE_INFO */
struct mbox_cmd_ivrs_acpi_table_info {
	struct mbox_buffer_header header;
	struct ivrs_acpi_table_info info;
} __packed __aligned(32);

/* MBOX_BIOS_CMD_ARMOR_ENTER_SMM_MODE2 */
struct mbox_rom_armor_enforce_buffer {
	struct mbox_buffer_header header;
	uint32_t flash_size;		/* Returned by PSP: SPI flash size in bytes */
	uint32_t capsule_update;	/* 1 for capsule update/recovery mode, 0 otherwise */
} __packed __aligned(32);

enum mbox_rom_armor_transaction {
	READ_ACCESS	= 1,
	WRITE_ACCESS	= 2,
	ERASE		= 3,
};

struct mbox_rom_armor_flash_command {
	enum mbox_rom_armor_transaction transaction;
	uint64_t buffer_ptr;	/* Pointer to data buffer. Must not be NULL. */
	uint32_t offset;	/* SPI flash offset */
	uint32_t size;		/* Transfer size for all operations */
	uint32_t read_back;	/* Whether to read back data after write for validation */
} __packed;

struct mbox_rom_armor_flash_command_buffer {
	struct mbox_buffer_header header;
	struct mbox_rom_armor_flash_command cmd;
} __packed __aligned(32);


/* MBOX_BIOS_CMD_ARMOR_ENTER_SMM_MODE
 */
struct mbox_rom_armor1_buffer {
	struct mbox_buffer_header header;
	uint64_t tseg_addr;		/* TSEG command buffer address */
	uint32_t chip_select;		/* SPI chip select */
} __packed __aligned(32);

enum mbox_rom_armor1_chip_select {
	CHIP_SELECT_ALL	= 0,
	CHIP_SELECT_1,
	CHIP_SELECT_2
};

enum mbox_rom_armor1_addr_check {
	NO_ADDR_CHECK = 0,
	ADDR_CHECK_24BIT,
	ADDR_CHECK_32BIT
};

enum mbox_rom_armor1_cmd_freq {
	SPI_CMD_FREQ_66_66MHZ = 0,
	SPI_CMD_FREQ_33_33MHZ,
	SPI_CMD_FREQ_22_22MHZ,
	SPI_CMD_FREQ_16_66MHZ,
	SPI_CMD_FREQ_100MHZ,
	SPI_CMD_FREQ_800KHZ
};

enum mbox_rom_armor1_cmd_status {
	SPI_CMD_NOT_PROCEDDED = 0,
	SPI_CMD_COMPLETED,
	SPI_CMD_EXECUTION_ERROR,
	SPI_CMD_NOT_ALLOWED,
	SPI_CMD_MALFORMED
};

struct rom_armor_spi_cmd {
	uint8_t cs;		/* See mbox_rom_armor1_chip_select, cannot be 0 */
	uint8_t freq;		/* See mbox_rom_armor1_cmd_freq */
	uint8_t tx_bytes;	/* From 0 to PSP_MAX_SPI_DATA_BUFFER_SIZE (72) bytes*/
	uint8_t rx_bytes;	/* rx_bytes + tx_bytes <= 72 (PSP_MAX_SPI_DATA_BUFFER_SIZE) */
	uint8_t opcode;
	uint8_t reserved[3];
	uint8_t buffer[PSP_MAX_SPI_DATA_BUFFER_SIZE];
} __packed;

struct rom_armor1_comm_buffer {
	uint8_t	ready_to_run;
	uint8_t cmd_count;
	uint16_t cmd_result;
	struct rom_armor_spi_cmd spi_cmd[PSP_MAX_SPI_CMD_SUPPORT];
} __packed;

struct rom_armor1_allowed_cmd {
	uint8_t cs;		/* See mbox_rom_armor1_chip_select */
	uint8_t freq;		/* See mbox_rom_armor1_cmd_freq */
	uint8_t opcode;		/* The allowed commands opcode */
	uint8_t min_tx, max_tx;	/* Allowed TX byte counts for this command (opcode excluded) */
	uint8_t min_rx, max_rx;	/* The range of allowed Rx byte counts */
	uint8_t addr_check;	/* See mbox_rom_armor1_addr_check */
	uint32_t impact_size;	/* Aligned power of two sized block the command modifies */
} __packed;

struct rom_armor1_region {
	uint32_t start;	/* LSB must be 0x00, bit31 identifies a chipselect: 0=CS1, 1=CS2 */
	uint32_t end;		/* LSB must be 0xFF, start must be less than end */
} __packed;

struct psp_rom_armor1_whitelist {
	uint8_t allowed_cmd_count;
	uint8_t allowed_region_count;
	struct rom_armor1_allowed_cmd allowed_cmds[PSP_MAX_WHITE_LIST_CMD_NUM];
	struct rom_armor1_region allowed_regions[PSP_MAX_WHITE_LIST_REGION_NUM];
} __packed;


#define PSP_INIT_TIMEOUT 10000 /* 10 seconds */
#define PSP_CMD_TIMEOUT 1000 /* 1 second */

#define C2P_BUFFER_MAXSIZE 0xc00 /* Core-to-PSP buffer */
#define P2C_BUFFER_MAXSIZE 0x1000 /* PSP-to-core buffer */

/* PSP to x86 status */
enum mbox_p2c_status {
	MBOX_PSP_SUCCESS		= 0x00,
	MBOX_PSP_INVALID_PARAMETER	= 0x01,
	MBOX_PSP_CRC_ERROR		= 0x02,
	/*
	 * Send to PSP when the requested SPI command in the psp_smi_handler()
	 * handler failed due to an unknown error. The PSP usually doesn't like
	 * seeing this return code and will stop operating.
	 */
	 MBOX_PSP_COMMAND_PROCESS_ERROR	= 0x04,
	 MBOX_PSP_UNSUPPORTED		= 0x08,
	 MBOX_PSP_SPI_BUSY_ASYNC	= 0x0a,
	 /*
	  * Send to PSP when the requested SPI command in the psp_smi_handler()
	  * handler cannot be executed right away. This can happen when the SPI
	  * flash is busy or the SPI controller is busy or being used by ring 0.
	  *
	  * The PSP will raise an SMI later again.
	  */
	 MBOX_PSP_SPI_BUSY		= 0x0b,
};

uintptr_t get_psp_mmio_base(void);

void psp_print_cmd_status(int cmd_status, struct mbox_buffer_header *header);

/* This command needs to be implemented by the generation specific code. */
int send_psp_command(uint32_t command, void *buffer);

enum cb_err psp_get_ftpm_capabilties(uint32_t *capabilities);
enum cb_err psp_get_psp_capabilities(uint32_t *capabilities);
enum cb_err psp_get_hsti_state(uint32_t *state);
enum cb_err soc_read_c2p38(uint32_t *msg_38_value);
enum cb_err psp_send_generic_command(uint32_t command, const char *msg);
enum cb_err psp_command_set_config(uint32_t config, const char *msg);

void enable_psp_smi(void);

void psp_set_smm_flag(void);
void psp_clear_smm_flag(void);

struct mbox_rom_armor_flash_command;
/*
 * psp_rom_armor3_spi_transaction - Send PSP ROM Armor SPI transaction command to PSP firmware
 *
 * @param cmd_buf: Command buffer with SPI transaction to execute.
 *
 * On ROM Armor2:
 * READ/WRITE/ERASE
 *
 * On ROM Armor3:
 * WRITE/ERASE
 * (READ is not supported on ROM Armor3 as flash contents can be read directly from MMIO)
 *
 * Communicates with PSP via mailbox to perform the requested SPI flash operation through ROM Armor.
 * The PSP firmware will enforce the access based on the command parameters and the
 * protection configuration in PSP firmware ('Writable' bit set on PSP directory entries) and
 * BIOS directory types 0x6d (whitelisted flash regions) and return the result via the same command buffer.
 *
 * Returns 0 on success, negative error code on failure.
 */
int psp_rom_armor3_spi_transaction(const struct mbox_rom_armor_flash_command *cmd_buf);

#if !CONFIG(SOC_AMD_COMMON_BLOCK_PSP_ROM_ARMOR1)
static inline int psp_rom_armor1_spi_transaction(struct rom_armor_spi_cmd *cmd_buf)
{
	return -7; /* Unsupported */
}
#else
int psp_rom_armor1_spi_transaction(struct rom_armor_spi_cmd *cmd_buf);
#endif

/**
 * psp_rom_armor_enter_smm_mode - Active PSP Rom Armor
 * @param param: Structrue with parameters to init ROM Armor 1 and 3
 * @param flash_size: Pointer to store the flash size retrieved from PSP firmware
 *
 * After this function is called, PSP Rom Armor will be active and protect
 * the SPI flash according to the configuration in PSP firmware. This function
 * also retrieves the flash size from PSP firmware and returns it via the
 * flash_size output parameter.
 *
 * Returns: 0 on success, negative error code on failure
 */
int psp_rom_armor_enter_smm_mode(void *param, size_t *flash_size);

/**
 * psp_rom_armor_enforce_whitelist - Enforce PSP Rom Armor whitelist
 * @param param: Pointer to the SPI whitelist
 * @param spi_freq: SPI controller normal operation speed to be updated in the
 * whitelist.
 *
 * After this function is called, PSP Rom Armor will be enforce strict
 * checking of the allowed SPI flash commands and SPI flash regions passed in
 * the whitelist. Only used in ROM Armor 1.
 *
 * Returns: 0 on success, negative error code on failure
 */
int psp_rom_armor_enforce_whitelist(void *param, uint8_t spi_freq);

const struct psp_rom_armor1_whitelist *soc_get_psp_rom_armor_whitelist(void);

#endif /* __AMD_PSP_DEF_H__ */
