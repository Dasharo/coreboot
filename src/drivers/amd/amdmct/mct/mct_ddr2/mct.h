/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef MCT_H
#define MCT_H

#include <stdint.h>

/*===========================================================================
	CPU - K8/FAM10
===========================================================================*/
#define PT_L1		0		/* CPU Package Type*/
#define PT_M2		1
#define PT_S1		2

#define J_MIN		0		/* j loop constraint. 1 = CL 2.0 T*/
#define J_MAX		4		/* j loop constraint. 4 = CL 6.0 T*/
#define K_MIN		1		/* k loop constraint. 1 = 200 MHz*/
#define K_MAX		4		/* k loop constraint. 9 = 400 MHz*/
#define CL_DEF		2		/* Default value for failsafe operation. 2 = CL 4.0 T*/
#define T_DEF		1		/* Default value for failsafe operation. 1 = 5ns (cycle time)*/

#define BSCRate	1		/* reg bit field = rate of dram scrubber for ecc*/
					/* memory initialization (ecc and check-bits).*/
					/* 1 = 40 ns/64 bytes.*/
#define FIRST_PASS	1		/* First pass through RcvEn training*/
#define SECOND_PASS	2		/* Second pass through Rcven training*/

#define RCVR_EN_MARGIN	6		/* number of DLL taps to delay beyond first passing position*/
#define MAX_ASYNC_LAT_CTL_3	60	/* Max Async Latency Control value (This value will be divided by 20)*/
#define DQS_FAIL	1
#define DQS_PASS	0
#define DQS_WRITEDIR	0
#define DQS_READDIR	1
#define MIN_DQS_WNDW	3
#define SEC_PASS_OFFSET	6

/* Node 0 Host Bus function PCI Address bits [15:0] */
#define PA_HOST	(((24 << 3)+0) << 8)
/* Node 0 MAP function PCI Address bits [15:0] */
#define PA_MAP		(((24 << 3)+1) << 8)
/* Node 0 DCT function PCI Address bits [15:0] */
#define PA_DCT		(((24 << 3)+2) << 8)
/* Node x DCT function, Additional Registers PCI Address bits [15:0] */
#define PA_DCTADDL	(((00 << 3)+2) << 8)
/* Node 0 Misc PCI Address bits [15:0] */
#define PA_NBMISC	(((24 << 3)+3) << 8)
/* Node 0 Misc PCI Address bits [15:0] */
#define PA_NBDEVOP	(((00 << 3)+3) << 8)

#define DCC_EN		1	/* X:2:0x94[19]*/
#define ILD_Lmt		3	/* X:2:0x94[18:16]*/

#define ENCODED_T_SPD	0x00191709	/* encodes which SPD byte to get T from*/
					/* versus CL X, CL X-.5, and CL X-1*/

#define BIAS_TRPT	3		/* bias to convert bus clocks to bit field value*/
#define BIAS_TRRDT	2
#define BIAS_TRCDT	3
#define BIAS_TRAST	3
#define BIAS_TRCT	11
#define BIAS_TRTPT	4
#define BIAS_TWRT	3
#define BIAS_TWTRT	0

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
#define MIN_TRTPT	4
#define MAX_TRTPT	5
#define MIN_TWRT	3
#define MAX_TWRT	6
#define MIN_TWTRT	1
#define MAX_TWTRT	3

/* common register bit names */
#define DRAM_HOLE_VALID	0		/* func 1, offset F0h, bit 0 */
#define CS_ENABLE	0		/* func 2, offset 40h-5C, bit 0 */
#define SPARE		1		/* func 2, offset 40h-5C, bit 1 */
#define TEST_FAIL	2		/* func 2, offset 40h-5C, bit 2 */
#define DQS_RCV_EN_TRAIN	18	/* func 2, offset 78h, bit 18 */
#define EN_DRAM_INIT	31		/* func 2, offset 7Ch, bit 31 */
#define DIS_AUTO_REFRESH	18	/* func 2, offset 8Ch, bit 18 */
#define INIT_DRAM	0		/* func 2, offset 90h, bit 0 */
#define BURST_LENGTH_32	10		/* func 2, offset 90h, bit 10 */
#define WIDTH_128	11		/* func 2, offset 90h, bit 11 */
#define X4_DIMM		12		/* func 2, offset 90h, bit 12 */
#define UN_BUFF_DIMM	16		/* func 2, offset 90h, bit 16 */
#define DIMM_EC_EN	19		/* func 2, offset 90h, bit 19 */
#define MEM_CLK_FREQ_VAL	3		/* func 2, offset 94h, bit 3 */
#define RDQS_EN		12		/* func 2, offset 94h, bit 12 */
#define DIS_DRAM_INTERFACE	14	/* func 2, offset 94h, bit 14 */
#define DCT_ACCESS_WRITE	30		/* func 2, offset 98h, bit 30 */
#define DCT_ACCESS_DONE	31		/* func 2, offset 98h, bit 31 */
#define PWR_SAVINGS_EN	10		/* func 2, offset A0h, bit 10 */
#define MOD_64_BIT_MUX	4		/* func 2, offset A0h, bit 4 */
#define DISABLE_JITTER	1		/* func 2, offset A0h, bit 1 */
#define DRAM_ENABLED	9		/* func 2, offset A0h, bit 9 */
#define SYNC_ON_UC_ECC_EN	2		/* func 3, offset 44h, bit 2 */

