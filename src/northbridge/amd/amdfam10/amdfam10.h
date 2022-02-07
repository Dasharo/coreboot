/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef AMDFAM10_H
#define AMDFAM10_H

#include <stdint.h>
#include <cpu/amd/common/nums.h>
#include <cpu/amd/family_10h-family_15h/amdfam10_sysconf.h>
#include <device/device.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include <drivers/amd/hypertransport/porting.h>

#include "raminit.h"

/* Definitions for setup_resourcemap() variants. */

#define PCI_ADDR(SEGBUS, DEV, FN, WHERE) ( \
	(((SEGBUS) & 0xFFF) << 20) | \
	(((DEV) & 0x1F) << 15) | \
	(((FN) & 0x07) << 12) | \
	((WHERE) & 0xFFF))

#define ADDRMAP_REG(r) PCI_ADDR(CONFIG_CBB, CONFIG_CDB, 1, r)

#define RES_PCI_IO 0x10
#define RES_PORT_IO_8 0x22
#define RES_PORT_IO_32 0x20
#define RES_MEM_IO 0x40

#define NODE_ID		0x60
#define HT_INIT_CONTROL	0x6c
#define HTIC_COLDR_DETECT	(1 << 4)
#define HTIC_BIOSR_DETECT	(1 << 5)
#define HTIC_INIT_DETECT	(1 << 6)

/* Definitions of various FAM10 registers */
/* Function 0 */
#define HT_TRANSACTION_CONTROL 0x68
#define  HTTC_DIS_RD_B_P		(1 << 0)
#define  HTTC_DIS_RD_DW_P		(1 << 1)
#define  HTTC_DIS_WR_B_P		(1 << 2)
#define  HTTC_DIS_WR_DW_P		(1 << 3)
#define  HTTC_DIS_MTS			(1 << 4)
#define  HTTC_CPU1_EN			(1 << 5)
#define  HTTC_CPU_REQ_PASS_PW		(1 << 6)
#define  HTTC_CPU_RD_RSP_PASS_PW	(1 << 7)
#define  HTTC_DIS_P_MEM_C		(1 << 8)
#define  HTTC_DIS_RMT_MEM_C		(1 << 9)
#define  HTTC_DIS_FILL_P		(1 << 10)
#define  HTTC_RSP_PASS_PW		(1 << 11)
#define  HTTC_BUF_REL_PRI_SHIFT	13
#define  HTTC_BUF_REL_PRI_MASK		3
#define   HTTC_BUF_REL_PRI_64		0
#define   HTTC_BUF_REL_PRI_16		1
#define   HTTC_BUF_REL_PRI_8		2
#define   HTTC_BUF_REL_PRI_2		3
#define  HTTC_LIMIT_CLDT_CFG		(1 << 15)
#define  HTTC_LINT_EN			(1 << 16)
#define  HTTC_APIC_EXT_BRD_CST		(1 << 17)
#define  HTTC_APIC_EXT_ID		(1 << 18)
#define  HTTC_APIC_EXT_SPUR		(1 << 19)
#define  HTTC_SEQ_ID_SRC_NODE_EN	(1 << 20)
#define  HTTC_DS_NP_REQ_LIMIT_SHIFT	21
#define  HTTC_DS_NP_REQ_LIMIT_MASK	3
#define   HTTC_DS_NP_REQ_LIMIT_NONE	0
#define   HTTC_DS_NP_REQ_LIMIT_1	1
#define   HTTC_DS_NP_REQ_LIMIT_4	2
#define   HTTC_DS_NP_REQ_LIMIT_8	3


/* Function 1 */
#define PCI_IO_BASE0	0xc0
#define PCI_IO_BASE1	0xc8
#define PCI_IO_BASE2	0xd0
#define PCI_IO_BASE3	0xd8
#define PCI_IO_BASE_VGA_EN	(1 << 4)
#define PCI_IO_BASE_NO_ISA	(1 << 5)

/* Function 2 */
// 0x1xx is for DCT1
#define DRAM_CSBASE	0x40
#define DRAM_CSMASK	0x60
#define DRAM_BANK_ADDR_MAP 0x80

#define DRAM_CTRL	0x78
#define  DC_RD_PTR_INIT_SHIFT 0
#define  DC_RD_PTR_INIT_MASK  0xf
#define  DC_TWRRD3_2_SHIFT 8  /*DDR3 */
#define  DC_TWRRD3_2_MASK 3
#define  DC_TWRWR3_2_SHIFT 10 /*DDR3 */
#define  DC_TWRWR3_2_MASK 3
#define  DC_TWDRD3_2_SHIFT 12 /*DDR3 */
#define  DC_TWDRD3_2_MASK 3
#define  DC_ALT_VID_C3_MEM_CLK_TRI_EN (1 << 16)
#define  DC_DQS_RCV_EN_TRAIN (1 << 18)
#define  DC_MAX_RD_LATENCY_SHIFT 22
#define  DC_MAX_RD_LATENCY_MASK 0x3ff

#define DRAM_INIT	0x7c
#define  DI_MRS_ADDRESS_SHIFT 0
#define  DI_MRS_ADDRESS_MASK 0xffff
#define  DI_MRS_BANK_SHIFT 16
#define  DI_MRS_BANK_MASK 7
#define  DI_MRS_CHIP_SEL_SHIFT 20
#define  DI_MRS_CHIP_SEL_MASK 7
#define  DI_SEND_RCHG_ALL (1 << 24)
#define  DI_SEMD_AUTO_REFRESH (1 << 25)
#define  DI_SEMD_MRS_CMD	  (1 << 26)
#define  DI_DEASSERT_MEM_RST_X (1 << 27)
#define  DI_ASSERT_CKE	 (1 << 28)
#define  DI_SEND_ZQ_CMD	(1 << 29) /*DDR3 */
#define  DI_EN_MRS_CMD	(1 << 30)
#define  DI_EN_DRAM_INIT	 (1 << 31)

#define DRAM_MRS	0x84
#define  DM_BURST_CTRL_SHIFT 0
#define  DM_BURST_CTRL_MASK 3
#define  DM_DRV_IMP_CTRL_SHIFT 2 /* DDR3 */
#define  DM_DRV_IMP_CTRL_MASK 3
#define  DM_TWR_SHIFT 4 /* DDR3 */
#define  DM_TWR_MASK 7
#define  DM_TWR_BASE 4
#define  DM_TWR_MIN  5
#define  DM_TWR_MAX  12
#define  DM_DRAM_TERM_SHIFT 7 /*DDR3 */
#define  DM_DRAM_TERM_MASK 7
#define  DM_DRAM_TERM_DYN_SHIFT 10 /* DDR3 */
#define  DM_DRAM_TERM_DYN_MASK 3
#define  DM_OOFF (1 << 13)
#define  DM_ASR (1 << 18)
#define  DM_SRT (1 << 19)
#define  DM_TCWL_SHIFT 20
#define  DM_TCWL_MASK 7
#define  DM_PCHG_PD_MODE_SEL (1 << 23) /* DDR3 */
#define  DM_MPR_LOC_SHIFT 24 /* DDR3 */
#define  DM_MPR_LOC_MASK 3
#define  DM_MPR_EN (1 << 26) /* DDR3 */

