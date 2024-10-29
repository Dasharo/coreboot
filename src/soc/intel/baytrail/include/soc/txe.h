/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef BAYTRAIL_TXE_H
#define BAYTRAIL_TXE_H

#include <types.h>

/* TXE PCI configuration registers */
#define TXE_FWSTS0			0x40
#define   TXE_CWS_MASK			(0xf << 0)
#define   TXE_CWS_SHIFT			0
#define   TXE_MANUF_MODE		(1 << 4)
#define   TXE_FPT_BAD			(1 << 5)
#define   TXE_CURR_OPSTATE_MASK		(0x7 << 6)
#define   TXE_CURR_OPSTATE_SHIFT	6
#define   TXE_FW_INIT_COMPLETE		(1 << 9)
#define   TXE_FW_BUP_LD_FLR		(1 << 10)
#define   TXE_FW_UPD_IN_PROGRESS	(1 << 11)
#define   TXE_ERR_CODE_MASK		(0xf << 12)
#define   TXE_ERR_CODE_SHIFT		12
#define   TXE_CURR_OPMODE_MASK		(0xf << 16)
#define   TXE_CURR_OPMODE_SHIFT		16
#define   SEC_RESET_COUNT_MASK		(0xf << 20)
#define   DID_ACK_DATA_MASK		(0x7 << 25)
#define   DID_ACK_DATA_SHIFT		25
#define     NO_DID_ACK_RECEIVED		(0 << 25)
#define     NON_PWR_CYCLE_RESET		(1 << 25)
#define     PWR_CYCLE_RESET		(2 << 25)
#define     ACK_DATA_GO_TO_S3		(3 << 25)
#define     ACK_DATA_GO_TO_S4		(4 << 25)
#define     ACK_DATA_GO_TO_S5		(5 << 25)
#define     PERFORM_GLOBAL_RESET	(6 << 25)
#define     ACK_DATA_CONTINUE_BOOT	(7 << 25)
#define   DID_MSG_ACK_MASK		(0xf << 28)
#define     DID_MSG_GOT_ACK		(1 << 28)
#define     DID_MSG_NO_ACK		(0 << 28)

#define TXE_MEM_REQ			0x44
#define  MEM_REQ_MEM_SIZE_MASK		0xffff
#define  MEM_REQ_INVALID		(1 << 30)
#define  MEM_REQ_VALID			(1 << 31)

#define TXE_FWSTS1			0x48 /* aka GS_SHDW */

#define TXE_SEC_TO_HOST0		0x4c
#define TXE_SEC_TO_HOST1		0x50 /* Secure Boot Status */
#define TXE_SEC_TO_HOST2		0x54
#define TXE_SEC_TO_HOST3		0x58
#define TXE_SEC_TO_HOST4		0x5c

#define TXE_SECURE_BOOT_STS		TXE_SEC_TO_HOST1

#define TXE_DID_MSG			0x60
#define   DID_MSG_STATUS_MASK		(0xf << 24)
#define     DID_STATUS_SUCCESS		(0 << 24)
#define     DID_STATUS_NO_MEM		(1 << 24)
#define     DID_STATUS_MEM_INIT_ERR	(2 << 24)
#define   DID_MSG_DONE_MASK		(0xf << 28)
#define     DID_MSG_DID_NOT_DONE	(0 << 28)
#define     DID_MSG_DID_DONE		(1 << 28)

#define TXE_HOST_TO_SEC0		0x64
#define TXE_HOST_TO_SEC1		0x68
#define TXE_HOST_TO_SEC2		0x6c
#define TXE_HOST_TO_SEC3		0x70
#define TXE_HOST_TO_SEC4		0x74
#define TXE_HOST_TO_SEC5		0x78
#define TXE_HOST_TO_SEC6		0x7c

#define TXE_POWER_CAPID			0x80
#define TXE_PME_CTRLSTS			0x84

#define TXE_CLK_GATE_DIS		0x88

#define TXE_TPM_ICR			0x8C
#define   TPM_REQ			(1 << 0)