/*=============================================================================
	Jedec DDR II
=============================================================================*/
#define SPD_TYPE	2		/* SPD byte read location*/
	#define JED_DDRSDRAM	0x07	/* Jedec defined bit field*/
	#define JED_DDR2SDRAM	0x08	/* Jedec defined bit field*/

#define SPD_DIMMTYPE	20
#define SPD_ATTRIB	21
	#define JED_DIFCKMSK	0x20	/* Differential Clock Input*/
	#define JED_REGADCMSK	0x11	/* Registered Address/Control*/
	#define JED_PROBEMSK	0x40	/* Analysis Probe installed*/
#define SPD_DEVATTRIB	22
#define SPD_EDCTYPE	11
	#define JED_ECC		0x02
	#define JED_ADRCPAR	0x04
#define SPD_ROWSZ	3
#define SPD_COLSZ	4
#define SPD_LBANKS	17		/* number of [logical] banks on each device*/
#define SPD_DMBANKS	5		/* number of physical banks on dimm*/
    #define SPD_PLBIT	4		/* Dram package bit*/
#define SPD_BANKSZ	31		/* capacity of physical bank*/
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

#define SPD_MANDATEYR	93		/* Module Manufacturing Year (BCD) */

#define SPD_MANDATEWK	94		/* Module Manufacturing Week (BCD) */

/*--------------------------------------
	Jedec DDR II related equates
--------------------------------------*/
#define MYEAR06	6		/* Manufacturing Year BCD encoding of 2006 - 06d*/
#define MWEEK24	0x24		/* Manufacturing Week BCD encoding of June - 24d*/

/*=============================================================================
	Macros
=============================================================================*/

#define _2GB_RJ8	(2 << (30 - 8))
#define _4GB_RJ8	(4 << (30 - 8))
#define _4GB_RJ4	(4 << (30 - 4))

#define BIG_PAGE_X8_RJ8	(1 << (17 + 3 - 8))	/* 128KB * 8 >> 8 */

/*=============================================================================
	Global MCT Status Structure
=============================================================================*/
struct MCTStatStruc {
	u32 g_status;	/* Global Status bitfield*/
	u32 hole_base;	/* If not zero, BASE[39:8] (system address)
				   of sub 4GB dram hole for HW remapping.*/
	u32 sub_4G_cache_top;	/* If not zero, the 32-bit top of cacheable memory.*/
	u32 sys_limit;	/* LIMIT[39:8] (system address)*/
};
/*=============================================================================
	Global MCT Configuration Status Word (g_status)
=============================================================================*/
/*These should begin at bit 0 of g_status[31:0]*/
/* Ran out of MTRRs while mapping memory*/
#define GSB_MTRR_SHORT	0
/* All banks of all Nodes are ECC capable*/
#define GSB_ECCDIMMS	1
/* Dram ECC requested but not enabled.*/
#define GSB_DRAM_ECCDIS	2
/* A Node Base gap was created*/
#define GSB_SOFT_HOLE	3
/* A HW dram remap was created*/
#define GSB_HW_HOLE	4
/* Node Memory interleaving was enabled*/
#define GSB_Node_INTLV	5
/* Special condition for Node Interleave and HW remapping*/
#define GSB_SP_INTLV_REMAP_HOLE	16


/*===============================================================================
	Local DCT Status structure (a structure for each DCT)
===============================================================================*/