#define DRAM_TIMING_LOW	   0x88
#define	 DTL_TCL_SHIFT	   0
#define	 DTL_TCL_MASK	   0xf
#define	  DTL_TCL_BASE	   1 /* DDR3 =4 */
#define	  DTL_TCL_MIN	   3 /* DDR3 =4 */
#define	  DTL_TCL_MAX	   6 /* DDR3 =12 */
#define	 DTL_TRCD_SHIFT	   4
#define	 DTL_TRCD_MASK	   3 /* DDR3 =7 */
#define	  DTL_TRCD_BASE	   3 /* DDR3 =5 */
#define	  DTL_TRCD_MIN	   3 /* DDR3 =5 */
#define   DTL_TRCD_MAX	    6 /* DDR3 =12 */
#define	 DTL_TRP_SHIFT	   8 /* DDR3 =7 */
#define	 DTL_TRP_MASK	   3 /* DDR3 =7 */
#define	  DTL_TRP_BASE	   3 /* DDR3 =5 */
#define	  DTL_TRP_MIN	   3 /* DDR3 =5 */
#define   DTL_TRP_MAX	    6 /* DDR3 =12 */
#define	 DTL_TRTP_SHIFT	   11 /*DDR3 =10 */
#define	 DTL_TRTP_MASK	   1  /*DDR3 =3 */
#define	  DTL_TRTP_BASE	   2  /* DDR3 =4 */
#define	  DTL_TRTP_MIN	   2  /* 4 for 64 bytes*/  /* DDR3 =4 for 32bytes or 64bytes */
#define   DTL_TRTP_MAX	    3  /* 5 for 64 bytes */ /* DDR3 =7 for 32Bytes or 64bytes */
#define	 DTL_TRAS_SHIFT	   12
#define	 DTL_TRAS_MASK	   0xf
#define	  DTL_TRAS_BASE	   3 /* DDR3 =15 */
#define	  DTL_TRAS_MIN	   5 /* DDR3 =15 */
#define	  DTL_TRAS_MAX	   18 /*DDR3 =30 */
#define	 DTL_TRC_SHIFT	   16
#define	 DTL_TRC_MASK	   0xf /* DDR3 =0x1f */
#define	  DTL_TRC_BASE	   11
#define	  DTL_TRC_MIN	   11
#define	  DTL_TRC_MAX	   26  /* DDR3 =43 */
#define	 DTL_TWR_SHIFT	   20 /* only for DDR2, DDR3's is on DC */
#define	 DTL_TWR_MASK	   3
#define	  DTL_TWR_BASE	   3
#define	  DTL_TWR_MIN	   3
#define	  DTL_TWR_MAX	   6
#define  DTL_TRRD_SHIFT    22
#define   DTL_TRRD_MASK    3
#define   DTL_TRRD_BASE    2 /* DDR3 =4 */
#define   DTL_TRRD_MIN	   2 /* DDR3 =4 */
#define   DTL_TRRD_MAX	    5 /* DDR3 =7 */
#define  DTL_MEM_CLK_DIS_SHIFT 24    /* Channel A */
#define  DTL_MEM_CLK_DIS_3       (1 << 26)
#define  DTL_MEM_CLK_DIS_2       (1 << 27)
#define  DTL_MEM_CLK_DIS_1       (1 << 28)
#define  DTL_MEM_CLK_DIS_0       (1 << 29)
/* DTL_MemClkDis for m2 and s1g1 is different */

#define DRAM_TIMING_HIGH   0x8c
#define  DTH_TRWTWB_SHIFT 0
#define  DTH_TRWTWB_MASK 3
#define   DTH_TRWTWB_BASE 3  /* DDR3 =4 */
#define   DTH_TRWTWB_MIN  3  /* DDR3 =5 */
#define   DTH_TRWTWB_MAX  10 /* DDR3 =11 */
#define  DTH_TRWTTO_SHIFT  4
#define  DTH_TRWTTO_MASK   7
#define   DTH_TRWTTO_BASE   2 /* DDR3 =3 */
#define   DTH_TRWTTO_MIN    2 /* DDR3 =3 */
#define   DTH_TRWTTO_MAX    9 /* DDR3 =10 */
#define	 DTH_TWTR_SHIFT	   8
#define	 DTH_TWTR_MASK	   3
#define	  DTH_TWTR_BASE	   0  /* DDR3 =4 */
#define	  DTH_TWTR_MIN	   1  /* DDR3 =4 */
#define	  DTH_TWTR_MAX	   3  /* DDR3 =7 */
#define	 DTH_TWRRD_SHIFT   10
#define	 DTH_TWRRD_MASK	   3	/* For DDR3 3_2 is at 0x78 DC */
#define	  DTH_TWRRD_BASE   0  /* DDR3 =0 */
#define	  DTH_TWRRD_MIN	   0  /* DDR3 =2 */
#define	  DTH_TWRRD_MAX	   3  /* DDR3 =12 */
#define  DTH_TWRWR_SHIFT   12
#define  DTH_TWRWR_MASK    3	/* For DDR3 3_2 is at 0x78 DC */
#define   DTH_TWRWR_BASE   1
#define   DTH_TWRWR_MIN    1	/* DDR3 =3 */
#define   DTH_TWRWR_MAX    3	/* DDR3 =12 */
#define  DTH_TRDRD_SHIFT   14
#define  DTH_TRDRD_MASK    3  /* For DDR3 3_2 is at 0x78 DC */
#define   DTH_TRDRD_BASE   2
#define   DTH_TRDRD_MIN    2
#define   DTH_TRDRD_MAX    5	/* DDR3 =10 */
#define	 DTH_TREF_SHIFT	   16
#define	 DTH_TREF_MASK	   3
#define	  DTH_TREF_7_8_US  2
#define	  DTH_TREF_3_9_US  3
#define  DTH_DIS_AUTO_REFRESH (1 << 18)
#define  DTH_TRFC0_SHIFT   20 /* for Logical DIMM0 */
#define  DTH_TRFC_MASK	     7
#define	  DTH_TRFC_75_256M   0
#define	  DTH_TRFC_105_512M  1
#define   DTH_TRFC_127_5_1G  2
#define   DTH_TRFC_195_2G    3
#define   DTH_TRFC_327_5_4G  4
#define  DTH_TRFC1_SHIFT   23 /*for Logical DIMM1 */
#define  DTH_TRFC2_SHIFT   26 /*for Logical DIMM2 */
#define  DTH_TRFC3_SHIFT   29 /*for Logical DIMM3 */

#define DRAM_CONFIG_LOW	   0x90
#define	 DCL_INIT_DRAM	   (1 << 0)
#define	 DCL_EXIT_SELF_REF   (1 << 1)
#define  DCL_PLL_LOCK_TIME_SHIFT 2
#define  DCL_PLL_LOCK_TIME_MASK 3
#define   DCL_PLL_LOCK_TIME_15US 0
#define   DCL_PLL_LOCK_TIME_60US 1
#define  DCL_DRAM_TERM_SHIFT 4
#define  DCL_DRAM_TERM_MASK  3
#define   DCL_DRAM_TERM_NO   0
#define   DCL_DRAM_TERM_75_OH 1
#define   DCL_DRAM_TERM_150_OH 2
#define   DCL_DRAM_TERM_50_OH 3
#define  DCL_DIS_DQS_BAR	    (1 << 6) /* only for DDR2 */
#define  DCL_DRAM_DRV_WEAK   (1 << 7) /* only for DDR2 */
#define  DCL_PAR_EN	   (1 << 8)
#define  DCL_SEL_REF_RATE_EN (1 << 9) /* only for DDR2 */
#define  DCL_BURST_LENGTH_32 (1 << 10) /* only for DDR3 */
#define  DCL_WIDTH_128	   (1 << 11)
#define  DCL_X4_DIMM_SHIFT  12
#define  DCL_X4_DIMM_MASK   0xf
#define  DCL_UN_BUFF_DIMM    (1 << 16)
#define  DCL_EN_PHY_DQS_RCV_EN_TR (1 << 18)
#define	 DCL_DIMM_ECC_EN	   (1 << 19)
#define  DCL_DYN_PAGE_CLOSE_EN (1 << 20)
#define  DCL_IDLE_CYC_INIT_SHIFT 21
#define  DCL_IDLE_CYC_INIT_MASK	3
#define   DCL_IDLE_CYC_INIT_16CLK 0
#define   DCL_IDLE_CYC_INIT_32CLK 1
#define   DCL_IDLE_CYC_INIT_64CLK 2
#define   DCL_IDLE_CYC_INIT_96CLK 3
#define  DCL_FORCE_AUTO_PCHG   (1 << 23)

