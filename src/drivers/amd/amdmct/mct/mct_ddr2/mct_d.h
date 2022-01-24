/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Description: Include file for all generic DDR 2 MCT files.
 */
#ifndef MCT_D_H
#define MCT_D_H

#define DQS_TRAIN_DEBUG CONFIG(DEBUG_RAM_SETUP)

#include <stdint.h>

#include "mct_d_gcc.h"

extern const u8 table_dqs_rcv_en_offset[];
extern const u32 test_pattern_0_d[];
extern const u32 test_pattern_1_d[];
extern const u32 test_pattern_2_d[];

/*===========================================================================
	CPU - K8/FAM10
===========================================================================*/
#define PT_L1		0		/* CPU Package Type */
#define PT_M2		1
#define PT_S1		2
#define PT_GR		3

#define J_MIN		0		/* j loop constraint. 1 = CL 2.0 T*/
#define J_MAX		5		/* j loop constraint. 5 = CL 7.0 T*/
#define K_MIN		1		/* k loop constraint. 1 = 200 MHz*/
#define K_MAX		5		/* k loop constraint. 5 = 533 MHz*/
#define CL_DEF		2		/* Default value for failsafe operation. 2 = CL 4.0 T*/
#define T_DEF		1		/* Default value for failsafe operation. 1 = 5ns (cycle time)*/

#define BCS_RATE	1		/* reg bit field = rate of dram scrubber for ecc*/
					/* memory initialization (ecc and check-bits).*/
					/* 1 = 40 ns/64 bytes.*/
#define FIRST_PASS	1		/* First pass through RcvEn training*/
#define SECOND_PASS	2		/* Second pass through Rcven training*/

#define RCVR_EN_MARGIN	6		/* number of DLL taps to delay beyond first passing position*/
#define MAXASYNCLATCTL_2	2	/* Max Async Latency Control value*/
#define MAX_ASYNC_LAT_CTL_3	3	/* Max Async Latency Control value*/

#define DQS_FAIL	1
#define DQS_PASS	0
#define DQS_WRITEDIR	1
#define DQS_READDIR	0
#define MIN_DQS_WNDW	3
#define SEC_PASS_OFFSET	6
#define PASS_1_MEM_CLK_DLY	0x20		/* Add 1/2 Memlock delay */
#define MAX_RD_LAT	0x3FF
#define MIN_FENCE	14
#define MAX_FENCE	20
#define MIN_DQS_WR_FENCE	14
#define MAX_DQS_WR_FENCE	20
#define FENCE_TRN_FIN_DLY_SEED	19
#define EARLY_ARB_EN	19

#define PA_HOST(Node)	((((0x18 + Node) << 3) + 0) << 12)	/* Node 0 Host Bus function PCI Address bits [15:0]*/
#define PA_MAP(Node)	((((0x18 + Node) << 3) + 1) << 12)	/* Node 0 MAP function PCI Address bits [15:0]*/
#define PA_DCT(Node)	((((0x18 + Node) << 3) + 2) << 12)	/* Node 0 DCT function PCI Address bits [15:0]*/
//#define PA_EXT_DCT	(((00 << 3)+4) << 8)	/*Node 0 DCT extended configuration registers*/
//#define PA_DCTADDL	(((00 << 3)+2) << 8)	/*Node x DCT function, Additional Registers PCI Address bits [15:0]*/
//#define PA_EXT_DCTADDL (((00 << 3)+5) << 8)	/*Node x DCT function, Additional Registers PCI Address bits [15:0]*/

#define PA_NBMISC(Node)	((((0x18 + Node) << 3) + 3) << 12)	/*Node 0 Misc PCI Address bits [15:0]*/
//#define PA_NBDEVOP	(((00 << 3)+3) << 8)  /*Node 0 Misc PCI Address bits [15:0]*/

#define DCC_EN		1		/* X:2:0x94[19]*/
#define ILD_LMT	3		/* X:2:0x94[18:16]*/

#define ENCODED_T_SPD	0x00191709	/* encodes which SPD byte to get T from*/
					/* versus CL X, CL X-.5, and CL X-1*/

#define BIAS_TRPT	3		/* bias to convert bus clocks to bit field value*/
#define BIAS_TRRDT	2
#define BIAS_TRCDT	3
#define BIAS_TRAST	3
#define BIAS_TRCT	11
#define BIAS_TRTPT	2
#define BIAS_TWRT	3
#define BIAS_TWTRT	0
#define BIAS_TFAW_T	7

#define MIN_TRPT	3		/* min programmable value in busclocks*/
#define MAX_TRPT	6		/* max programmable value in busclocks*/
#define MIN_TRRDT	2
#define MAX_TRRDT	5
#define MIN_TRCDT	3
#define MAX_TRCDT	6
#define MIN_TRAST	5
#define MAX_TRAST	18
#define MIN_TRCT	11
#define MAX_TRCT	26
#define MIN_TRTPT	2
#define MAX_TRTPT	3
#define MIN_TWRT	3
#define MAX_TWRT	6
#define MIN_TWTRT	1
#define MAX_TWTRT	3

/*DDR2-1066 support*/
#define BIAS_TRCDT_1066	5
#define BIAS_TRAST_1066	15
#define BIAS_TRRDT_1066	4
#define BIAS_TWRT_1066		4
#define BIAS_TRPT_1066		5
#define BIAS_TWTRT_1066	4
#define Bias_TfawT_1066	15

#define MIN_TRCDT_1066		5
#define MAX_TRCDT_1066		12
#define MIN_TRAST_1066		15
#define MAX_TRAST_1066		30
#define MIN_TRCT_1066		11
#define MAX_TRCT_1066		42
#define MIN_TRRDT_1066		4
#define MAX_TRRDT_1066		7
#define MIN_TWRT_1066		5
#define MAX_TWRT_1066		8
#define MIN_TRPT_1066		5
#define MAX_TRPT_1066		12
#define MIN_TWTRT_1066		4
#define MAX_TWTRT_1066		7