/* A per Node structure*/
struct DCTStatStruc {
	/* Node ID of current controller*/
	u8 node_id;
	/* Current error condition of Node
	 * 0= no error
	 * 1= Variance Error, DCT is running but not in an optimal configuration.
	 * 2= Stop Error, DCT is NOT running
	 * 3= Fatal Error, DCT/MCT initialization has been halted.
	 */
	u8 err_code;
	/* Error Status bit Field */
	u32 err_status;
	/* Status bit Field*/
	u32 status;
	/* SPD address of DIMM controlled by MA0_CS_L[0,1]*/
	/* SPD address of..MB0_CS_L[0,1]*/
	/* SPD address of..MA1_CS_L[0,1]*/
	/* SPD address of..MB1_CS_L[0,1]*/
	/* SPD address of..MA2_CS_L[0,1]*/
	/* SPD address of..MB2_CS_L[0,1]*/
	/* SPD address of..MA3_CS_L[0,1]*/
	/* SPD address of..MB3_CS_L[0,1]*/
	u8 dimm_addr[8];
	/* For each bit n 0..7, 1 = DIMM n is present.
	 * DIMM#  Select Signal
	 * 0  MA0_CS_L[0,1]
	 * 1  MB0_CS_L[0,1]
	 * 2  MA1_CS_L[0,1]
	 * 3  MB1_CS_L[0,1]
	 * 4  MA2_CS_L[0,1]
	 * 5  MB2_CS_L[0,1]
	 * 6  MA3_CS_L[0,1]
	 * 7  MB3_CS_L[0,1]
	 */
	u16 dimm_present;
	/* For each bit n 0..7, 1 = DIMM n is valid and is/will be configured*/
	u16 dimm_valid;
	/* For each bit n 0..7, 1 = DIMM n SPD checksum error*/
	u16 dimm_spd_checksum_err;
	/* For each bit n 0..7, 1 = DIMM n is ECC capable.*/
	u16 dimm_ecc_present;
	/* For each bit n 0..7, 1 = DIMM n is ADR/CMD Parity capable.*/
	u16 dimm_parity_present;
	/* For each bit n 0..7, 1 = DIMM n contains x4 data devices.*/
	u16 dimm_x4_present;
	/* For each bit n 0..7, 1 = DIMM n contains x8 data devices.*/
	u16 dimm_x8_present;
	/* For each bit n 0..7, 1 = DIMM n contains x16 data devices.*/
	u16 dimm_x16_present;
	/* For each bit n 0..7, 1 = DIMM n contains 1K page devices.*/
	u16 dimm_1k_page;
	/* Number of devices loading MAA bus*/
	/* Number of devices loading MAB bus*/
	u8 ma_load[2];
	/* Number of DIMMs loading CH A*/
	/* Number of DIMMs loading CH B*/
	u8 ma_dimms[2];
	/* Number of ranks loading CH A DATA*/
	/* Number of ranks loading CH B DATA*/
	u8 data_load[2];
	/* Max valid Mfg. Speed of DIMMs
	 * 1 = 200MHz
	 * 2 = 266MHz
	 * 3 = 333MHz
	 * 4 = 400MHz
	 */
	u8 dimm_auto_speed;
	/* Min valid Mfg. CL bitfield
	 * 0 = 2.0
	 * 1 = 3.0
	 * 2 = 4.0
	 * 3 = 5.0
	 * 4 = 6.0
	 */
	u8 dimm_casl;
	/* Minimax trcd*40 (ns) of DIMMs*/
	u16 dimm_trcd;
	/* Minimax trp*40 (ns) of DIMMs*/
	u16 dimm_trp;
	/* Minimax trtp*40 (ns) of DIMMs*/
	u16 dimm_trtp;
	/* Minimax Tras*40 (ns) of DIMMs*/
	u16 dimm_tras;
	/* Minimax Trc*40 (ns) of DIMMs*/
	u16 dimm_trc;
	/* Minimax Twr*40 (ns) of DIMMs*/
	u16 dimm_twr;
	/* Minimax Trrd*40 (ns) of DIMMs*/
	u16 dimm_trrd;
	/* Minimax Twtr*40 (ns) of DIMMs*/
	u16 dimm_twtr;
	/* Bus Speed (to set Controller)
	 * 1 = 200MHz
	 * 2 = 266MHz
	 * 3 = 333MHz
	 * 4 = 400MHz
	 */
	u8 speed;
	/* CAS latency DCT setting
	 * 0 = 2.0
	 * 1 = 3.0
	 * 2 = 4.0
	 * 3 = 5.0
	 * 4 = 6.0
	 */
	u8 cas_latency;
	/* DCT trcd (busclocks) */
	u8 trcd;
	/* DCT trp (busclocks) */
	u8 trp;
	/* DCT trtp (busclocks) */
	u8 trtp;
	/* DCT Tras (busclocks) */
	u8 tras;
	/* DCT Trc (busclocks) */
	u8 trc;
	/* DCT Twr (busclocks) */
	u8 twr;
	/* DCT Trrd (busclocks) */
	u8 trrd;
	/* DCT Twtr (busclocks) */
	u8 twtr;
	/* DCT Logical DIMM0 Trfc
	 * 0 = 75ns (for 256Mb devs)
	 * 1 = 105ns (for 512Mb devs)
	 * 2 = 127.5ns (for 1Gb devs)
	 * 3 = 195ns (for 2Gb devs)
	 * 4 = 327.5ns (for 4Gb devs)
	 */
	/* DCT Logical DIMM1 Trfc (see Trfc0 for format) */
	/* DCT Logical DIMM2 Trfc (see Trfc0 for format) */
	/* DCT Logical DIMM3 Trfc (see Trfc0 for format) */
	u8 trfc[4];
	/* For each bit n 0..7, 1 = Chip-select n is present */
	u16 cs_present;
	/* For each bit n 0..7, 1 = Chip-select n is present but disabled */
	u16 cs_test_fail;
	/* BASE[39:8] (system address) of this Node's DCTs. */
	u32 dct_sys_base;
	/* If not zero, BASE[39:8] (system address) of dram hole for HW remapping.  Dram hole exists on this Node's DCTs. */
	u32 dct_hole_base;
	/* LIMIT[39:8] (system address) of this Node's DCTs */
	u32 dct_sys_limit;
	/* Maximum OEM defined DDR frequency
	 * 200 = 200MHz (DDR400)
	 * 266 = 266MHz (DDR533)
	 * 333 = 333MHz (DDR667)
	 * 400 = 400MHz (DDR800)
	 */
	u16 preset_max_freq;
	/* 1T or 2T CMD mode (slow access mode)
	 * 1 = 1T
	 * 2 = 2T
	 */
	u8 _2t_mode;
	/* DCT TrwtTO (busclocks)*/
	u8 trwt_to;
	/* DCT Twrrd (busclocks)*/
	u8 twrrd;
	/* DCT Twrwr (busclocks)*/
	u8 twrwr;
	/* DCT Trdrd (busclocks)*/
	u8 trdrd;
	/* Output Driver Strength (see BKDG FN2:Offset 9Ch, index 00h*/
	u32 ch_odc_ctl[2];
	/* Address Bus Timing (see BKDG FN2:Offset 9Ch, index 04h*/
	/* Output Driver Strength (see BKDG FN2:Offset 9Ch, index 20h*/
	/* Address Bus Timing (see BKDG FN2:Offset 9Ch, index 24h*/
	u32 ch_addr_tmg[2];
	/* CHA DQS ECC byte like...*/
	u16 ch_ecc_dqs_like[2];
	/* CHA DQS ECC byte scale*/
	u8 ch_ecc_dqs_scale[2];
	/* Reserved*/
	/* CHB DQS ECC byte like...*/
	/* CHB DQS ECC byte scale*/
//	u8 reserved_b_1;