#define DRAM_CONFIG_HIGH   0x94
#define  DCH_MEM_CLK_FREQ_SHIFT 0
#define  DCH_MEM_CLK_FREQ_MASK  7
#define   DCH_MEM_CLK_FREQ_200MHZ 0  /* DDR2 */
#define   DCH_MEM_CLK_FREQ_266MHZ 1  /* DDR2 */
#define   DCH_MEM_CLK_FREQ_333MHZ 2  /* DDR2 */
#define	  DCH_MEM_CLK_FREQ_400MHZ 3  /* DDR2 and DDR 3*/
#define	  DCH_MEM_CLK_FREQ_533MHZ 4  /* DDR 3 */
#define	  DCH_MEM_CLK_FREQ_667MHZ 5  /* DDR 3 */
#define	  DCH_MEM_CLK_FREQ_800MHZ 6  /* DDR 3 */
#define  DCH_MEM_CLK_FREQ_VAL	(1 << 3)
#define  DCH_DDR3_MODE		(1 << 8)
#define  DCH_LEGACY_BIOS_MODE	(1 << 9)
#define  DCH_ZQCS_INTERVAL_SHIFT 10
#define  DCH_ZQCS_INTERVAL_MASK	3
#define  DCH_ZQCS_INTERVAL_DIS	0
#define  DCH_ZQCS_INTERVAL_64_MS	 1
#define  DCH_ZQCS_INTERVAL_128_MS  2
#define  DCH_ZQCS_INTERVAL_256_MS  3
#define  DCH_RDQS_EN	      (1 << 12) /* only for DDR2 */
#define  DCH_DIS_SIMUL_RD_WR	(1 << 13)
#define  DCH_DIS_DRAM_INTERFACE (1 << 14)
#define  DCH_POWER_DOWN_EN      (1 << 15)
#define  DCH_POWER_DOWN_MODE_SHIFT 16
#define  DCH_POWER_DOWN_MODE_MASK 1
#define   DCH_POWER_DOWN_MODE_CHANNEL_CKE 0
#define   DCH_POWER_DOWN_MODE_CHIP_SELECT_CKE 1
#define  DCH_FOUR_RANK_SO_DIMM	(1 << 17)
#define  DCH_FOUR_RANK_R_DIMM	(1 << 18)
#define  DCH_SLOW_ACCESS_MODE	(1 << 20)
#define  DCH_BANK_SWIZZLE_MODE	 (1 << 22)
#define  DCH_DCQ_BYPASS_MAX_SHIFT 24
#define  DCH_DCQ_BYPASS_MAX_MASK	 0xf
#define   DCH_DCQ_BYPASS_MAX_BASE 0
#define   DCH_DCQ_BYPASS_MAX_MIN	 0
#define   DCH_DCQ_BYPASS_MAX_MAX	 15
#define  DCH_FOUR_ACT_WINDOW_SHIFT 28
#define  DCH_FOUR_ACT_WINDOW_MASK 0xf
#define   DCH_FOUR_ACT_WINDOW_BASE 7 /* DDR3 15 */
#define   DCH_FOUR_ACT_WINDOW_MIN 8  /* DDR3 16 */
#define   DCH_FOUR_ACT_WINDOW_MAX 20 /* DDR3 30 */


// for 0x98 index and 0x9c data for DCT0
// for 0x198 index and 0x19c data for DCT1
// even at ganged mode, 0x198/0x19c will be used for channel B

#define DRAM_CTRL_ADDI_DATA_OFFSET	0x98
#define  DCAO_DCT_OFFSET_SHIFT	0
#define  DCAO_DCT_OFFSET_MASK	0x3fffffff
#define  DCAO_DCT_ACCESS_WRITE	(1 << 30)
#define  DCAO_DCT_ACCESS_DONE	(1 << 31)

#define DRAM_CTRL_ADDI_DATA_PORT	 0x9c

#define DRAM_OUTPUT_DRV_COMP_CTRL	0x00
#define  DODCC_CKE_DRV_STREN_SHIFT 0
#define  DODCC_CKE_DRV_STREN_MASK  3
#define   DODCC_CKE_DRV_STREN_1_0X  0
#define   DODCC_CKE_DRV_STREN_1_25X 1
#define   DODCC_CKE_DRV_STREN_1_5X  2
#define   DODCC_CKE_DRV_STREN_2_0X  3
#define  DODCC_CS_ODT_DRV_STREN_SHIFT 4
#define  DODCC_CS_ODT_DRV_STREN_MASK  3
#define   DODCC_CS_ODT_DRV_STREN_1_0X  0
#define   DODCC_CS_ODT_DRV_STREN_1_25X 1
#define   DODCC_CS_ODT_DRV_STREN_1_5X  2
#define   DODCC_CS_ODT_DRV_STREN_2_0X  3
#define  DODCC_ADDR_CMD_DRV_STREN_SHIFT 8
#define  DODCC_ADDR_CMD_DRV_STREN_MASK  3
#define   DODCC_ADDR_CMD_DRV_STREN_1_0X  0
#define   DODCC_ADDR_CMD_DRV_STREN_1_25X 1
#define   DODCC_ADDR_CMD_DRV_STREN_1_5X  2
#define   DODCC_ADDR_CMD_DRV_STREN_2_0X  3
#define  DODCC_CLK_DRV_STREN_SHIFT 12
#define  DODCC_CLK_DRV_STREN_MASK  3
#define   DODCC_CLK_DRV_STREN_0_75X  0
#define   DODCC_CLK_DRV_STREN_1_0X 1
#define   DODCC_CLK_DRV_STREN_1_25X  2
#define   DODCC_CLK_DRV_STREN_1_5X  3
#define  DODCC_DATA_DRV_STREN_SHIFT 16
#define  DODCC_DATA_DRV_STREN_MASK  3
#define   DODCC_DATA_DRV_STREN_0_75X  0
#define   DODCC_DATA_DRV_STREN_1_0X 1
#define   DODCC_DATA_DRV_STREN_1_25X  2
#define   DODCC_DATA_DRV_STREN_1_5X  3
#define  DODCC_DQS_DRV_STREN_SHIFT 20
#define  DODCC_DQS_DRV_STREN_MASK  3
#define   DODCC_DQS_DRV_STREN_0_75X  0
#define   DODCC_DQS_DRV_STREN_1_0X 1
#define   DODCC_DQS_DRV_STREN_1_25X  2
#define   DODCC_DQS_DRV_STREN_1_5X  3
#define  DODCC_PROC_ODT_SHIFT 28
#define  DODCC_PROC_ODT_MASK  3
#define   DODCC_PROC_ODT_300_OHMS  0
#define   DODCC_PROC_ODT_150_OHMS 1
#define   DODCC_PROC_ODT_75_OHMS  2