/*common register bit names*/
#define DRAM_HOLE_VALID		0	/* func 1, offset F0h, bit 0*/
#define DRAM_MEM_HOIST_VALID	1	/* func 1, offset F0h, bit 1*/
#define CS_ENABLE		0	/* func 2, offset 40h-5C, bit 0*/
#define SPARE			1	/* func 2, offset 40h-5C, bit 1*/
#define TEST_FAIL		2	/* func 2, offset 40h-5C, bit 2*/
#define DQS_RCV_EN_TRAIN		18	/* func 2, offset 78h, bit 18*/
#define EN_DRAM_INIT		31	/* func 2, offset 7Ch, bit 31*/
#define DIS_AUTO_REFRESH		18	/* func 2, offset 8Ch, bit 18*/
#define INIT_DRAM		0	/* func 2, offset 90h, bit 0*/
#define BURST_LENGTH_32		10	/* func 2, offset 90h, bit 10*/
#define WIDTH_128		11	/* func 2, offset 90h, bit 11*/
#define X4_DIMM			12	/* func 2, offset 90h, bit 12*/
#define UN_BUFF_DIMM		16	/* func 2, offset 90h, bit 16*/
#define DIMM_EC_EN		19	/* func 2, offset 90h, bit 19*/
#define MEM_CLK_FREQ_VAL		3	/* func 2, offset 94h, bit 3*/
#define RDQS_EN			12	/* func 2, offset 94h, bit 12*/
#define DIS_DRAM_INTERFACE	14	/* func 2, offset 94h, bit 14*/
#define DCT_ACCESS_WRITE		30	/* func 2, offset 98h, bit 30*/
#define DCT_ACCESS_DONE		31	/* func 2, offset 98h, bit 31*/
#define MEM_CLR_STATUS		0	/* func 2, offset A0h, bit 0*/
#define PWR_SAVINGS_EN		10	/* func 2, offset A0h, bit 10*/
#define MOD_64_BIT_MUX		4	/* func 2, offset A0h, bit 4*/
#define DISABLE_JITTER		1	/* func 2, offset A0h, bit 1*/
#define MEM_CLR_DIS		1	/* func 3, offset F8h, FNC 4, bit 1*/
#define SYNC_ON_UC_ECC_EN		2	/* func 3, offset 44h, bit 2*/
#define DR_MEM_CLR_STATUS	10	/* func 3, offset 110h, bit 10*/
#define MEM_CLR_BUSY		9	/* func 3, offset 110h, bit 9*/
#define DCT_GANG_EN		4	/* func 3, offset 110h, bit 4*/
#define MEM_CLR_INIT		3	/* func 3, offset 110h, bit 3*/
#define ASSERT_CKE		28	/* func 2, offset 7Ch, bit 28*/
#define DEASSERT_MEM_RST_X	27	/* func 2, offset 7Ch, bit 27*/
#define SEND_MRS_CMD		26	/* func 2, offset 7Ch, bit 26*/
#define SEND_AUTO_REFRESH	25	/* func 2, offset 7Ch, bit 25*/
#define SEND_PCHG_ALL		24	/* func 2, offset 7Ch, bit 24*/
#define DIS_DQS_BAR		6	/* func 2, offset 90h, bit 6*/
#define DRAM_ENABLED		8	/* func 2, offset 110h, bit 8*/
#define LEGACY_BIOS_MODE		9	/* func 2, offset 94h, bit 9*/
#define PREF_DRAM_TRAIN_MODE	28	/* func 2, offset 11Ch, bit 28*/
#define FLUSH_WR		30	/* func 2, offset 11Ch, bit 30*/
#define DIS_AUTO_COMP		30	/* func 2, offset 9Ch, Index 8, bit 30*/
#define DQS_RCV_TR_EN		13	/* func 2, offset 9Ch, Index 8, bit 13*/
#define FORCE_AUTO_PCHG		23	/* func 2, offset 90h, bit 23*/
#define CL_LINES_TO_NB_DIS	15	/* Bu_CFG2, bit 15*/
#define WB_ENH_WSB_DIS_D		(48-32)
#define PHY_FENCE_TR_EN		3	/* func 2, offset 9Ch, Index 8, bit 3 */
#define PAR_EN			8	/* func 2, offset 90h, bit 8 */
#define DCQ_ARB_BYPASS_EN		19	/* func 2, offset 94h, bit 19 */
#define ACTIVE_CMD_AT_RST		1	/* func 2, offset A8H, bit 1 */
#define FLUSH_WR_ON_STP_GNT	29	/* func 2, offset 11Ch, bit 29 */
#define BANK_SWIZZLE_MODE	22	/* func 2, offset 94h, bit 22 */
#define CH_SETUP_SYNC		15	/* func 2, offset 78h, bit 15 */



/*=============================================================================
	SW Initialization
============================================================================*/
#define DLL_ENABLE	1
#define OCD_DEFAULT	2
#define OCD_EXIT	3



/*=============================================================================
	Jedec DDR II
=============================================================================*/
#define SPD_TYPE	2		/*SPD byte read location*/
	#define JED_DDRSDRAM	0x07	/*Jedec defined bit field*/
	#define JED_DDR2SDRAM	0x08	/*Jedec defined bit field*/

#define SPD_DIMMTYPE	20
#define SPD_ATTRIB	21
	#define JED_DIFCKMSK	0x20	/*Differential Clock Input*/
	#define JED_REGADCMSK	0x11	/*Registered Address/Control*/
	#define JED_PROBEMSK	0x40	/*Analysis Probe installed*/
#define SPD_DEVATTRIB	22
#define SPD_EDCTYPE	11
	#define JED_ECC		0x02
	#define JED_ADRCPAR	0x04
