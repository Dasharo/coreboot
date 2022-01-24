/* SPDX-License-Identifier: GPL-2.0-only */

/* IBV defined Structure */ /* IBV Specific Options */
#ifndef MWLC_D_H
#define MWLC_D_H

#include <stdbool.h>
/*
 *   SBDFO - Segment Bus Device Function Offset
 *   31:28   Segment (4-bits)
 *   27:20   Bus     (8-bits)
 *   19:15   Device  (5-bits)
 *   14:12   Function(3-bits)
 *   11:00   Offset  (12-bits)
 */
typedef u32 SBDFO;

#define BYTESIZE 1
#define WORDSIZE 2
#define DWORDSIZE 4

#define MAKE_SBDFO(seg,bus,dev,fun,off) ((((uint32)(seg)) << 28) | (((uint32)(bus)) << 20) | \
		    (((uint32)(dev)) << 15) | (((uint32)(fun)) << 12) | ((uint32)(off)))
#define SBDFO_SEG(x) (((uint32)(x) >> 28) & 0x0F)
#define SBDFO_BUS(x) (((uint32)(x) >> 20) & 0xFF)
#define SBDFO_DEV(x) (((uint32)(x) >> 15) & 0x1F)
#define SBDFO_FUN(x) (((uint32)(x) >> 12) & 0x07)
#define SBDFO_OFF(x) (((uint32)(x)) & 0xFFF)
#define ILLEGAL_SBDFO 0xFFFFFFFF

#define MAX_TOTAL_DIMMS 8	/* Maximum Number of DIMMs in systems */
				/* (DCT0 + DCT1) */
#define MAX_DIMMS 4		/* Maximum Number of DIMMs on each DCT */
#define MAX_LDIMMS 4		/* Maximum number of Logical DIMMs per DCT */

/*MCT Max variables */
#define MAX_ERRORS 32		/* Maximum number of Errors Reported */
#define MAX_STATUS 32		/* Maximum number of Status variables*/
#define MAX_BYTE_LANES (8 + 1)	/* Maximum number of Byte Lanes - include ECC */

#define C_MAX_DIMMS 4		/* Maximum Number of DIMMs on each DCT */

/* STATUS Definition */
#define DCT_STATUS_REGISTERED 3		/* Registered DIMMs support */
#define DCT_STATUS_LOAD_REDUCED 4	/* Load-Reduced DIMMs support */
#define DCT_STATUS_OnDimmMirror 24	/* OnDimmMirror support */

/* PCI Definitions */
#define FUN_HT 0	  /* Function 0 Access */
#define FUN_MAP 1	  /* Function 1 Access */
#define FUN_DCT 2	  /* Function 2 Access */
#define FUN_MISC 3	  /* Function 3 Access */
#define FUN_ADD_DCT 0xF	  /* Function 2 Additional Register Access */
#define BOTH_DCTS 2	  /* The access is independent of DCTs */
#define PCI_MIN_LOW 0	  /* Lowest possible PCI register location */
#define PCI_MAX_HIGH 31	  /* Highest possible PCI register location */

/*Function 2 */
/* #define DRAM_INIT 0x7C */
#define DRAM_MRS_REGISTER 0x84
#define DRAM_CONFIG_HIGH 0x94
#define DRAM_CONTROLLER_ADD_DATA_OFFSET_REG 0x98
#define DRAM_CONTROLLER_ADD_DATA_PORT_REG 0x9C

/*Function 2 Additional DRAM control registers */
#define DRAM_ADD_DCT_PHY_CONTROL_REG 0x8
#define DRAM_CONT_ADD_DQS_TIMING_CTRL_BL_01 0x30
#define DRAM_CONT_ADD_DQS_TIMING_CTRL_BL_45 0x40
#define DRAM_CONT_ADD_PHASE_REC_CTRL_LOW 0x50
#define DRAM_CONT_ADD_PHASE_REC_CTRL_HIGH 0x51
#define DRAM_CONT_ADD_ECC_PHASE_REC_CTRL 0x52
#define DRAM_CONT_ADD_WRITE_LEV_ERROR_REG 0x53

/* CPU Register definitions */

/* Register Bit Location */
#define DCT_ACCESS_DONE 31
#define DCT_ACCESS_WRITE 30
#define RDQS_EN 12
#define TR_DIMM_SEL_START 4
#define TR_DIMM_SEL_END 5
#define WR_LV_TR_MODE 1
#define TR_NIBBLE_SEL 2
#define WR_LV_ODT_EN 12
#define WR_LV_ERR_START 0
#define WR_LV_ERR_END 8
#define SEND_MRS_CMD 26
#define QOFF 12
#define MRS_LEVEL 7
#define MRS_ADDRESS_START_FAM_10 0
#define MRS_ADDRESS_END_FAM_10 15
#define MRS_ADDRESS_START_FAM_15 0
#define MRS_ADDRESS_END_FAM_15 17
#define MRS_BANK_START_FAM_10 16
#define MRS_BANK_END_FAM_10 18
#define MRS_BANK_START_FAM_15 18
#define MRS_BANK_END_FAM_15 20
#define MRS_CHIP_SEL_START_FAM_10 20
#define MRS_CHIP_SEL_END_FAM_10 22
#define MRS_CHIP_SEL_START_FAM_15 21
#define MRS_CHIP_SEL_END_FAM_15 23
#define ASR 18
#define SRT 19
#define DRAM_TERM_DYN_START 10
#define DRAM_TERM_DYN_END 11
#define WRT_LV_TR_MODE 1
#define TR_NIBBLE_SEL 2
#define TR_DIMM_SEL_START 4
#define TR_DIMM_SEL_END 5
#define WRT_LV_TR_EN 0
#define DRV_IMP_CTRL_START 2
#define DRV_IMP_CTRL_END 3
#define DRAM_TERM_NB_START 7
#define DRAM_TERM_NB_END 9
#define ON_DIMM_MIRROR 3