/*
 * for DDR2 400, 533, 667, F2x[1,0]9C_x[02:01], [03], [06:05], [07] control timing of all DIMMs
 * for DDR2 800, DDR3 800, 1067, 1333, 1600, F2x[1,0]9C_x[02:01], [03], [06:05], [07] control timing of DIMM0
 *                                           F2x[1,0]9C_x[102:101], [103], [106:105], [107] control timing of DIMM1
 *      So Socket F with Four Logical DIMM will only support DDR2 800  ?
*/

/* there are index	   +100	   ===> for DIMM1
 * that are corresponding to 0x01, 0x02, 0x03, 0x05, 0x06, 0x07
*/

//02/15/2006 18:37
#define DRAM_WRITE_DATA_TIMING_CTRL_LOW 0x01
#define  DWDTC_WR_DAT_FINE_DLY_BYTE_0_SHIFT 0
#define  DWDTC_WR_DAT_FINE_DLY_BYTE_MASK  0x1f
#define   DWDTC_WR_DAT_FINE_DLY_BYTE_BASE 0
#define   DWDTC_WR_DAT_FINE_DLY_BYTE_MIN  0
#define   DWDTC_WR_DAT_FINE_DLY_BYTE_MAX  31 // 1/64 MEMCLK
#define  DWDTC_WR_DAT_GROSS_DLY_BYTE_0_SHIFT 5
#define  DWDTC_WR_DAT_GROSS_DLY_BYTE_MASK	0x3
#define   DWDTC_WR_DAT_GROSS_DLY_BYTE_NO_DELAY_MASK 0
#define   DWDTC_WR_DAT_GROSS_DLY_BYTE_0_5_	 1
#define   DWDTC_WR_DAT_GROSS_DLY_BYTE_1  2
#define  DWDTC_WR_DAT_FINE_DLY_BYTE_1_SHIFT 8
#define  DWDTC_WR_DAT_GROSS_DLY_BYTE_1_SHIFT 13
#define  DWDTC_WR_DAT_FINE_DLY_BYTE_2_SHIFT 16
#define  DWDTC_WR_DAT_GROSS_DLY_BYTE_2_SHIFT 21
#define  DWDTC_WR_DAT_FINE_DLY_BYTE_3_SHIFT 24
#define  DWDTC_WR_DAT_GROSS_DLY_BYTE_3_SHIFT 29

#define DRAM_WRITE_DATA_TIMING_CTRL_HIGH 0x02
#define  DWDTC_WR_DAT_FINE_DLY_BYTE_4_SHIFT 0
#define  DWDTC_WR_DAT_GROSS_DLY_BYTE_4_SHIFT 5
#define  DWDTC_WR_DAT_FINE_DLY_BYTE_5_SHIFT 8
#define  DWDTC_WR_DAT_GROSS_DLY_BYTE_5_SHIFT 13
#define  DWDTC_WR_DAT_FINE_DLY_BYTE_6_SHIFT 16
#define  DWDTC_WR_DAT_GROSS_DLY_BYTE_6_SHIFT 21
#define  DWDTC_WR_DAT_FINE_DLY_BYTE_7_SHIFT 24
#define  DWDTC_WR_DAT_GROSS_DLY_BYTE_7_SHIFT 29

#define DRAM_WRITE_ECC_TIMING_CTRL 0x03
#define  DWETC_WR_CHK_FIN_DLY_SHIFT 0
#define  DWETC_WR_CHK_GROSS_DLY_SHIFT 5

#define DRAM_ADDR_CMD_TIMING_CTRL 0x04
#define  DACTC_CKE_FINE_DELAY_SHIFT 0
#define  DACTC_CKE_FINE_DELAY_MASK  0x1f
#define   DACTC_CKE_FINE_DELAY_BASE 0
#define   DACTC_CKE_FINE_DELAY_MIN  0
#define   DACTC_CKE_FINE_DELAY_MAX 31
#define  DACTC_CKE_SETUP	(1 << 5)
#define  DACTC_CS_ODT_FINE_DELAY_SHIFT 8
#define  DACTC_CS_ODT_FINE_DELAY_MASK  0x1f
#define   DACTC_CS_ODT_FINE_DELAY_BASE 0
#define   DACTC_CS_ODT_FINE_DELAY_MIN  0
#define   DACTC_CS_ODT_FINE_DELAY_MAX 31
#define  DACTC_CS_ODT_SETUP   (1 << 13)
#define  DACTC_ADDR_CMD_FINE_DELAY_SHIFT 16
#define  DACTC_ADDR_CMD_FINE_DELAY_MASK  0x1f
#define   DACTC_ADDR_CMD_FINE_DELAY_BASE 0
#define   DACTC_ADDR_CMD_FINE_DELAY_MIN  0
#define   DACTC_ADDR_CMD_FINE_DELAY_MAX 31
#define  DACTC_ADDR_CMD_SETUP   (1 << 21)

#define DRAM_READ_DQS_TIMING_CTRL_LOW 0x05
#define  DRDTC_RD_DQS_TIME_BYTE_0_SHIFT 0
#define  DRDTC_RD_DQS_TIME_BYTE_MASK  0x3f
#define   DRDTC_RD_DQS_TIME_BYTE_BASE 0
#define   DRDTC_RD_DQS_TIME_BYTE_MIN  0
#define   DRDTC_RD_DQS_TIME_BYTE_MAX  63 // 1/128 MEMCLK
#define  DRDTC_RD_DQS_TIME_BYTE_1_SHIFT 8
#define  DRDTC_RD_DQS_TIME_BYTE_2_SHIFT 16
#define  DRDTC_RD_DQS_TIME_BYTE_3_SHIFT 24

#define DRAM_READ_DQS_TIMING_CTRL_HIGH 0x06
#define  DRDTC_RD_DQS_TIME_BYTE_4_SHIFT 0
#define  DACTC_RD_DQS_TIME_BYTE_5_SHIFT 8
#define  DACTC_RD_DQS_TIME_BYTE_6_SHIFT 16
#define  DACTC_RD_DQS_TIME_BYTE_7_SHIFT 24

#define DRAM_READ_DQS_ECC_TIMING_CTRL 0x07
#define  DRDETC_RD_DQS_TIME_CHECK_SHIFT 0

#define DRAM_PHY_CTRL 0x08
#define  DPC_WRT_LV_TR_EN	(1 << 0)
#define  DPC_WRT_LV_TR_MODE (1 << 1)
#define  DPC_TR_NIBBLE_SEL (1 << 2)
#define  DPC_TR_DIMM_SEL_SHIFT 4
#define   DPC_TR_DIMM_SEL_MASK 3 /* 0-->dimm0, 1-->dimm1, 2--->dimm2, 3--->dimm3 */
#define  DPC_WR_LV_ODT_SHIFT 8
#define   DPC_WR_LV_ODT_MASK 0xf /* bit 0-->odt 0, ...*/
#define  DPC_WR_LV_ODT_EN (1 << 12)
#define  DPC_DQS_RCV_TR_EN (1 << 13)
#define  DPC_DIS_AUTO_COMP (1 << 30)
#define  DPC_ASYNC_COMP_UPDATE (1 << 31)

#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_0_0 0x10 // DIMM0 Channel A
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_0_SHIFT 0
#define   DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_0_MASK 0x1f
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_0_SHIFT 5
#define   DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_0_MASK 0x3
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_1_SHIFT 8
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_1_SHIFT 13
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_2_SHIFT 16
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_2_SHIFT 21
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_3_SHIFT 24
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_3_SHIFT 29