#define SPD_ROWSZ	3
#define SPD_COLSZ	4
#define SPD_LBANKS	17		/*number of [logical] banks on each device*/
#define SPD_DMBANKS	5		/*number of physical banks on dimm*/
	#define SPD_PLBIT	4	/* Dram package bit*/
#define SPD_BANKSZ	31		/*capacity of physical bank*/
#define SPD_DEVWIDTH	13
#define SPD_CASLAT	18
#define SPD_TRP	27
#define SPD_TRRD	28
#define SPD_TRCD	29
#define SPD_TRAS	30
#define SPD_TWR	36
#define SPD_TWTR	37
#define SPD_TRTP	38
#define SPD_TRCRFC	40
#define SPD_TRC	41
#define SPD_TRFC	42

#define SPD_MANID_START		64
#define SPD_PARTN_START		73
#define SPD_PARTN_LENGTH	18
#define SPD_REVNO_START		91

#define SPD_MANDATEYR	93		/*Module Manufacturing Year (BCD)*/

#define SPD_MANDATEWK	94		/*Module Manufacturing Week (BCD)*/

#define SPD_SERIAL_START	95

/*-----------------------------
	Jedec DDR II related equates
-----------------------------*/
#define MYEAR06	6	/* Manufacturing Year BCD encoding of 2006 - 06d*/
#define MWEEK24	0x24	/* Manufacturing Week BCD encoding of June - 24d*/

/*=============================================================================
	Macros
=============================================================================*/

#define _2GB_RJ8	(2 << (30 - 8))
#define _4GB_RJ8	(4 << (30 - 8))
#define _4GB_RJ4	(4 << (30 - 4))

#define BIG_PAGE_X8_RJ8	(1 << (17 + 3 - 8))	/*128KB * 8 >> 8 */

/*=============================================================================
	Global MCT Status Structure
=============================================================================*/
struct MCTStatStruc {
	u32 g_status;		/* Global Status bitfield*/
	u32 hole_base;		/* If not zero, BASE[39:8] (system address)
				      of sub 4GB dram hole for HW remapping.*/
	u32 sub_4G_cache_top;	/* If not zero, the 32-bit top of cacheable memory.*/
	u32 sys_limit;		/* LIMIT[39:8] (system address)*/
} __packed;

/*=============================================================================
	Global MCT Configuration Status Word (g_status)
=============================================================================*/
/*These should begin at bit 0 of g_status[31:0]*/
#define GSB_MTRR_SHORT	0		/* Ran out of MTRRs while mapping memory*/
#define GSB_ECCDIMMS	1		/* All banks of all Nodes are ECC capable*/
#define GSB_DRAM_ECCDIS	2		/* Dram ECC requested but not enabled.*/
#define GSB_SOFT_HOLE	3		/* A Node Base gap was created*/
#define GSB_HW_HOLE	4		/* A HW dram remap was created*/
#define GSB_NODE_INTLV	5		/* Node Memory interleaving was enabled*/
#define GSB_SP_INTLV_REMAP_HOLE	16	/* Special condition for Node Interleave and HW remapping*/
#define GSB_EN_DIMM_SPARE_NW	17	/* Indicates that DIMM spare can be used without a warm reset */
					/* NOTE: This is a local bit used by memory code */


/*===============================================================================
	Local DCT Status structure (a structure for each DCT)
===============================================================================*/

struct DCTPersistentStatStruc {
	u8 ch_d_dir_b_dqs[2][4][2][9];	/* [A/B] [DIMM1-4] [R/W] [DQS] */
		/* CHA DIMM0 Byte 0 - 7 and Check Write DQS Delay*/
		/* CHA DIMM0 Byte 0 - 7 and Check Read DQS Delay*/
		/* CHA DIMM1 Byte 0 - 7 and Check Write DQS Delay*/
		/* CHA DIMM1 Byte 0 - 7 and Check Read DQS Delay*/
		/* CHB DIMM0 Byte 0 - 7 and Check Write DQS Delay*/
		/* CHB DIMM0 Byte 0 - 7 and Check Read DQS Delay*/
		/* CHB DIMM1 Byte 0 - 7 and Check Write DQS Delay*/
		/* CHB DIMM1 Byte 0 - 7 and Check  Read DQS Delay*/
	u8 ch_d_b_rcvr_dly[2][4][8];	/* [A/B] [DIMM0-3] [DQS] */
		/* CHA DIMM 0 receiver Enable Delay*/
		/* CHA DIMM 1 receiver Enable Delay*/
		/* CHA DIMM 2 receiver Enable Delay*/
		/* CHA DIMM 3 receiver Enable Delay*/

		/* CHB DIMM 0 receiver Enable Delay*/
		/* CHB DIMM 1 receiver Enable Delay*/
		/* CHB DIMM 2 receiver Enable Delay*/
		/* CHB DIMM 3 receiver Enable Delay*/
	u8 ch_d_bc_rcvr_dly[2][4];
		/* CHA DIMM 0 - 4 Check Byte receiver Enable Delay*/
		/* CHB DIMM 0 - 4 Check Byte receiver Enable Delay*/
	u16 host_bios_srvc_1;	/* Word sized general purpose field for use by host BIOS.  Scratch space.*/
	u32 host_bios_srvc_2;	/* Dword sized general purpose field for use by host BIOS.  Scratch space.*/
} __packed;