	/*Reserved*/
//	u8 reserved_b_2;
	/* Max Asynchronous Latency (ns)*/
	u8 max_async_lat;
	/* CHA Byte 0 - 7 and Check Write DQS Delay*/
	/* Reserved*/
	/* CHA Byte 0 - 7 and Check Read DQS Delay*/
	/* Reserved*/
	/* CHB Byte 0 - 7 and Check Write DQS Delay*/
	/* Reserved*/
	/* CHB Byte 0 - 7 and Check Read DQS Delay*/
	/* Reserved*/
	u8 ch_b_dqs[2][2][9];
	/* CHA DIMM 0 - 3 Receiver Enable Delay*/
	/* CHB DIMM 0 - 3 Receiver Enable Delay*/
	u8 ch_d_rcvr_dly[2][4];

	/* Ptr on stack to aligned DQS testing pattern*/
	u32 ptr_pattern_buf_a;
	/*Ptr on stack to aligned DQS testing pattern*/
	u32 ptr_pattern_bu
	/* Current Channel (0= CH A, 1 = CH B)*/;
	u8 channel;
	/* Current Byte Lane (0..7)*/
	u8 byte_lane;
	/* Current DQS-DQ training write direction (0 = read, 1 = write)*/
	u8 direction;
	/* Current pattern*/
	u8 pattern;
	/* Current DQS delay value*/
	u8 dqs_delay;
	/* Current Training Errors*/
	u32 train_errors;
	/* RSVD */
//	u8 reserved_b_3;
	/* Time Stamp Counter measurement of AMC, Low dword*/
	u32 amc_tsc_delta_lo;
	/* Time Stamp Counter measurement of AMC, High dword*/
	u32 amc_tsc_delta_hi;
	/* CH A byte lane 0 - 7 minimum filtered window	passing DQS delay value*/
	/* CH A byte lane 0 - 7 maximum filtered window  passing DQS delay value*/
	/* CH B byte lane 0 - 7 minimum filtered window  passing DQS delay value*/
	/* CH B byte lane 0 - 7 maximum filtered window  passing DQS delay value*/
	/* CH A byte lane 0 - 7 minimum filtered window  passing DQS delay value*/
	/* CH A byte lane 0 - 7 maximum filtered window  passing DQS delay value*/
	/* CH B byte lane 0 - 7 minimum filtered window  passing DQS delay value*/
	/* CH B byte lane 0 - 7 maximum filtered window  passing DQS delay value*/
	u8 ch_b_dly[2][2][2][8];
	/* The logical CPUID of the node*/
	u32 logical_cpuid;
	/* Word sized general purpose field for use by host BIOS.  Scratch space.*/
	u16 host_bios_srvc_1;
	/* Dword sized general purpose field for use by host BIOS.  Scratch space.*/
	u32 host_bios_srvc_2;
	/* QuadRank DIMM present?*/
	u16 DimmQRPresent;
	/* Bitmap showing which dimms failed training*/
	u16 dimm_train_fail;
	/* Bitmap showing which chipselects failed training*/
	u16 CSTrainFail;
	/* Bitmap indicating which Dimms have a manufactur's year code <= 2006*/
	u16 DimmYr06;
	/* Bitmap indicating which Dimms have a manufactur's week code <= 24 of 2006 (June)*/
	u16 DimmWk2406;
	/* Bitmap indicating that Dual Rank Dimms are present*/
	u16 DimmDRPresent;
	/* Bitmap indicating that Planar (1) or Stacked (0) Dimms are present.*/
	u16 DimmPlPresent;
	/* Bitmap showing the channel information about failed Chip Selects*/
	/* 0 in any bit field indicates Channel 0*/
	/* 1 in any bit field indicates Channel 1*/
	u16 channel_train_fail;
};

