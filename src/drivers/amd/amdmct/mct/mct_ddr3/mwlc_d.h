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
#define TrDimmSelStart 4
#define TrDimmSelEnd 5
#define WrLvTrMode 1
#define TrNibbleSel 2
#define WrLvOdtEn 12
#define WrLvErrStart 0
#define WrLvErrEnd 8
#define SendMrsCmd 26
#define Qoff 12
#define MRS_Level 7
#define MrsAddressStartFam10 0
#define MrsAddressEndFam10 15
#define MrsAddressStartFam15 0
#define MrsAddressEndFam15 17
#define MrsBankStartFam10 16
#define MrsBankEndFam10 18
#define MrsBankStartFam15 18
#define MrsBankEndFam15 20
#define MrsChipSelStartFam10 20
#define MrsChipSelEndFam10 22
#define MrsChipSelStartFam15 21
#define MrsChipSelEndFam15 23
#define ASR 18
#define SRT 19
#define DramTermDynStart 10
#define DramTermDynEnd 11
#define WrtLvTrMode 1
#define TrNibbleSel 2
#define TrDimmSelStart 4
#define TrDimmSelEnd 5
#define WrtLvTrEn 0
#define DrvImpCtrlStart 2
#define DrvImpCtrlEnd 3
#define DramTermNbStart 7
#define DramTermNbEnd 9
#define onDimmMirror 3

typedef struct _sMCTStruct
{
	void (*AgesaDelay)(u32 delayval);	/* IBV defined Delay Function */
} __attribute__((packed, aligned(4))) sMCTStruct;

/* DCT 0 and DCT 1 Data structure */
typedef struct _sDCTStruct
{
	u8 NodeId;			/* Node ID */
	u8 DctTrain;			/* Current DCT being trained */
	u8 CurrDct;			/* Current DCT number (0 or 1) */
	u8 DctCSPresent;		/* Current DCT CS mapping */
	u8 WrDqsGrossDlyBaseOffset;
	int32_t WLSeedGrossDelay[MAX_BYTE_LANES * MAX_LDIMMS];	/* Write Levelization Seed Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	int32_t WLSeedFineDelay[MAX_BYTE_LANES * MAX_LDIMMS];	/* Write Levelization Seed Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	int32_t WLSeedPreGrossDelay[MAX_BYTE_LANES * MAX_LDIMMS];	/* Write Levelization Seed Pre-Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 WLSeedPreGrossPrevNibble[MAX_BYTE_LANES * MAX_LDIMMS];
	u8 WLSeedGrossPrevNibble[MAX_BYTE_LANES * MAX_LDIMMS];
	u8 WLSeedFinePrevNibble[MAX_BYTE_LANES * MAX_LDIMMS];
								/* per byte Lane Per Logical DIMM*/
	u8 WLGrossDelay[MAX_BYTE_LANES * MAX_LDIMMS];		/* Write Levelization Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 WLFineDelay[MAX_BYTE_LANES * MAX_LDIMMS];		/* Write Levelization Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 WLGrossDelayFirstPass[MAX_BYTE_LANES * MAX_LDIMMS];	/* First-Pass Write Levelization Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 WLFineDelayFirstPass[MAX_BYTE_LANES * MAX_LDIMMS];	/* First-Pass Write Levelization Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 WLGrossDelayPrevPass[MAX_BYTE_LANES * MAX_LDIMMS];	/* Previous Pass Write Levelization Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 WLFineDelayPrevPass[MAX_BYTE_LANES * MAX_LDIMMS];	/* Previous Pass Write Levelization Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 WLGrossDelayFinalPass[MAX_BYTE_LANES * MAX_LDIMMS];	/* Final-Pass Write Levelization Gross Delay */
								/* per byte Lane Per Logical DIMM*/
	u8 WLFineDelayFinalPass[MAX_BYTE_LANES * MAX_LDIMMS];	/* Final-Pass Write Levelization Fine Delay */
								/* per byte Lane Per Logical DIMM*/
	int32_t WLCriticalGrossDelayFirstPass;
	int32_t WLCriticalGrossDelayPrevPass;
	int32_t WLCriticalGrossDelayFinalPass;
	u16 WLPrevMemclkFreq[MAX_TOTAL_DIMMS];
	u16 reg_man_1_present;
	u8 DimmPresent[MAX_TOTAL_DIMMS];/* Indicates which DIMMs are present */
					/* from Total Number of DIMMs(per Node)*/
	u8 DimmX8Present[MAX_TOTAL_DIMMS];	/* Which DIMMs x8 devices */
	u8 Status[MAX_STATUS];		/* Status for DCT0 and 1 */
	u8 err_code[MAX_ERRORS];		/* Major Error codes for DCT0 and 1 */
	u8 err_status[MAX_ERRORS];	/* Minor Error codes for DCT0 and 1 */
	u8 DimmValid[MAX_TOTAL_DIMMS];	/* Indicates which DIMMs are valid for */
					/* Total Number of DIMMs(per Node) */
	u8 WLTotalDelay[MAX_BYTE_LANES];/* Write Levelization Total Delay */
					/* per byte lane */
	u8 MaxDimmsInstalled;		/* Max Dimms Installed for current DCT */
	u8 dimm_ranks[MAX_TOTAL_DIMMS];	/* Total Number of Ranks(per Dimm) */
	u64 logical_cpuid;
	u8 WLPass;
} __attribute__((packed, aligned(4))) sDCTStruct;

void set_DCT_ADDR_Bits(sDCTStruct *pDCTData,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high, u32 value);
void AmdMemPCIWriteBits(SBDFO loc, u8 highbit, u8 lowbit, u32 *pValue);
u32 get_Bits(sDCTStruct *pDCTData,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high);
void AmdMemPCIReadBits(SBDFO loc, u8 highbit, u8 lowbit, u32 *pValue);
u32 bitTestSet(u32 csMask,u32 tempD);
u32 bitTestReset(u32 csMask,u32 tempD);
void set_Bits(sDCTStruct *pDCTData,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high, u32 value);
bool bitTest(u32 value, u8 bitLoc);
u32 RttNomNonTargetRegDimm (sMCTStruct *pMCTData, sDCTStruct *pDCTData, u8 dimm, bool wl, u8 MemClkFreq, u8 rank);
u32 RttNomTargetRegDimm (sMCTStruct *pMCTData, sDCTStruct *pDCTData, u8 dimm, bool wl, u8 MemClkFreq, u8 rank);
u32 RttWrRegDimm (sMCTStruct *pMCTData, sDCTStruct *pDCTData, u8 dimm, bool wl, u8 MemClkFreq, u8 rank);
u8 WrLvOdtRegDimm (sMCTStruct *pMCTData, sDCTStruct *pDCTData, u8 dimm);
u32 get_ADD_DCT_Bits(sDCTStruct *pDCTData,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high);
void AmdMemPCIRead(SBDFO loc, u32 *Value);
void AmdMemPCIWrite(SBDFO loc, u32 *Value);

#endif