struct DCTStatStruc {		/* A per Node structure*/
/* DCTStatStruct_F -  start */
	u8 node_id;			/* Node ID of current controller*/
	u8 internal_node_id;	/* Internal Node ID of the current controller */
	u8 dual_node_package;	/* 1 = Dual node package (G34) */
	u8 stop_dtc;		/* Set if the DCT will be stopped */
	u8 err_code;			/* Current error condition of Node
		0= no error
		1= Variance Error, DCT is running but not in an optimal configuration.
		2= Stop Error, DCT is NOT running
		3= Fatal Error, DCT/MCT initialization has been halted.*/
	u32 err_status;		/* Error Status bit Field */
	u32 status;		/* Status bit Field*/
	u8 dimm_addr[8];		/* SPD address of DIMM controlled by MA0_CS_L[0,1]*/
		/* SPD address of..MB0_CS_L[0,1]*/
		/* SPD address of..MA1_CS_L[0,1]*/
		/* SPD address of..MB1_CS_L[0,1]*/
		/* SPD address of..MA2_CS_L[0,1]*/
		/* SPD address of..MB2_CS_L[0,1]*/
		/* SPD address of..MA3_CS_L[0,1]*/
		/* SPD address of..MB3_CS_L[0,1]*/
	u16 dimm_present;		/*For each bit n 0..7, 1 = DIMM n is present.
		DIMM#  Select Signal
		0  MA0_CS_L[0,1]
		1  MB0_CS_L[0,1]
		2  MA1_CS_L[0,1]
		3  MB1_CS_L[0,1]
		4  MA2_CS_L[0,1]
		5  MB2_CS_L[0,1]
		6  MA3_CS_L[0,1]
		7  MB3_CS_L[0,1]*/
	u16 dimm_valid;		/* For each bit n 0..7, 1 = DIMM n is valid and is/will be configured*/
	u16 dimm_mismatch;	/* For each bit n 0..7, 1 = DIMM n is mismatched, channel B is always considered the mismatch */
	u16 dimm_spd_checksum_err;		/* For each bit n 0..7, 1 = DIMM n SPD checksum error*/
	u16 dimm_ecc_present;	/* For each bit n 0..7, 1 = DIMM n is ECC capable.*/
	u16 dimm_parity_present;	/* For each bit n 0..7, 1 = DIMM n is ADR/CMD Parity capable.*/
	u16 dimm_x4_present;	/* For each bit n 0..7, 1 = DIMM n contains x4 data devices.*/
	u16 dimm_x8_present;	/* For each bit n 0..7, 1 = DIMM n contains x8 data devices.*/
	u16 dimm_x16_present;	/* For each bit n 0..7, 1 = DIMM n contains x16 data devices.*/
	u16 dimm_2k_page;		/* For each bit n 0..7, 1 = DIMM n contains 1K page devices.*/
	u8 ma_load[2];		/* Number of devices loading MAA bus*/
		/* Number of devices loading MAB bus*/
	u8 ma_dimms[2];		/*Number of DIMMs loading CH A*/
		/* Number of DIMMs loading CH B*/
	u8 data_load[2];		/*Number of ranks loading CH A DATA*/
		/* Number of ranks loading CH B DATA*/
	u8 dimm_auto_speed;	/*Max valid Mfg. Speed of DIMMs
		1 = 200MHz
		2 = 266MHz
		3 = 333MHz
		4 = 400MHz
		5 = 533MHz*/
	u8 dimm_casl;		/* Min valid Mfg. CL bitfield
		0 = 2.0
		1 = 3.0
		2 = 4.0
		3 = 5.0
		4 = 6.0 */
	u16 dimm_trcd;		/* Minimax trcd*40 (ns) of DIMMs*/
	u16 dimm_trp;		/* Minimax trp*40 (ns) of DIMMs*/
	u16 dimm_trtp;		/* Minimax trtp*40 (ns) of DIMMs*/
	u16 dimm_tras;		/* Minimax Tras*40 (ns) of DIMMs*/
	u16 dimm_trc;		/* Minimax Trc*40 (ns) of DIMMs*/
	u16 dimm_twr;		/* Minimax Twr*40 (ns) of DIMMs*/
	u16 dimm_trrd;		/* Minimax Trrd*40 (ns) of DIMMs*/
	u16 dimm_twtr;		/* Minimax Twtr*40 (ns) of DIMMs*/
	u8 speed;		/* Bus Speed (to set Controller)
		1 = 200MHz
		2 = 266MHz
		3 = 333MHz
		4 = 400MHz */
	u8 cas_latency;		/* CAS latency DCT setting
		0 = 2.0
		1 = 3.0
		2 = 4.0
		3 = 5.0
		4 = 6.0 */
	u8 trcd;		/* DCT trcd (busclocks) */
	u8 trp;			/* DCT trp (busclocks) */
	u8 trtp;		/* DCT trtp (busclocks) */
	u8 tras;		/* DCT Tras (busclocks) */
	u8 trc;			/* DCT Trc (busclocks) */
	u8 twr;			/* DCT Twr (busclocks) */
	u8 trrd;		/* DCT Trrd (busclocks) */
	u8 twtr;		/* DCT Twtr (busclocks) */
	u8 trfc[4];		/* DCT Logical DIMM0 Trfc
		0 = 75ns (for 256Mb devs)
		1 = 105ns (for 512Mb devs)
		2 = 127.5ns (for 1Gb devs)
		3 = 195ns (for 2Gb devs)
		4 = 327.5ns (for 4Gb devs) */
		/* DCT Logical DIMM1 Trfc (see Trfc0 for format) */
		/* DCT Logical DIMM2 Trfc (see Trfc0 for format) */
		/* DCT Logical DIMM3 Trfc (see Trfc0 for format) */
	u16 cs_present;		/* For each bit n 0..7, 1 = Chip-select n is present */
	u16 cs_test_fail;		/* For each bit n 0..7, 1 = Chip-select n is present but disabled */
	u32 dct_sys_base;		/* BASE[39:8] (system address) of this Node's DCTs. */
	u32 dct_hole_base;	/* If not zero, BASE[39:8] (system address) of dram hole for HW remapping.  Dram hole exists on this Node's DCTs. */
	u32 dct_sys_limit;	/* LIMIT[39:8] (system address) of this Node's DCTs */
	u16 preset_max_freq;	/* Maximum OEM defined DDR frequency
		200 = 200MHz (DDR400)
		266 = 266MHz (DDR533)
		333 = 333MHz (DDR667)
		400 = 400MHz (DDR800) */
	u8 _2t_mode;		/* 1T or 2T CMD mode (slow access mode)
		1 = 1T
		2 = 2T */
	u8 trwt_to;		/* DCT TrwtTO (busclocks)*/
	u8 twrrd;		/* DCT Twrrd (busclocks)*/
	u8 twrwr;		/* DCT Twrwr (busclocks)*/
	u8 trdrd;		/* DCT Trdrd (busclocks)*/
	u32 ch_odc_ctl[2];	/* Output Driver Strength (see BKDG FN2:Offset 9Ch, index 00h*/
	u32 ch_addr_tmg[2];	/* Address Bus Timing (see BKDG FN2:Offset 9Ch, index 04h*/
		/* Output Driver Strength (see BKDG FN2:Offset 9Ch, index 20h*/
		/* Address Bus Timing (see BKDG FN2:Offset 9Ch, index 24h*/
	u16 ch_ecc_dqs_like[2];	/* CHA DQS ECC byte like...*/
	u8 ch_ecc_dqs_scale[2];	/* CHA DQS ECC byte scale*/
		/* CHA DQS ECC byte like...*/
		/* CHA DQS ECC byte scale*/
	u8 max_async_lat;		/* Max Asynchronous Latency (ns)*/
	// NOTE: Not used in Barcelona - u8 ch_d_rcvr_dly[2][4];
		/* CHA DIMM 0 - 4 receiver Enable Delay*/
		/* CHB DIMM 0 - 4 receiver Enable Delay */
	// NOTE: Not used in Barcelona - u8 CH_D_B_DQS[2][2][8];
		/* CHA Byte 0-7 Write DQS Delay */
		/* CHA Byte 0-7 Read DQS Delay */
		/* CHB Byte 0-7 Write DQS Delay */
		/* CHB Byte 0-7 Read DQS Delay */
	u32 ptr_pattern_buf_a;	/* Ptr on stack to aligned DQS testing pattern*/
	u32 ptr_pattern_buf_b;	/* Ptr on stack to aligned DQS testing pattern*/
	u8 channel;		/* Current channel (0= CH A, 1 = CH B)*/
	u8 byte_lane;		/* Current Byte Lane (0..7)*/
	u8 direction;		/* Current DQS-DQ training write direction (0 = read, 1 = write)*/
	u8 pattern;		/* Current pattern*/
	u8 dqs_delay;		/* Current DQS delay value*/
	u32 train_errors;	/* Current Training Errors*/