#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_0_1 0x11 // DIMM0 Channel A
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_4_SHIFT 0
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_4_SHIFT 5
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_5_SHIFT 8
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_5_SHIFT 13
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_6_SHIFT 16
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_6_SHIFT 21
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_7_SHIFT 24
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_7_SHIFT 29

#define DRAM_DQS_RECV_ENABLE_TIMING_CTRL_ECC_0_0 0x12
#define  DDRETCE_WR_CHK_FINE_DLY_BYTE_0_SHIFT 0
#define  DDRETCE_WR_CHK_GROSS_DLY_BYTE_0_SHIFT 5

#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_0_2 0x20   // DIMM0 channel B
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_8_SHIFT 0
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_8_SHIFT 5
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_9_SHIFT 8
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_9_SHIFT 13
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_10_SHIFT 16
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_10_SHIFT 21
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE_11_SHIFT 24
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE_11_SHIFT 29

#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_0_3 0x21  // DIMM0 Channel B
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE12_SHIFT 0
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE12_SHIFT 5
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE13_SHIFT 8
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE13_SHIFT 13
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE14_SHIFT 16
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE14_SHIFT 21
#define  DDRETC_DQS_RCV_EN_FINE_DELAY_BYTE15_SHIFT 24
#define  DDRETC_DQS_RCV_EN_GROSS_DELAY_BYTE15_SHIFT 29

#define DRAM_DQS_RECV_ENABLE_TIMING_CTRL_ECC_0_1 0x22
#define  DDRETCE_WR_CHK_FINE_DLY_BYTE_1_SHIFT 0
#define  DDRETCE_WR_CHK_GROSS_DLY_BYTE_1_SHIFT 5

#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_1_0 0x13  //DIMM1
#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_1_1 0x14
#define DRAM_DQS_RECV_ENABLE_TIMING_CTRL_ECC_1_0 0x15
#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_1_2 0x23
#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_1_3 0x24
#define DRAM_DQS_RECV_ENABLE_TIMING_CTRL_ECC_1_1 0x25

#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_2_0 0x16   // DIMM2
#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_2_1 0x17
#define DRAM_DQS_RECV_ENABLE_TIMING_CTRL_ECC_2_0 0x18
#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_2_2 0x26
#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_2_3 0x27
#define DRAM_DQS_RECV_ENABLE_TIMING_CTRL_ECC_2_1 0x28

#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_3_0 0x19   // DIMM3
#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_3_1 0x1a
#define DRAM_DQS_RECV_ENABLE_TIMING_CTRL_ECC_3_0 0x1b
#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_3_2 0x29
#define DRAM_DQS_RECV_ENABLE_TIME_CTRL_3_3 0x2a
#define DRAM_DQS_RECV_ENABLE_TIMING_CTRL_ECC_3_1 0x2b

/* 04.06.2006 19:12 */

#define DRAM_PHASE_RECOVERY_CTRL_0 0x50
#define  DPRC_PH_REC_FINE_DLY_BYTE_0_SHIFT 0
#define   DDWTC_PH_REC_FINE_DLY_BYTE_0_MASK 0x1f
#define  DDWTC_PH_REC_GROSS_DLY_BYTE_0_SHIFT 5
#define   DDWTC_PH_REC_GROSS_DLY_BYTE_0_MASK 0x3
#define  DDWTC_PH_REC_FINE_DLY_BYTE_1_SHIFT 8
#define  DDWTC_PH_REC_GROSS_DLY_BYTE_1_SHIFT 13
#define  DDWTC_PH_REC_FINE_DLY_BYTE_2_SHIFT 16
#define  DDWTC_PH_REC_GROSS_DLY_BYTE_2_SHIFT 21
#define  DDWTC_PH_REC_FINE_DLY_BYTE_3_SHIFT 24
#define  DDWTC_PH_REC_GROSS_DLY_BYTE_3_SHIFT 29

#define DRAM_PHASE_RECOVERY_CTRL_1 0x51
#define  DPRC_PH_REC_FINE_DLY_BYTE_4_SHIFT 0
#define  DDWTC_PH_REC_GROSS_DLY_BYTE_4_SHIFT 5
#define  DDWTC_PH_REC_FINE_DLY_BYTE_5_SHIFT 8
#define  DDWTC_PH_REC_GROSS_DLY_BYTE_5_SHIFT 13
#define  DDWTC_PH_REC_FINE_DLY_BYTE_6_SHIFT 16
#define  DDWTC_PH_REC_GROSS_DLY_BYTE_6_SHIFT 21
#define  DDWTC_PH_REC_FINE_DLY_BYTE_7_SHIFT 24
#define  DDWTC_PH_REC_GROSS_DLY_BYTE_7_SHIFT 29

#define DRAM_ECC_PHASE_RECOVERY_CTRL 0x52
#define  DEPRC_PH_REC_ECC_DLY_BYTE_0_SHIFT 0
#define  DEPRC_PH_REC_ECC_GROSS_DLY_BYTE_0_SHIFT 5

#define DRAM_WRITE_LEVEL_ERROR 0x53 /* read only */
#define  DWLE_WR_LV_ERR_SHIFT 0
#define   DWLE_WR_LV_ERR_MASK 0xff

#define DRAM_CTRL_MISC 0xa0
#define  DCM_MEM_CLEARED (1 << 0) /* RD  == F2x110 [MemCleared] */
#define  DCM_DRAM_ENABLED (1 << 9) /* RD == F2x110 [DramEnabled] */

#define NB_TIME_STAMP_COUNT_LOW 0xb0
#define  TSC_LOW_SHIFT 0
#define  TSC_LOW_MASK 0xffffffff

#define NB_TIME_STAMP_COUNT_HIGH 0xb4
#define  TSC_HIGH_SHIFT 0
#define  TSC_HIGH_MASK 0xff

#define DCT_DEBUG_CTRL 0xf0 /* 0xf0 for DCT0,	0x1f0 is for DCT1*/
#define  DDC_DLL_ADJUST_SHIFT 0
#define   DDC_DLL_ADJUST_MASK 0xff
#define  DDC_DLL_SLOWER (1 << 8)
#define  DDC_DLL_FASTER (1 << 9)
#define  DDC_WRT_DQS_ADJUST_SHIFT 16
#define   DDC_WRT_DQS_ADJUST_MASK 0x7
#define  DDC_WRT_DQS_ADJUST_EN (1 << 19)

#define DRAM_CTRL_SEL_LOW 0x110
#define  DCSL_DCT_SEL_HI_RNG_EN (1 << 0)
#define  DCSL_DCT_SEL_HI (1 << 1)
#define  DCSL_DCT_SEL_INTLV_EN (1 << 2)
#define  DCSL_MEM_CLR_INIT (1 << 3) /* WR only */
#define  DCSL_DCT_GANG_EN (1 << 4)
#define  DCSL_DCT_DATA_INTLV (1 << 5)
#define  DCSL_DCT_SEL_INTLV_ADDR_SHIFT
#define   DCSL_DCT_SEL_INTLV_ADDR_MASK 3
#define  DCSL_DRAM_ENABLE (1 << 8)  /* RD only */
#define  DCSL_MEM_CLR_BUSY (1 << 9)  /* RD only */
#define  DCSL_MEM_CLEARED (1 << 10) /* RD only */
#define  DCSL_DCT_SEL_BASE_ADDR_47_27_SHIFT 11
#define   DCSL_DCT_SEL_BASE_ADDR_47_27_MASK 0x1fffff