/*===============================================================================
	Local Error Status Codes (DCTStatStruc.err_code)
===============================================================================*/
#define SC_RUNNING_OK		0
/* Running non-optimally*/
#define SC_VARIANCE_ERR		1
/* Not Running*/
#define SC_STOP_ERR		2
/* Fatal Error, MCTB has exited immediately*/
#define SC_FATAL_ERR		3

/*===============================================================================
     Local Error Status (DCTStatStruc.err_status[31:0])
  ===============================================================================*/
#define SB_NO_DIMMS		0
#define SB_DIMM_CHKSUM		1
/* dimm module type(buffer) mismatch*/
#define SB_DIMM_MISMATCH_M	2
/* dimm CL/T mismatch*/
#define SB_DIMM_MISMATCH_T	3
/* dimm organization mismatch (128-bit)*/
#define SB_DIMM_MISMATCH_O	4
/* SPD missing Trc or Trfc info*/
#define SB_NO_TRC_TRFC		5
/* SPD missing byte 23 or 25*/
#define SB_NO_CYC_TIME		6
/* Bank interleave requested but not enabled*/
#define SB_BK_INT_DIS		7
/* Dram ECC requested but not enabled*/
#define SB_DRAM_ECC_DIS		8
/* Online spare requested but not enabled*/
#define SB_SPARE_DIS		9
/* Running in Minimum Mode*/
#define SB_MINIMUM_MODE		10
/* No DQS Receiver Enable pass window found*/
#define SB_NO_RCVR_EN		11
/* DQS Rcvr En pass window CHA to CH B too large*/
#define SB_CH_A2B_RCVR_EN		12
/* DQS Rcvr En pass window too small (far right of dynamic range)*/
#define SB_SMALL_RCVR		13
/* No DQS-DQ passing positions*/
#define SB_NO_DQS_POS		14
/* DQS-DQ passing window too small*/
#define SB_SMALL_DQS		15