typedef struct _sMCTStruct
{
	void (*agesa_delay)(u32 delayval);	/* IBV defined Delay Function */
} __attribute__((packed, aligned(4))) sMCTStruct;

/* DCT 0 and DCT 1 Data structure */
typedef struct _sDCTStruct
{
	u8 node_id;			/* Node ID */
	u8 dct_train;			/* Current DCT being trained */
	u8 curr_dct;			/* Current DCT number (0 or 1) */
	u8 dct_cs_present;		/* Current DCT CS mapping */
	u8 wr_dqs_gross_dly_base_offset;
	int32_t wl_seed_gross_delay[MAX_BYTE_LANES * MAX_LDIMMS];	/* Write Levelization Seed Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	int32_t wl_seed_fine_delay[MAX_BYTE_LANES * MAX_LDIMMS];	/* Write Levelization Seed Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	int32_t wl_seed_pre_gross_delay[MAX_BYTE_LANES * MAX_LDIMMS];	/* Write Levelization Seed Pre-Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 wl_seed_pre_gross_prev_nibble[MAX_BYTE_LANES * MAX_LDIMMS];
	u8 wl_seed_gross_prev_nibble[MAX_BYTE_LANES * MAX_LDIMMS];
	u8 wl_seed_fine_prev_nibble[MAX_BYTE_LANES * MAX_LDIMMS];
								/* per byte Lane Per Logical DIMM*/
	u8 wl_gross_delay[MAX_BYTE_LANES * MAX_LDIMMS];		/* Write Levelization Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 wl_fine_delay[MAX_BYTE_LANES * MAX_LDIMMS];		/* Write Levelization Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 wl_gross_delay_first_pass[MAX_BYTE_LANES * MAX_LDIMMS];	/* First-Pass Write Levelization Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 wl_fine_delay_first_pass[MAX_BYTE_LANES * MAX_LDIMMS];	/* First-Pass Write Levelization Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 wl_gross_delay_prev_pass[MAX_BYTE_LANES * MAX_LDIMMS];	/* Previous Pass Write Levelization Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 wl_fine_delay_prev_pass[MAX_BYTE_LANES * MAX_LDIMMS];	/* Previous Pass Write Levelization Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 wl_gross_delay_final_pass[MAX_BYTE_LANES * MAX_LDIMMS];	/* Final-Pass Write Levelization Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 wl_fine_delay_final_pass[MAX_BYTE_LANES * MAX_LDIMMS];	/* Final-Pass Write Levelization Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	int32_t wl_critical_gross_delay_first_pass;
	int32_t wl_critical_gross_delay_prev_pass;
	int32_t wl__critical_gross_delay_final_pass;
	u16 wl_prev_memclk_freq[MAX_TOTAL_DIMMS];
	u16 reg_man_1_present;
	u8 dimm_present[MAX_TOTAL_DIMMS];/* Indicates which DIMMs are present */
					/* from Total Number of DIMMs(per Node)*/
	u8 dimm_x8_present[MAX_TOTAL_DIMMS];	/* Which DIMMs x8 devices */
	u8 status[MAX_STATUS];		/* status for DCT0 and 1 */
	u8 err_code[MAX_ERRORS];		/* Major Error codes for DCT0 and 1 */
	u8 err_status[MAX_ERRORS];	/* Minor Error codes for DCT0 and 1 */
	u8 dimm_valid[MAX_TOTAL_DIMMS];	/* Indicates which DIMMs are valid for */
					/* Total Number of DIMMs(per Node) */
	u8 wl_total_delay[MAX_BYTE_LANES];/* Write Levelization Total Delay */
					/* per byte lane */
	u8 max_dimms_installed;		/* Max Dimms Installed for current DCT */
	u8 dimm_ranks[MAX_TOTAL_DIMMS];	/* Total Number of Ranks(per Dimm) */
	u64 logical_cpuid;
	u8 wl_pass;
} __attribute__((packed, aligned(4))) sDCTStruct;

void set_dct_addr_bits(sDCTStruct *p_dct_data,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high, u32 value);
void amd_mem_pci_write_bits(SBDFO loc, u8 highbit, u8 lowbit, u32 *p_value);
u32 get_bits(sDCTStruct *p_dct_data,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high);
void amd_mem_pci_read_bits(SBDFO loc, u8 highbit, u8 lowbit, u32 *p_value);
u32 bit_test_set(u32 cs_mask,u32 temp_d);
u32 bit_test_reset(u32 cs_mask,u32 temp_d);
void set_bits(sDCTStruct *p_dct_data,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high, u32 value);
bool bit_test(u32 value, u8 bitLoc);
u32 rtt_nom_non_target_reg_dimm (sMCTStruct *p_mct_data, sDCTStruct *p_dct_data, u8 dimm, bool wl, u8 mem_clk_freq, u8 rank);
u32 rtt_nom_target_reg_dimm (sMCTStruct *p_mct_data, sDCTStruct *p_dct_data, u8 dimm, bool wl, u8 mem_clk_freq, u8 rank);
u32 rtt_wr_reg_dimm (sMCTStruct *p_mct_data, sDCTStruct *p_dct_data, u8 dimm, bool wl, u8 mem_clk_freq, u8 rank);
u8 wr_lv_odt_reg_dimm (sMCTStruct *p_mct_data, sDCTStruct *p_dct_data, u8 dimm);
u32 get_add_dct_bits(sDCTStruct *p_dct_data,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high);
void amd_mem_pci_read(SBDFO loc, u32 *value);
void amd_mem_cpi_write(SBDFO loc, u32 *value);

#endif