	u32 amc_tsc_delta_lo;	/* Time Stamp Counter measurement of AMC, Low dword*/
	u32 amc_tsc_delta_hi;	/* Time Stamp Counter measurement of AMC, High dword*/
	// NOTE: Not used in Barcelona - u8 CH_D_DIR_MaxMin_B_Dly[2][4][2][2][8];
		/* CH A byte lane 0 - 7 minimum filtered window  passing DQS delay value*/
		/* CH A byte lane 0 - 7 maximum filtered window  passing DQS delay value*/
		/* CH B byte lane 0 - 7 minimum filtered window  passing DQS delay value*/
		/* CH B byte lane 0 - 7 maximum filtered window  passing DQS delay value*/
		/* CH A byte lane 0 - 7 minimum filtered window  passing DQS delay value*/
		/* CH A byte lane 0 - 7 maximum filtered window  passing DQS delay value*/
		/* CH B byte lane 0 - 7 minimum filtered window  passing DQS delay value*/
		/* CH B byte lane 0 - 7 maximum filtered window  passing DQS delay value*/
	u64 logical_cpuid;	/* The logical CPUID of the node*/
	u16 dimm_qr_present;	/* QuadRank DIMM present?*/
	u16 dimm_train_fail;	/* Bitmap showing which dimms failed training*/
	u16 cs_train_fail;	/* Bitmap showing which chipselects failed training*/
	u16 dimm_yr_06;		/* Bitmap indicating which Dimms have a manufactur's year code <= 2006*/
	u16 dimm_wk_2406;		/* Bitmap indicating which Dimms have a manufactur's week code <= 24 of 2006 (June)*/
	u16 dimm_dr_present;	/* Bitmap indicating that Dual Rank Dimms are present*/
	u16 dimm_pl_present;	/* Bitmap indicating that Planar (1) or Stacked (0) Dimms are present.*/
	u16 channel_train_fail;	/* Bitmap showing the channel information about failed Chip Selects
		0 in any bit field indicates channel 0
		1 in any bit field indicates channel 1 */
	u16 cs_usr_test_fail;	/* Chip selects excluded by user */
/* DCTStatStruct_F -  end */

	u16 ch_max_rd_lat[2];	/* Max Read Latency (ns) for DCT 0*/
		/* Max Read Latency (ns) for DCT 1*/
	u8 dimm_valid_dct[2];	/* DIMM# in DCT0*/
				/* DIMM# in DCT1*/
	u8 max_dcts;		/* Max number of DCTs in system*/
	// NOTE: removed u8 DCT. Use ->dev_ for pci R/W;	/*DCT pointer*/
	u8 ganged_mode;		/* Ganged mode enabled, 0 = disabled, 1 = enabled*/
	u8 dr_present;		/* Family 10 present flag, 0 = not Fam10, 1 = Fam10*/
	u32 node_sys_limit;	/* BASE[39:8],for DCT0+DCT1 system address*/
	u8 wr_dat_gross_h;
	u8 dqs_rcv_en_gross_l;
	// NOTE: Not used - u8 NodeSpeed		/* Bus Speed (to set Controller)
		/* 1 = 200MHz */
		/* 2 = 266MHz */
		/* 3 = 333MHz */
	// NOTE: Not used - u8 NodeCASL		/* CAS latency DCT setting
		/* 0 = 2.0 */
		/* 1 = 3.0 */
		/* 2 = 4.0 */
		/* 3 = 5.0 */
		/* 4 = 6.0 */
	u8 trwt_wb;
	u8 curr_rcvr_ch_a_delay;	/* for keep current rcvr_en_dly of chA*/
	u16 t1000;		/* get the t1000 figure (cycle time (ns)*1K)*/
	u8 dqs_rcv_en_pass;	/* for TrainRcvrEn byte lane pass flag*/
	u8 dqs_rcv_en_saved;	/* for TrainRcvrEn byte lane saved flag*/
	u8 seed_pass_1_remainder;	/* for Phy assisted DQS receiver enable training*/