#define DRAM_CTRL_SEL_HIGH 0x114
#define  DCSH_DCT_SEL_BASE_OFFSET_47_26_SHIFT 10
#define   DCSH_DCT_SEL_BASE_OFFSET_47_26_MASK 0x3fffff

#define MEM_CTRL_CONF_LOW 0x118
#define  MCCL_MCT_PRI_CPU_RD (1 << 0)
#define  MCCL_MCT_PRI_CPU_WR (1 << 1)
#define  MCCL_MCT_PRI_ISOC_RD_SHIFT 4
#define   MCCL_MCT_PRI_ISOC_MASK 0x3
#define  MCCL_MCT_PRI_ISOC_WR_SHIFT 6
#define   MCCL_MCT_PRI_ISOC_WE_MASK 0x3
#define  MCCL_MCT_PRI_DEFAULT_SHIFT 8
#define   MCCL_MCT_PRI_DEFAULT_MASK 0x3
#define  MCCL_MCT_PRI_WR_SHIFT 10
#define   MCCL_MCT_PRI_WR_MASK 0x3
#define  MCCL_MCT_PRI_ISOC_SHIFT 12
#define   MCCL_MCT_PRI_ISOC_MASK 0x3
#define  MCCL_MCT_PRI_TRACE_SHIFT 14
#define   MCCL_MCT_PRI_TRACE_MASK 0x3
#define  MCCL_MCT_PRI_SCRUB_SHIFT 16
#define   MCCL_MCT_PRI_SCRUB_MASK 0x3
#define  MCCL_MCQ_MED_PRI_BYPASSMAX_SHIFT 20
#define   MCCL_MCQ_MED_PRI_BYPASSMAX_MASK 0x7
#define  MCCL_MCQ_HI_PRI_BYPASS_MAX_SHIFT 24
#define   MCCL_MCQ_HI_PRI_BYPASS_MAX_MASK 0x7
#define  MCCL_MCT_VAR_PRI_CNT_LMT_SHIFT 28
#define   MCCL_MCT_VAR_PRI_CNT_LMT_MASK 0x7

#define MEM_CTRL_CONF_HIGH 0x11c
#define  MCCH_DCT_WR_LIMIT_SHIFT 0
#define   MCCH_DCT_WR_LIMIT_MASK 0x3
#define  MCCH_MCT_WR_LIMIT_SHIFT 2
#define   MCCH_MCT_WR_LIMIT_MASK 0x1f
#define  MCCH_MCT_PREF_REQ_LIMIT_SHIFT 7
#define   MCCH_MCT_PREF_REQ_LIMIT_MASK 0x1f
#define  MCCH_PREF_CPU_DIS (1 << 12)
#define  MCCH_PREF_IO_DIS (1 << 13)
#define  MCCH_PREF_IO_FIX_STRIDE_EN (1 << 14)
#define  MCCH_PREF_FIX_STRIDE_EN (1 << 15)
#define  MCCH_PREF_FIX_DIST_SHIFT 16
#define   MCCH_PREF_FIX_DIST_MASK 0x3
#define  MCCH_PREF_CONF_SAT_SHIFT 18
#define   MCCH_PREF_CONF_SAT_MASK 0x3
#define  MCCH_PREF_ONE_CONF_SHIFT 20
#define   MCCH_PREF_ONE_CONF_MASK 0x3
#define  MCCH_PREF_TWO_CONF_SHIFT 22
#define   MCCH_PREF_TWO_CONF_MASK 0x7
#define  MCCH_PREF_THREE_CONF_SHIFT 25
#define   MCCH_PREF_THREE_CONF_MASK 0x7
#define  MCCH_PREF_DRAM_TRAIN_MODE (1 << 28)
#define  MCCH_FLUSH_WR_ON_STP_GNT (1 << 29)
#define  MCCH_FLUSH_WR (1 << 30)
#define  MCCH_MCT_SCRUB_EN (1 << 31)


/* Function 3 */
#define MCA_NB_CONTROL		0x40
#define  MNCT_CORR_ECC_EN (1 << 0)
#define  MNCT_UN_CORR_ECC_EN (1 << 1)
#define  MNCT_CRC_ERR_0_EN	(1 << 2) /* Link 0 */
#define  MNCT_CRC_ERR_1_EN (1 << 3)
#define  MNCT_CRC_ERR_2_EN (1 << 4)
#define  MBCT_SYNC_PKT_0_EN (1 << 5) /* Link 0 */
#define  MBCT_SYNC_PKT_1_EN (1 << 6)
#define  MBCT_SYNC_PKT_2_EN (1 << 7)
#define  MBCT_MSTR_ABRT_EN (1 << 8)
#define  MBCT_TGT_ABRT_EN	 (1 << 9)
#define  MBCT_GART_TBL_EK_EN (1 << 10)
#define  MBCT_ATOMIC_RMW_EN  (1 << 11)
#define  MBCT_WDOG_TMR_RPT_EN (1 << 12)
#define  MBCT_DEV_ERR_EN	(1 << 13)
#define  MBCT_L3ARRAY_COR_EN (1 << 14)
#define  MBCT_L3ARRAY_UNC_EN (1 << 15)
#define  MBCT_HT_PROT_EN	(1 << 16)
#define  MBCT_HT_DATA_EN	(1 << 17)
#define  MBCT_DRAM_PAR_EN	(1 << 18)
#define  MBCT_RTRY_HT_0_EN (1 << 19) /* Link 0 */
#define  MBCT_RTRY_HT_1_EN (1 << 20)
#define  MBCT_RTRY_HT_2_EN (1 << 21)
#define  MBCT_RTRY_HT_3_EN (1 << 22)
#define  MBCT_CRC_ERR_3_EN (1 << 23) /* Link 3*/
#define  MBCT_SYNC_PKT3EN (1 << 24) /* Link 4 */
#define  MBCT_MCA_US_PW_DAT_ERR_EN (1 << 25)
#define  MBCT_NB_ARRAY_PAR_EN (1 << 26)
#define  MBCT_TBL_WLK_DAT_ERR_EN (1 << 27)
#define  MBCT_FB_DIMM_COR_ERR_EN (1 << 28)
#define  MBCT_FB_DIMM_UN_COR_ERR_EN (1 << 29)



#define MCA_NB_CONFIG	    0x44
#define   MNC_CPU_RD_DAT_ERR_EN   (1 << 1)
#define   MNC_SYNC_ON_UC_ECC_EN   (1 << 2)
#define   MNC_SYNV_PKT_GEN_DIS   (1 << 3)
#define   MNC_SYNC_PKT_PROP_DIS  (1 << 4)
#define   MNC_IO_MST_ABORT_DIS   (1 << 5)
#define   MNC_CPU_ERR_DIS	      (1 << 6)
#define   MNC_IO_ERR_DIS	      (1 << 7)
#define   MNC_WDOG_TMR_DIS      (1 << 8)
#define   MNC_WDOG_TMR_CNT_SEL_2_0_SHIFT 9 /* 3 is ar f3x180 */
#define    MNC_WDOG_TMR_CNT_SEL_2_0_MASK 0x3
#define   MNC_WDOG_TMR_BASE_SEL_SHIFT 12
#define    MNC_WDOG_TMR_BASE_SEL_MASK 0x3
#define   MNC_LDT_LINK_SEL_SHIFT 14
#define    MNC_LDT_LINK_SEL_MASK 0x3
#define   MNC_GEN_CRC_ERR_BYTE0	(1 << 16)
#define   MNC_GEN_CRC_ERR_BYTE1	(1 << 17)
#define   MNC_SUB_LINK_SEL_SHIFT 18
#define    MNC_SUB_LINK_SEL_MASK 0x3
#define   MNC_SYNC_ON_WDOG_EN  (1 << 20)
#define   MNC_SYNC_ON_ANY_ERR_EN (1 << 21)
#define   MNC_DRAM_ECC_EN       (1 << 22)
#define   MNC_CHIPKILL_ECC_EN  (1 << 23)
#define   MNC_IO_RD_DAT_ERR_EN (1 << 24)
#define   MNC_DIS_PCI_CFG_CPU_ERR_RSP (1 << 25)
#define   MNC_CORR_MCA_EXC_EN (1 << 26)
#define   MNC_NB_MCA_TO_MST_CPU_EN (1 << 27)
#define   MNC_DIS_TGT_ABT_CPU_ERR_RSP (1 << 28)
#define   MNC_DIS_MST_ABT_CPU_ERR_RSP (1 << 29)
#define   MNC_SYNC_ON_DRAM_ADR_PAR_ERR_EN (1 << 30)
#define   MNC_NB_MCA_LOG_EN (1 << 31)