#define TXE_MSI_CAP			0xa0

#define TXE_IADBGCTRL			0xB0
#define   CPU_PUBLIC_DBG_EN		(1 << 0)
#define   EXPOSE_SOCID			(1 << 1)
#define   IADBGCTRL_LOCK		(1 << 30)
#define   IADBGCTRL_DBG_STS		(1 << 31)

#define TXE_CPU_BIOS_ENV_PROTECT	0xb4
#define  CPU_PROTECT_BIOS_ENV		(1 << 0)
#define  CPU_DIS_BSP_INIT		(1 << 1)
#define  BIOS_RDY_FOR_EVENT		(1 << 2)
#define  CPU_BIOS_ENV_ATTACKED		(1 << 3)

#define TXE_SATT1_BIOS_CTRL		0xc0
#define TXE_SATT1_BIOS_BASE		0xc4
#define TXE_SATT1_BIOS_SIZE		0xc8
#define TXE_SATT4_TPM_CTRL		0xd0
#define  SATT_VALID			(1 << 0)
#define  SATT_TARGET_MASK		(7 << 1)
#define    SATT_TARGET_MEM		(0 << 1)
#define    SATT_TARGET_PCI		(1 << 1)
#define    SATT_TARGET_IOSF		(2 << 1)
#define    SATT_TARGET_SPI		(3 << 1)
#define    SATT_TARGET_DDR		(4 << 1)
#define  SATT_BRG_BA_MSB_SHIFT		8
#define  SATT_BRG_BA_MSB_MASK		(0xf << 8)
#define  SATT_BRG_HOST_EN		(1 << 12)
#define  SATT4_SEC_TPM_DIS		(1 << 13)
#define TXE_SATT4_TPM_BASE		0xd4
#define TXE_SATT4_TPM_SIZE		0xd8

/* TXE MMIO configuration registers behind BAR0 */
#define TXE_MMIO_SEC_IPC_INPUT_DRBELL	0x80400
#define TXE_MMIO_SEC_IPC_INPUT_STS	0x80408
#define TXE_MMIO_SEC_IPC_OUTPUT_STS	0x8040C
#define TXE_MMIO_SEC_IPC_HOST_INT_STS	0x80410
#define TXE_MMIO_IPC_INPUT_PAYLOAD	0x80500

/* TXE MMIO configuration registers behind BAR1 */
#define TXE_MMIO_HOST_CB_WW		0x00
#define TXE_MMIO_HOST_CSR		0x04
#define TXE_MMIO_SEC_CB_RW		0x08
#define TXE_MMIO_SEC_CSR		0x0c
#define   CSR_IE			(1 << 0)
#define   CSR_IS			(1 << 1)
#define   CSR_IG			(1 << 2)
#define   CSR_READY			(1 << 3)
#define   CSR_RESET			(1 << 4)
#define   CSR_RP_START			8
#define   CSR_RP			(((1 << 8) - 1) << CSR_RP_START)
#define   CSR_WP_START			16
#define   CSR_WP			(((1 << 8) - 1) << CSR_WP_START)
#define   CSR_CBD_START			24
#define   CSR_CBD			(((1 << 8) - 1) << CSR_CBD_START)

#define TXE_MMIO_SATT_TPM_SAP_BA	0x1064

#define TXE_MMIO_SEC_IPC_READINESS	0x2040
#define TXE_MMIO_SEC_IPC_OUTPUT_DRBELL	0x2048
#define TXE_MMIO_IPC_OUTPUT_PAYLOAD	0x20c0
#define TXE_MMIO_HOST_IPC_READINESS	0x2150
#define   IPC_READINESS_HOST_RDY	(1 << 0)
#define   IPC_READINESS_SEC_RDY		(1 << 1)
#define   IPC_READINESS_RDY_CLR		(1 << 2)
#define TXE_MMIO_S_SEC_IPC_OUTPUT_STS	0x2154
#define   IPC_OUTPUT_READY		(1 << 0)