	/* for second pass  - Second pass should never run for Fam10*/
	// NOTE: Not used for Barcelona - u8 CH_D_B_RCVRDLY_1[2][4][8];	/* CHA DIMM 0 receiver Enable Delay*/
		/* CHA DIMM 1 receiver Enable Delay*/
		/* CHA DIMM 2 receiver Enable Delay*/
		/* CHA DIMM 3 receiver Enable Delay*/

		/* CHB DIMM 0 receiver Enable Delay*/
		/* CHB DIMM 1 receiver Enable Delay*/
		/* CHB DIMM 2 receiver Enable Delay*/
		/* CHB DIMM 3 receiver Enable Delay*/

	u8 cl_to_nb_tag;	/* is used to restore ClLinesToNbDis bit after memory */
	u32 node_sys_base;	/* for channel interleave usage */

/* New for LB Support */
	u8 node_present;
	u32 dev_host;
	u32 dev_map;
	u32 dev_dct;
	u32 dev_nbmisc;

	u8 dimm_rows[MAX_DIMMS_SUPPORTED];
	u8 dimm_cols[MAX_DIMMS_SUPPORTED];
	u8 dimm_ranks[MAX_DIMMS_SUPPORTED];
	u8 dimm_banks[MAX_DIMMS_SUPPORTED];
	u8 dimm_width[MAX_DIMMS_SUPPORTED];
	u8 dimm_registered[MAX_DIMMS_SUPPORTED];

	u64 dimm_manufacturer_id[MAX_DIMMS_SUPPORTED];
	char dimm_part_number[MAX_DIMMS_SUPPORTED][SPD_PARTN_LENGTH + 1];
	u16 dimm_revision_number[MAX_DIMMS_SUPPORTED];
	u32 dimm_serial_number[MAX_DIMMS_SUPPORTED];

	/* NOTE: This must remain the last entry in this structure */
	struct DCTPersistentStatStruc persistentData;
} __packed;

/*===============================================================================
	Local Error Status Codes (DCTStatStruc.err_code)
===============================================================================*/
#define SC_RUNNING_OK		0
#define SC_VARIANCE_ERR		1	/* Running non-optimally*/
#define SC_STOP_ERR		2	/* Not Running*/
#define SC_FATAL_ERR		3	/* Fatal Error, MCTB has exited immediately*/

/*===============================================================================
	Local Error Status (DCTStatStruc.err_status[31:0])
===============================================================================*/
#define SB_NO_DIMMS		0
#define SB_DIMM_CHKSUM		1
#define SB_DIMM_MISMATCH_M	2	/* dimm module type(buffer) mismatch*/
#define SB_DIMM_MISMATCH_T	3	/* dimm CL/T mismatch*/
#define SB_DIMM_MISMATCH_O	4	/* dimm organization mismatch (128-bit)*/
#define SB_NO_TRC_TRFC		5	/* SPD missing Trc or Trfc info*/
#define SB_NO_CYC_TIME		6	/* SPD missing byte 23 or 25*/
#define SB_BK_INT_DIS		7	/* Bank interleave requested but not enabled*/
#define SB_DRAM_ECC_DIS		8	/* Dram ECC requested but not enabled*/
#define SB_SPARE_DIS		9	/* Online spare requested but not enabled*/
#define SB_MINIMUM_MODE		10	/* Running in Minimum Mode*/
#define SB_NO_RCVR_EN		11	/* No DQS Receiver Enable pass window found*/
#define SB_CH_A2B_RCVR_EN	12	/* DQS Rcvr En pass window CHA to CH B too large*/
#define SB_SMALL_RCVR		13	/* DQS Rcvr En pass window too small (far right of dynamic range)*/
#define SB_NO_DQS_POS		14	/* No DQS-DQ passing positions*/
#define SB_SMALL_DQS		15	/* DQS-DQ passing window too small*/
#define SB_DCBK_SCRUB_DIS	16	/* DCache scrub requested but not enabled */

/*===============================================================================
	Local Configuration Status (DCTStatStruc.Status[31:0])
===============================================================================*/
#define SB_REGISTERED		0	/* All DIMMs are Registered*/
#define SB_ECC_DIMMS		1	/* All banks ECC capable*/
#define SB_PAR_DIMMS		2	/* All banks Addr/CMD Parity capable*/
#define SB_DIAG_CLKS		3	/* Jedec ALL slots clock enable diag mode*/
#define SB_128_BIT_MODE		4	/* DCT in 128-bit mode operation*/
#define SB_64_MUXED_MODE		5	/* DCT in 64-bit mux'ed mode.*/
#define SB_2T_MODE		6	/* 2T CMD timing mode is enabled.*/
#define SB_SW_NODE_HOLE		7	/* Remapping of Node Base on this Node to create a gap.*/
#define SB_HW_HOLE		8	/* Memory Hole created on this Node using HW remapping.*/
#define SB_OVER_400MHZ		9	/* DCT freq >= 400MHz flag*/
#define SB_DQS_POS_PASS_2	10	/* Using for TrainDQSPos DIMM0/1, when freq >= 400MHz*/
#define SB_DQS_RCV_LIMIT		11	/* Using for DQSRcvEnTrain to know we have reached to upper bound.*/
#define SB_EXT_CONFIG		12	/* Indicator the default setting for extend PCI configuration support*/