#define MCA_NB_STATUS_LOW 0x48
#define  MNSL_ERROR_CODE_SHIFT 0
#define   MNSL_ERROR_CODE_MASK 0xffff
#define  MNSL_ERROR_CODE_EXT_SHIFT 16
#define   MNSL_ERROR_CODE_EXT_MASK 0x1f
#define  MNSL_SYNDROME_15_8_SHIFT 24
#define   MNSL_SYNDROME_15_8_MASK 0xff

#define MCA_NB_STATUS_HIGH 0x4c
#define  MNSH_ERR_CPU_SHIFT 0
#define   MNSH_ERR_CPU_MASK 0xf
#define  MNSH_LDT_LINK_SHIFT 4
#define   MNSH_LD_TLINK_MASK 0xf
#define  MNSH_ERR_SCRUB (1 << 8)
#define  MNSH_SUB_LINK (1 << 9)
#define  MNSH_MCA_STATUS_SUB_CACHE_SHIFT 10
#define   MNSH_MCA_STATUS_SUB_CACHE_MASK 0x3
#define  MNSH_DEFFERED (1 << 12)
#define  MNSH_UN_CORR_ECC (1 << 13)
#define  MNSH_CORR_ECC (1 << 14)
#define  MNSH_SYNDROME_7_0_SHIFT 15
#define   MNSH_SYNDROME_7_0_MASK 0xff
#define  MNSH_PCC (1 << 25)
#define  MNSH_ERR_ADDR_VAL (1 << 26)
#define  MNSH_ERR_MISC_VAL (1 << 27)
#define  MNSH_ERR_EN  (1 << 28)
#define  MNSH_ERR_UN_CORR (1 << 29)
#define  MNSH_ERR_OVER (1 << 30)
#define  MNSH_ERR_VALID (1 << 31)

#define MCA_NB_ADDR_LOW 0x50
#define  MNAL_ERR_ADDR_31_1_SHIFT 1
#define   MNAL_ERR_ADDR_31_1_MASK 0x7fffffff

#define MCA_NB_ADDR_HIGH 0x54
#define  MNAL_ERR_ADDR_47_32_SHIFT 0
#define   MNAL_ERR_ADDR_47_32_MASK 0xffff

#define DRAM_SCRUB_RATE_CTRL	   0x58
#define	  SCRUB_NONE	    0
#define	  SCRUB_40NS	    1
#define	  SCRUB_80NS	    2
#define	  SCRUB_160NS	    3
#define	  SCRUB_320NS	    4
#define	  SCRUB_640NS	    5
#define	  SCRUB_1_28US	    6
#define	  SCRUB_2_56US	    7
#define	  SCRUB_5_12US	    8
#define	  SCRUB_10_2US	    9
#define	  SCRUB_20_5US	   0xa
#define	  SCRUB_41_0US	   0xb
#define	  SCRUB_81_9US	   0xc
#define	  SCRUB_163_8US	   0xd
#define	  SCRUB_327_7US	   0xe
#define	  SCRUB_655_4US	   0xf
#define	  SCRUB_1_31MS	   0x10
#define	  SCRUB_2_62MS	   0x11
#define	  SCRUB_5_24MS	   0x12
#define	  SCRUB_10_49MS	   0x13
#define	  SCRUB_20_97MS	   0x14
#define	  SCRUB_42MS	   0x15
#define	  SCRUB_84MS	   0x16
#define	 DSRC_DRAM_SCRUB_SHFIT  0
#define	  DSRC_DRAM_SCRUB_MASK	0x1f
#define	 DSRC_L2_SCRUB_SHIFT	   8
#define	  DSRC_L2_SCRUB_MASK	   0x1f
#define	 DSRC_DCACHE_SCRUB_SHIFT	  16
#define	  DSRC_DCACHE_SCRUB_MASK	   0x1f
#define	 DSRC_L3_SCRUB_SHIFT	   24
#define	  DSRC_L3_SCRUB_MASK	   0x1f

#define DRAM_SCRUB_ADDR_LOW	   0x5C
#define  DSAL_SCRUB_RE_DIR_EN (1 << 0)
#define  DSAL_SCRUB_ADDR_LO_SHIFT 6
#define   DSAL_SCRUB_ADDR_LO_MASK 0x3ffffff

#define DRAM_SCRUB_ADDR_HIGH	   0x60
#define  DSAH_SCRUB_ADDR_HI_SHIFT 0
#define   DSAH_SCRUB_ADDR_HI_MASK 0xffff

#define HW_THERMAL_CTRL 0x64

#define SW_THERMAL_CTRL 0x68

#define DATA_BUF_CNT 0x6c

#define SRI_XBAR_CMD_BUF_CNT 0x70

#define XBAR_SRI_CMD_BUF_CNT 0x74

#define MCT_XBAR_CMD_BUF_CNT 0x78

#define ACPI_PWR_STATE_CTRL 0x80 /* till 0x84 */

#define NB_CONFIG_LOW 0x88
#define NB_CONFIG_HIGH 0x8c

#define GART_APERTURE_CTRL 0x90

#define GART_APERTURE_BASE 0x94

#define GART_TBL_BASE 0x98

#define GART_CACHE_CTRL 0x9c

#define PWR_CTRL_MISC 0xa0

#define RPT_TEMP_CTRL 0xa4

#define ON_LINE_SPARE_CTRL 0xb0

#define SBI_P_STATE_LIMIT 0xc4

#define CLK_PWR_TIMING_CTRL0 0xd4
#define CLK_PWR_TIMING_CTRL1 0xd8
#define CLK_PWR_TIMING_CTRL2 0xdc

#define THERMTRIP_STATUS 0xE4