#define TXE_MMIO_ALIVENESS_RESP		0x2044
#define   TXE_ALIVENESS_ACK		(1 << 0)
#define TXE_MMIO_ALIVENESS_REQ		0x214c
#define   TXE_ALIVENESS_REQ		(1 << 0)

#define TXE_MMIO_HHISR			0x2020
#define   HHISR_INT_BAR0_STS		(1 << 0)
#define   HHISR_INT_BAR1_STS		(1 << 1)
#define TXE_MMIO_HISR			0x2060

#define TXE_MMIO_TPM_DSABLE		0x2268
#define   TPM_DISABLE			(1 << 0)
#define   TPM_DISABLE_LOCK		(1 << 1)

#define TXEI_BASE_ADDRESS		0xce000000
#define TXEI2_BASE_ADDRESS		0xce100000

/* TXE Current Working States */
#define TXE_FWSTS0_CWS_RESET		0x0
#define TXE_FWSTS0_CWS_INIT		0x1
#define TXE_FWSTS0_CWS_RECOVERY		0x2
#define TXE_FWSTS0_CWS_NORMAL		0x5

/* TXE Current Operation Modes */
#define TXE_FWSTS0_COM_NORMAL			0x0
#define TXE_FWSTS0_COM_DEBUG			0x2
#define TXE_FWSTS0_COM_SOFT_TEMP_DISABLE	0x3
#define TXE_FWSTS0_COM_SECOVER_JMPR		0x4
#define TXE_FWSTS0_COM_SECOVER_TXEI_MSG		0x5

/* TXE Current Operational State */
#define  TXE_FWSTS0_STATE_PREBOOT		0x0
#define  TXE_FWSTS0_STATE_M0_UMA		0x1
#define  TXE_FWSTS0_STATE_MOFF			0x4
#define  TXE_FWSTS0_STATE_M0			0x5
#define  TXE_FWSTS0_STATE_BRINGUP		0x6
#define  TXE_FWSTS0_STATE_ERROR			0x7

/* TXE MKHI commands */
#define MKHI_CBM_GROUP_ID		0x00
#define   MKHI_CBM_GLOBAL_RESET		0x0b

/* Origin of Global Reset command */
#define GR_ORIGIN_BIOS_POST		0x2

#define MKHI_FWCAPS_GROUP_ID		0x03
#define   MKHI_GET_RULE			0x02
#define   MKHI_SET_RULE			0x03

#define MKHI_FWCAPS_RULE_ID		0x0
#define MKHI_SEC_DISABLE_RULE_ID	0x6
#define MKHI_LCL_FWUPD_RULE_ID		0x7
#define MKHI_FW_FEATURE_STATE_RULE_ID	0x20
#define MKHI_OEM_TAG_RULE_ID		0x2b

#define MKHI_HMRFPO_GROUP_ID		0x05
#define   HMRFPO_ENABLE			0x01
#define   HMRFPO_LOCK			0x02
#define   HMRFPO_GET_STATUS		0x03
#define   HMRFPO_DISABLE		0x04

#define MKHI_GEN_GROUP_ID		0xff
#define   MKHI_GET_FW_VERSION		0x02
#define   MKHI_SEC_UNCONFIGURE		0x0d
#define   MKHI_GET_SEC_UNCONFIG_STATE	0x0e
#define   MKHI_END_OF_POST		0x0c

/* Fixed Address TXEI Header's Host Address field value */
#define BIOS_HOST_ADDR	0x00
/* Fixed Address TXEI Header's TXE Address field value */
#define HECI_MKHI_ADDR	0x07
/* Fixed Address TXEI Header's TXE Address for MEI bus messages */
#define HECI_TXEI_ADDR	0x00

/* Timeouts */
#define HECI_WAIT_DELAY			1000		/* 1ms timeout for IO delay */
#define HECI_INIT_TIMEOUT		15000000	/* 15sec timeout in microseconds */
#define HECI_READY_TIMEOUT		8000000		/* 8sec timeout in microseconds */
#define HECI_READ_TIMEOUT		5000000		/* 5sec timeout in microseconds */
#define HECI_SEND_TIMEOUT		5000000		/* 5sec timeout in microseconds */
#define HECI_MAX_RETRY			3		/* Value based off HECI HPS */
#define HECI_MSG_DELAY			2000000		/* show warning msg and stay for 2 seconds. */