/*===============================================================================
	NVRAM/run-time-configurable Items
===============================================================================*/
/*Platform Configuration*/
#define NV_PACK_TYPE		0	/* CPU Package Type (2-bits)
					    0 = NPT L1
					    1 = NPT M2
					    2 = NPT S1*/
#define NV_MAX_NODES		1	/* Number of Nodes/Sockets (4-bits)*/
#define NV_MAX_DIMMS		2	/* Number of DIMM slots for the specified Node ID (4-bits)*/
#define NV_MAX_MEMCLK		3	/* Maximum platform demonstrated Memclock (10-bits)
					    200 = 200MHz (DDR400)
					    266 = 266MHz (DDR533)
					    333 = 333MHz (DDR667)
					    400 = 400MHz (DDR800)*/
#define NV_MIN_MEMCLK		4	/* Minimum platform demonstrated Memclock (10-bits) */
#define NV_ECC_CAP		5	/* Bus ECC capable (1-bits)
					    0 = Platform not capable
					    1 = Platform is capable*/
#define NV_4_RANK_TYPE		6	/* Quad Rank DIMM slot type (2-bits)
					    0 = Normal
					    1 = R4 (4-Rank Registered DIMMs in AMD server configuration)
					    2 = S4 (Unbuffered SO-DIMMs)*/
#define NV_BYP_MAX		7	/* Value to set DcqBypassMax field (See Function 2, Offset 94h, [27:24] of BKDG for field definition).
					    4 = 4 times bypass (normal for non-UMA systems)
					    7 = 7 times bypass (normal for UMA systems)*/
#define NV_RD_WR_QBYP		8	/* Value to set RdWrQByp field (See Function 2, Offset A0h, [3:2] of BKDG for field definition).
					    2 = 8 times (normal for non-UMA systems)
					    3 = 16 times (normal for UMA systems)*/


/*Dram Timing*/
#define NV_MCT_USR_TMG_MODE	10	/* User Memclock Mode (2-bits)
					    0 = Auto, no user limit
					    1 = Auto, user limit provided in NV_MEM_CK_VAL
					    2 = Manual, user value provided in NV_MEM_CK_VAL*/
#define NV_MEM_CK_VAL		11	/* Memory Clock Value (2-bits)
					    0 = 200MHz
					    1 = 266MHz
					    2 = 333MHz
					    3 = 400MHz*/

/*Dram Configuration*/
#define NV_BANK_INTLV		20	/* Dram Bank (chip-select) Interleaving (1-bits)
					    0 = disable
					    1 = enable*/
#define NV_ALL_MEM_CLKS		21	/* Turn on All DIMM clocks (1-bits)
					    0 = normal
					    1 = enable all memclocks*/
#define NV_SPDCHK_RESTRT	22	/* SPD Check control bitmap (1-bits)
					    0 = Exit current node init if any DIMM has SPD checksum error
					    1 = Ignore faulty SPD checksums (Note: DIMM cannot be enabled)*/
#define NV_DQS_TRAIN_CTL		23	/* DQS Signal Timing Training Control
					    0 = skip DQS training
					    1 = perform DQS training*/
#define NV_NODE_INTLV		24	/* Node Memory Interleaving (1-bits)
					    0 = disable
					    1 = enable*/
#define NV_BURST_LEN_32		25	/* BURST_LENGTH_32 for 64-bit mode (1-bits)
					    0 = disable (normal)
					    1 = enable (4 beat burst when width is 64-bits)*/

/*Dram Power*/
#define NV_CKE_PDEN		30	/* CKE based power down mode (1-bits)
					    0 = disable
					    1 = enable*/
#define NV_CKE_CTL		31	/* CKE based power down control (1-bits)
					    0 = per channel control
					    1 = per Chip select control*/
#define NV_CLK_HZ_ALT_VID_C3	32	/* Memclock tri-stating during C3 and Alt VID (1-bits)
					    0 = disable
					    1 = enable*/

/*Memory Map/Mgt.*/
#define NV_BOTTOM_IO		40	/* Bottom of 32-bit IO space (8-bits)
					    NV_BOTTOM_IO[7:0]=Addr[31:24]*/
#define NV_BOTTOM_UMA		41	/* Bottom of shared graphics dram (8-bits)
					    NV_BOTTOM_UMA[7:0]=Addr[31:24]*/
#define NV_MEM_HOLE		42	/* Memory Hole Remapping (1-bits)
					    0 = disable
					    1 = enable  */

/*ECC*/
#define NV_ECC			50	/* Dram ECC enable*/
#define NV_NBECC		52	/* ECC MCE enable*/
#define NV_CHIP_KILL		53	/* Chip-Kill ECC Mode enable*/
#define NV_ECC_REDIR		54	/* Dram ECC Redirection enable*/
#define NV_DRAM_BK_SCRUB		55	/* Dram ECC Background Scrubber CTL*/
#define NV_L2_BK_SCRUB		56	/* L2 ECC Background Scrubber CTL*/
#define NV_L3_BK_SCRUB		57	/* L3 ECC Background Scrubber CTL*/
#define NV_DC_BK_SCRUB		58	/* DCache ECC Background Scrubber CTL*/
#define NV_CS_SPARE_CTL		59	/* Chip Select spare Control bit 0:
					       0 = disable spare
					       1 = enable spare */
					/* Chip Select spare Control bit 1-4:
					     Reserved, must be zero*/
#define NV_SYNC_ON_UN_ECC_EN	61	/* SyncOnUnEccEn control
					   0 = disable
					   1 = enable*/