#define NORTHBRIDGE_CAP	   0xE8
#define	 NBCAP_TWO_CHAN_DRAM_CAP	      (1 << 0)
#define	 NBCAP_DUAL_NODE_MP_CAP	      (1 << 1)
#define	 NBCAP_EIGHT_NODE_MP_CAP	      (1 << 2)
#define	 NBCAP_ECC_CAP	      (1 << 3)
#define	 NBCAP_CHIP_KILL_ECC_CAP	(1 << 4)
#define	 NBCAP_DDR_MAX_RATE_SHIFT	  5
#define	 NBCAP_DDR_MAX_RATE_MASK	  7
#define	  NBCAP_DDR_MAX_RATE_400	7
#define	  NBCAP_DDR_MAX_RATE_533	6
#define	  NBCAP_DDR_MAX_RATE_667	5
#define	  NBCAP_DDR_MAX_RATE_800	4
#define	  NBCAP_DDR_MAX_RATE_1067 3
#define	  NBCAP_DDR_MAX_RATE_1333 2
#define	  NBCAP_DDR_MAX_RATE_1600 1
#define	  NBCAP_DDR_MAX_RATE_3_2G	 6
#define	  NBCAP_DDR_MAX_RATE_4_0G	 5
#define	  NBCAP_DDR_MAX_RATE_4_8G	 4
#define	  NBCAP_DDR_MAX_RATE_6_4G 3
#define	  NBCAP_DDR_MAX_RATE_8_0G 2
#define	  NBCAP_DDR_MAX_RATE_9_6G 1
#define	 NBCAP_MEM_CTRL_CAP	      (1 << 8)
#define  MBCAP_SVM_CAP	     (1 << 9)
#define  NBCAP_HTC_CAP		(1 << 10)
#define  NBCAP_CMP_CAP_SHIFT	12
#define  NBCAP_CMP_CAP_MASK	3
#define  NBCAP_MP_CAP_SHIFT 16
#define  NBCAP_MP_CAP_MASK 7
#define   NBCAP_MP_CAP_1N 7
#define   NBCAP_MP_CAP_2N 6
#define   NBCAP_MP_CAP_4N 5
#define   NBCAP_MP_CAP_8N 4
#define   NBCAP_MP_CAP_32N 0
#define  NBCAP_UN_GANG_EN_SHIFT 20
#define   NBCAP_UN_GANG_EN_MASK 0xf
#define  NBCAP_L3CAP (1 << 25)
#define  NBCAP_HT_AC_CAP (1 << 26)

/* 04/04/2006 18:00 */

#define EXT_NB_MCA_CTRL	0x180

#define NB_EXT_CONF	0x188
#define DOWNCORE_CTRL	0x190
#define  DWNCC_Dis_Core_SHIFT 0
#define	 DWNCC_Dis_Core_MASK 0xf

/* Function 5 for FBDIMM */
#define FBD_DRAM_TIMING_LOW

#define LINK_CONNECTED	   (1 << 0)
#define INIT_COMPLETE	   (1 << 1)
#define NON_COHERENT	   (1 << 2)
#define CONNECTION_PENDING (1 << 4)

#define MAX_CORES_SUPPORTED 128

struct DCTStatStruc;
struct MCTStatStruc;

struct link_pair_t {
	pci_devfn_t udev;
	u32 upos;
	u32 uoffs;
	pci_devfn_t dev;
	u32 pos;
	u32 offs;
	u8 host;
	u8 node_id;
	u8 linkn;
	u8 rsv;
} __packed;

struct nodes_info_t {
	u32 nodes_in_group; // could be 2, 3, 4, 5, 6, 7, 8
	u32 groups_in_plane; // could be 1, 2, 3, 4, 5
	u32 planes; // could be 1, 2
	u32 up_planes; // down planes will be [up_planes, planes)
} __packed;

struct ht_link_config {
	u32 ht_speed_limit; // Speed in MHz; 0 for autodetect (default)
};

//DDR2 REG and unbuffered : Socket F 1027 and AM3
/* every channel have 4 DDR2 DIMM for socket F
 *		       2 for socket M2/M3
 *		       1 for socket s1g1
 */
struct mem_controller {
	u32 node_id;
	pci_devfn_t f0, f1, f2, f3, f4, f5;
	/* channel0 is DCT0 --- channelA
	 * channel1 is DCT1 --- channelB
	 * can be ganged, a single dual-channel DCT ---> 128 bit
	 *	 or unganged a two single-channel DCTs ---> 64bit
	 * When the DCTs are ganged, the writes to DCT1 set of registers
	 * (F2x1XX) are ignored and reads return all 0's
	 * The exception is the DCT phy registers, F2x[1,0]98, F2x[1,0]9C,
	 * and all the associated indexed registers, are still
	 * independently accessiable
	 */
	/* FIXME: I will only support ganged mode for easy support */
	u8 spd_switch_addr;
	u8 spd_addr[DIMM_SOCKETS * 2];
};

/* be careful with the alignment of sysinfo, bacause sysinfo may be shared by coreboot_car and ramstage stage. and ramstage may be running at 64bit later.*/

struct sys_info {
	int32_t needs_reset;

	u8 ln[NODE_NUMS * NODE_NUMS];// [0, 3] link n, [4, 7] will be hop num
	u16 ln_tn[NODE_NUMS * 8]; // for 0x0zzz: bit [0,7] target node num, bit[8,11] respone link from target num; 0x80ff mean not inited, 0x4yyy mean non coherent and yyy is link pair index
	struct nodes_info_t nodes_info;
	u32 nodes;

	u8 host_link_freq[NODE_NUMS * 8]; // record freq for every link from cpu, 0x0f means don't need to touch it
	u16 host_link_freq_cap[NODE_NUMS * 8]; //cap

	struct ht_link_config ht_link_cfg;

	u32 segbit;
	u32 sbdn;
	u32 sblk;
	u32 sbbusn;

	u32 ht_c_num;
	u32 ht_c_conf_bus[HC_NUMS]; // 4-->32

	struct link_pair_t link_pair[HC_NUMS * 4];// enough? only in_conherent, 32 chain and every chain have 4 HT device
	u32 link_pair_num;

	struct mem_controller ctrl[NODE_NUMS];

	struct MCTStatStruc MCTstat;
	struct DCTStatStruc DCTstatA[NODE_NUMS];
} __packed;

void mainboard_early_init(int s3resume);
void mainboard_sysinfo_hook(struct sys_info *sysinfo);
void mainboard_spd_info(struct sys_info *sysinfo);
void mainboard_after_raminit(struct sys_info *sysinfo);
void setup_mb_resource_map(void);
void setup_resource_map_offset(const u32 *register_values, u32 max, u32
		offset_pci_dev, u32 offset_io_base);
void setup_resource_map_x_offset(const u32 *register_values, u32 max, u32
		offset_pci_dev, u32 offset_io_base);
void setup_resource_map_x(const u32 *register_values, u32 max);
void setup_resource_map(const u32 *register_values, u32 max);

/* reset_test.c */
u32 cpu_init_detected(u8 node_id);
u32 bios_reset_detected(void);
u32 cold_reset_detected(void);
u32 other_reset_detected(void);
u32 warm_reset_detect(u8 node_id);
void distinguish_cpu_resets(u8 node_id);
u32 get_sblk(void);
u8 get_sbbusn(u8 sblk);
void set_bios_reset(void);

struct sys_info *get_sysinfo(void);

void southbridge_ht_init(void);
void southbridge_early_setup(void);
void southbridge_before_pci_init(void);

struct amdfam10_sysconf_t *get_sysconf(void);
void set_pirq_router_bus(u8 bus);
u8 get_pirq_router_bus(void);

BOOL amd_cb_manual_buid_swap_list(u8 node, u8 link, const u8 **list);

unsigned long northbridge_write_acpi_tables(const struct device *device,
					    unsigned long start,
					    struct acpi_rsdp *rsdp);
void northbridge_acpi_write_vars(const struct device *device);


void dump_memory_mapping(void);

struct resource *amdfam10_assign_new_io_res(resource_t base, resource_t size);
struct resource *amdfam10_assign_new_mmio_res(resource_t base, resource_t size);

#endif /* AMDFAM10_H */