/* TXE RX and TX error status */
enum txe_tx_rx_status {
	/*
	 * Transmission of HECI message is success or
	 * Reception of HECI message is success.
	 */
	TXE_TX_RX_SUCCESS = 0,

	 /* Timeout to send a message to TXE */
	TXE_TX_ERR_TIMEOUT = 1,

	/* Timeout to receive the response message from TXE */
	TXE_RX_ERR_TIMEOUT = 2,

	/*
	 * Response length doesn't match with expected
	 * response message length
	 */
	TXE_RX_ERR_RESP_LEN_MISMATCH = 3,

	/* TXE is not ready during TX flow */
	TXE_TX_ERR_TXE_NOT_READY = 4,

	/* TXE is not ready during RX flow */
	TXE_RX_ERR_TXE_NOT_READY = 5,

	/* Invalid input arguments provided for TX API */
	TXE_TX_ERR_INPUT = 6,

	/* Invalid input arguments provided for RX API */
	TXE_RX_ERR_INPUT = 7,
};

union txe_fwsts0 {
	u32 data;
	struct {
		u32 working_state: 4;
		u32 mfg_mode: 1;
		u32 fpt_bad: 1;
		u32 operation_state: 3;
		u32 fw_init_complete: 1;
		u32 ft_bup_ld_flr: 1;
		u32 update_in_progress: 1;
		u32 error_code: 4;
		u32 operation_mode: 4;
		u32 reset_count: 4;
		u32 reserved1: 1;
		u32 ack_data: 3;
		u32 bios_msg_ack: 4;
	} __packed fields;
};

union txe_fwsts1 {
	u32 data;
	struct {
	u32 reserved1: 16;
	u32 current_state: 8;
	u32 current_pmevent: 4;
	u32 progress_code: 4;
	} __packed fields;
};

union txe_sbsts {
	u32 data;
	struct {
	u32 sb_executed: 1;
	u32 recovery: 1;
	u32 debug_was_enabled: 1;
	u32 debug_is_enabled: 1;
	u32 emulation_mode: 1;
	u32 sbm_svn: 6;
	u32 km_id: 4;
	u32 km_svn: 4;
	u32 alt_bios_limit: 13;
	} __packed fields;
};

/* MKHI Message Header */
struct mkhi_hdr {
	uint8_t group_id;
	uint8_t command:7;
	uint8_t is_resp:1;
	uint8_t rsvd;
	uint8_t result;
} __packed;

struct get_fw_version_resp {
	struct mkhi_hdr hdr;
	uint16_t code_minor;
	uint16_t code_major;
	uint16_t code_build_no;
	uint16_t code_hotfix;
	uint16_t recovery_minor;
	uint16_t recovery_major;
	uint16_t recovery_build_no;
	uint16_t recovery_hotfix;
	uint16_t fitc_minor;
	uint16_t fitc_major;
	uint16_t fitc_build_no;
	uint16_t fitc_hotfix;
} __packed;

bool is_txe_devfn_visible(void);
void txe_early_init(void);
void txe_txei_init(void);
int txe_get_uma_size(void);
void txe_send_did(uintptr_t uma_base, int uma_size, int s3resume);
void txe_handle_bios_action(uint32_t bios_action);
bool txe_is_excluded_from_bios_flows(void);
bool txe_in_recovery(void);
bool txe_has_error(void);
bool txe_fw_update_in_progress(void);
void txe_hide_device(void);
void txe_fw_shadow_done(void);
int txe_global_reset(void);
int txe_aliveness_request(uintptr_t tempbar, bool request);
void txe_print_secure_boot_status(void);
void dump_txe_status(void);
void txe_do_send_end_of_post(void);
int txe_get_fw_version(struct get_fw_version_resp *reply);

#endif