#define NV_UNGANGED		62

#define NV_CHANNEL_INTLV	63	/* Channel Interleaving (3-bits)
					xx0b = disable
					yy1b = enable with DctSelIntLvAddr set to yyb */

#define NV_MAX_DIMMS_PER_CH	64	/* Maximum number of DIMMs per channel */

#include <northbridge/amd/amdfam10/debug.h>
#include <northbridge/amd/amdfam10/raminit.h>
#include <northbridge/amd/amdfam10/amdfam10.h>

/*===============================================================================
	CBMEM storage
===============================================================================*/
struct amdmct_memory_info {
	struct MCTStatStruc mct_stat;
	struct DCTStatStruc dct_stat[MAX_NODES_SUPPORTED];
	u16 ecc_enabled;
	u16 ecc_scrub_rate;
} __packed;

u32 get_nb32(u32 dev, u32 reg);
void set_nb32(u32 dev, u32 reg, u32 val);
u32 get_nb32_index(u32 dev, u32 index_reg, u32 index);
void set_nb32_index(u32 dev, u32 index_reg, u32 index, u32 data);
u32 get_nb32_index_wait(u32 dev, u32 index_reg, u32 index);
void set_nb32_index_wait(u32 dev, u32 index_reg, u32 index, u32 data);
u32 other_timing_a_d(struct DCTStatStruc *p_dct_stat, u32 val);
void mct_force_auto_precharge_d(struct DCTStatStruc *p_dct_stat, u32 dct);
u32 modify_d3_cmp(struct DCTStatStruc *p_dct_stat, u32 dct, u32 value);
u8 mct_check_number_of_dqs_rcv_en_1_pass(u8 pass);
u32 setup_dqs_pattern_1_pass_a(u8 pass);
u32 setup_dqs_pattern_1_pass_b(u8 pass);
u8 mct_get_start_rcvr_en_dly_1_pass(u8 pass);
u8 mct_average_rcvr_en_dly_pass(struct DCTStatStruc *p_dct_stat, u8 rcvr_en_dly, u8 rcvr_en_dly_limit, u8 channel, u8 receiver, u8 pass);
void cpu_mem_typing_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void uma_mem_typing_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
u8 ecc_init_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void train_receiver_en_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a, u8 pass);
void mct_train_dqs_pos_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void mct_set_ecc_dqs_rcvr_en_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void train_max_read_latency_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void mct_end_dqs_training_d(struct MCTStatStruc *p_mct_stat,struct DCTStatStruc *p_dct_stat_a);
void mct_set_rcvr_en_dly_d(struct DCTStatStruc *p_dct_stat, u8 rcvr_en_dly, u8 final_value, u8 channel, u8 receiver, u32 dev, u32 index_reg, u8 addl_index, u8 pass);
void set_ecc_dqs_rcvr_en_d(struct DCTStatStruc *p_dct_stat, u8 channel);
void mct_get_ps_cfg_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u32 dct);
void interleave_banks_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 dct);
void mct_set_dram_config_hi_d(struct DCTStatStruc *p_dct_stat, u32 dct, u32 DramConfigHi);
void mct_dram_init_hw_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 dct);
void sync_settings(struct DCTStatStruc *p_dct_stat);
void mct_set_cl_to_nb_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat);
void mct_set_wb_enh_wsb_dis_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat);
void mct_train_rcvr_en_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 pass);
void mct_enable_dimm_ecc_en_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 _DisableDramECC);
u32 proc_odt_workaround(struct DCTStatStruc *p_dct_stat, u32 dct, u32 val);
void mct_before_dram_init_d(struct DCTStatStruc *p_dct_stat, u32 dct);
void interleave_nodes_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void interleave_channels_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void mct_before_dqs_train_samp_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat);
void store_dqs_dat_struct_val_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 chip_sel);
void phy_assisted_mem_fence_training(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
u8 mct_save_rcv_en_dly_d_1_pass(struct DCTStatStruc *p_dct_stat, u8 pass);
u32 check_nb_cof_auto_prechg(struct DCTStatStruc *p_dct_stat, u32 dct);
u8 mct_adjust_dqs_pos_delay_d(struct DCTStatStruc *p_dct_stat, u8 dly);
void mct_adjust_scrub_d(struct DCTStatStruc *p_dct_stat, u16 *scrub_request);
u8 mct_init_receiver_d(struct DCTStatStruc *p_dct_stat, u8 dct);
void mct_wait(u32 cycles);
u8 mct_rcvr_rank_enabled_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 channel, u8 chip_sel);
u32 mct_get_rcvr_sys_addr_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 channel, u8 receiver, u8 *valid);
void mct_read_1l_test_pattern_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u32 addr);
void early_sample_support_d(void);

void mct_auto_init_mct_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void mct_adjust_delay_range_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 *dqs_pos);
void mct_enable_dat_intlv_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat);
void mct_mem_clr_sync_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
void before_interleave_channels_d(struct DCTStatStruc *p_dct_stat_a, u8 *enabled);
u8 mct_check_fence_hole_adjust_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 DQSDelay,
				u8 chip_sel,  u8 *result);
void proc_iocl_flush_d(u32 addr_hi);
void mct_write_1l_test_pattern_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat,
				u32 TestAddr, u8 pattern);
u8 node_present_d(u8 Node);
void dct_mem_clr_init_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
void mct_mem_clr_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
void print_debug_dqs(const char *str, u32 val, u8 level);
void print_debug_dqs_pair(const char *str, u32 val, const char *str2, u32 val2, u8 level);
u8 mct_disable_dimm_ecc_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
void reset_dct_wr_ptr_d(u32 dev, u32 index_reg, u32 index);
void set_target_wtio_d(u32 TestAddr);
void reset_target_wtio_d(void);
u32 mct_get_mct_sys_addr_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 channel,
				u8 receiver, u8 *valid);
#endif