/*===============================================================================
	Local Configuration Status (DCTStatStruc.Status[31:0])
===============================================================================*/
/* All DIMMs are Registered*/
#define SB_REGISTERED		0
/* All banks ECC capable*/
#define SB_ECC_DIMMS		1
/* All banks Addr/CMD Parity capable*/
#define SB_PAR_DIMMS		2
/* Jedec ALL slots clock enable diag mode*/
#define SB_DIAG_CLKS		3
/* DCT in 128-bit mode operation*/
#define SB_128_BIT_MODE		4
/* DCT in 64-bit mux'ed mode.*/
#define SB_64_MUXED_MODE		5
/* 2T CMD timing mode is enabled.*/
#define SB_2T_MODE		6
/* Remapping of Node Base on this Node to create a gap.*/
#define SB_SW_NODE_HOLE		7
/* Memory Hole created on this Node using HW remapping.*/
#define SB_HW_HOLE		8



/*===============================================================================
	NVRAM/run-time-configurable Items
===============================================================================*/
/* Platform Configuration */
/* CPU Package Type (2-bits)
 * 0 = NPT L1
 * 1 = NPT M2
 * 2 = NPT S1
 */
#define NV_PACK_TYPE		0
/* Number of Nodes/Sockets (4-bits)*/
#define NV_MAX_NODES		1
/* Number of DIMM slots for the specified Node ID (4-bits)*/
#define NV_MAX_DIMMS		2
/* Maximum platform demonstrated Memclock (10-bits)
 * 200 = 200MHz (DDR400)
 * 266 = 266MHz (DDR533)
 * 333 = 333MHz (DDR667)
 * 400 = 400MHz (DDR800)
 */
#define NV_MAX_MEMCLK		3
/* Bus ECC capable (1-bits)
 * 0 = Platform not capable
 * 1 = Platform is capable
 */
#define NV_ECC_CAP		4
/* Quad Rank DIMM slot type (2-bits)
 * 0 = Normal
 * 1 = R4 (4-Rank Registered DIMMs in AMD server configuration)
 * 2 = S4 (Unbuffered SO-DIMMs)
 */
#define NV_4_RANK_TYPE		5
/* Value to set DcqBypassMax field (See Function 2, Offset 94h, [27:24] of BKDG for field definition).
 * 4 = 4 times bypass (normal for non-UMA systems)
 * 7 = 7 times bypass (normal for UMA systems)
 */
#define NV_BYP_MAX		6
/* Value to set RdWrQByp field (See Function 2, Offset A0h, [3:2] of BKDG for field definition).
 * 2 = 8 times (normal for non-UMA systems)
 * 3 = 16 times (normal for UMA systems)
 */
#define NV_RD_WR_QBYP		7


/* Dram Timing */
/* User Memclock Mode (2-bits)
 * 0 = Auto, no user limit
 * 1 = Auto, user limit provided in NV_MEM_CK_VAL
 * 2 = Manual, user value provided in NV_MEM_CK_VAL
 */
#define NV_MCT_USR_TMG_MODE	10
/* Memory Clock Value (2-bits)
 * 0 = 200MHz
 * 1 = 266MHz
 * 2 = 333MHz
 * 3 = 400MHz
 */
#define NV_MEM_CK_VAL		11

/* Dram Configuration */
/* Dram Bank (chip-select) Interleaving (1-bits)
 * 0 = disable
 * 1 = enable */
#define NV_BANK_INTLV		20
/* Turn on All DIMM clocks (1-bits)
 * 0 = normal
 * 1 = enable all memclocks */
#define NV_ALL_MEM_CLKS		21
/* SPD Check control bitmap (1-bits)
 * 0 = Exit current node init if any DIMM has SPD checksum error
 * 1 = Ignore faulty SPD checksums (Note: DIMM cannot be enabled) */
#define NV_SPDCHK_RESTRT	22
/* DQS Signal Timing Training Control
 * 0 = skip DQS training
 * 1 = perform DQS training */
#define NV_DQS_TRAIN_CTL		23
/* Node Memory Interleaving (1-bits)
 * 0 = disable
 * 1 = enable */
#define NV_NODE_INTLV		24
/* burstLength32 for 64-bit mode (1-bits)
 * 0 = disable (normal)
 * 1 = enable (4 beat burst when width is 64-bits) */
#define NV_BURST_LEN_32		25

/* Dram Power */
/* CKE based power down mode (1-bits)
 * 0 = disable
 * 1 = enable
 */
#define NV_CKE_PDEN		30
/* CKE based power down control (1-bits)
 * 0 = per Channel control
 * 1 = per Chip select control
 */
#define NV_CKE_CTL		31
/* Memclock tri-stating during C3 and Alt VID (1-bits)
 * 0 = disable
 * 1 = enable
 */
#define NV_CLK_HZ_ALT_VID_C3	32

/* Memory Map/Mgt.*/
/* Bottom of 32-bit IO space (8-bits)
 * NV_BOTTOM_IO[7:0]=Addr[31:24]
 */
#define NV_BOTTOM_IO		40
/* Bottom of shared graphics dram (8-bits)
 * NV_BOTTOM_UMA[7:0]=Addr[31:24]
 */
#define NV_BOTTOM_UMA		41
/* Memory Hole Remapping (1-bits)
 * 0 = disable
 * 1 = enable
 */
#define NV_MEM_HOLE		42

/* ECC */
/* Dram ECC enable*/
#define NV_ECC			50
/* ECC MCE enable*/
#define NV_NBECC		52
/* Chip-Kill ECC Mode enable*/
#define NV_CHIP_KILL		53
/* Dram ECC Redirection enable*/
#define NV_ECC_REDIR		54
/* Dram ECC Background Scrubber CTL*/
#define NV_DRAM_BK_SCRUB		55
/* L2 ECC Background Scrubber CTL*/
#define NV_L2_BK_SCRUB		56
/* DCache ECC Background Scrubber CTL*/
#define NV_DC_BK_SCRUB		57
/* Chip Select spare Control bit 0:
 * 0 = disable spare
 * 1 = enable spare
 * Chip Select spare Control bit 1-4:
 * Reserved, must be zero
 */
#define NV_CS_SPARE_CTL		58
/* Parity Enable*/
#define NV_PARITY		60
/* SyncOnUnEccEn control
 * 0 = disable
 * 1 = enable
 */
#define NV_SYNC_ON_UN_ECC_EN	61


/* global function */
u32 node_present(u32 node);
u32 get_nb32n(struct DCTStatStruc *p_dct_stat, u32 addrx);
u32 get_nb32(u32 addr); /* NOTE: extend addr to 32 bit for bus > 0 */

void k8f_interleave_banks(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat);

void mct_init_with_write_to_cs(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat);

void mct_get_ps_cfg(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat);
void get_channel_ps_cfg_0(unsigned int maa_dimms, unsigned int speed, unsigned int maa_load,
		unsigned int DATAAload, unsigned int *addr_tmg_ctl, unsigned int *odc_ctl);
void get_channel_ps_cfg_1(unsigned int maa_dimms, unsigned int speed, unsigned int maa_load,
		unsigned int *addr_tmg_ctl, unsigned int *odc_ctl, unsigned int *val);
void get_channel_ps_cfg_2(unsigned int maa_dimms, unsigned int speed, unsigned int maa_load,
		unsigned int *addr_tmg_ctl, unsigned int *odc_ctl, unsigned int *val);

u8 mct_def_reset(void);

u32 get_rcvr_sys_addr(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 channel,
			u8 receiver, u8 *valid);
u32 get_mct_sys_addr(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 channel,
			u8 chipsel, u8 *valid);
void k8f_train_receiver_en(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a, u8 pass);
void k8f_train_dqs_pos(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
u32 set_upper_fs_base(u32 addr_hi);


void k8f_ecc_init(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);

void amd_mct_init(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);

void k8f_cpu_mem_typing(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);
void k8f_cpu_mem_typing_clear(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a);

void k8f_wait_mem_clr_delay(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat);
unsigned int k8f_calc_final_dqs_rcv_value(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, unsigned int left_rcv_en,
				unsigned int right_rcv_en, unsigned int *valid);

void k8f_get_delta_tcs_part_1(struct DCTStatStruc *p_dct_stat);
void k8f_get_delta_tcs_part_2(struct DCTStatStruc *p_dct_stat);
#endif
