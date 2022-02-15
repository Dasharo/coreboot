/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_ISTEP18_H
#define CPU_PPC64_ISTEP18_H

#include <cpu/power/scom.h>
#include <stdint.h>

void istep_18_11(uint8_t chips);
void istep_18_12(void);

/* TODO: everything is used internally only, don't define it in public header */
/* TODO: check again if all bits are using PPC_BIT numbering */
/* TODO: shorten names */
#define PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 (38)

#define PERV_TOD_ERROR_REG_RX_TTYPE_0 (38)

#define PERV_TOD_FSM_REG_IS_RUNNING (4)
#define PERV_TOD_LOAD_TOD_MOD_REG_FSM_TRIGGER (0)
#define PERV_TOD_START_TOD_REG_FSM_TRIGGER (0)
#define PERV_TOD_TX_TTYPE_REG_TRIGGER (0)

#define MDMT_TOD_GRID_CYCLE_STAGING_DELAY (6)
#define FREQ_X_MHZ (2000)
#define TOD_GRID_PS (400)

/* FIXME: P9A?! */
#define P9A_PERV_ROOT_CTRL8_TP_PLL_CLKIN_SEL9_DC (21)
#define PERV_TOD_M_PATH_CTRL_REG_STEP_CREATE_DUAL_EDGE_DISABLE (4)

#define PRI_M_S_TOD_SELECT (1)
#define PRI_M_S_DRAWER_SELECT (2)
#define SEC_M_S_TOD_SELECT (9)
#define SEC_M_S_DRAWER_SELECT (10)

#define M_PATH_0_OSC_NOT_VALID (0)
#define M_PATH_1_OSC_NOT_VALID (1)
#define M_PATH_0_STEP_ALIGN_DISABLE (2)

#define M_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR_OFFSET (24)
#define M_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR_LEN (2)
#define M_PATH_0_STEP_CHECK_CPS_DEVIATION_OFFSET (8)
#define M_PATH_0_STEP_CHECK_CPS_DEVIATION_LEN (4)
#define M_PATH_SYNC_CREATE_SPS_SELECT_OFFSET (5)
#define M_PATH_SYNC_CREATE_SPS_SELECT_LEN (3)
#define M_PATH_0_STEP_CHECK_VALIDITY_COUNT_OFFSET (13)
#define M_PATH_0_STEP_CHECK_VALIDITY_COUNT_LEN (3)

#define BUS_DELAY_63 (PPC_BITMASK(52, 63))
#define BUS_DELAY_47 (PPC_BITMASK(36, 47))

#define LOAD_TOD_VALUE_OFFSET (4)
#define WOF_OFFSET (0)

// Power Bus Electrical Round Trip Delay Control Register
// Trip Delay Control Register
// [0:5]   WO_1P PB_ELINK_RT_DELAY_CTL_SET:
//         Setting a bit to 1 (auto reset to 0) causes the matching link
//         to attempt to do a round-trip delay calculation.
//         Results end up in the PB_ELINK_DLY_*_REG regs.
#define PU_PB_ELINK_RT_DELAY_CTL_REG (0x05013419)
// Processor bus Electrical Link Delay 0123 Register
// [36:47] ROX Reserved.
// [52:63] ROX Reserved.
// Note: Documentations describes these bits as reserved however they are used
// to get bus_delay value.
#define PU_PB_ELINK_DLY_0123_REG (0x0501340E)
// Root Control 8 Register
// [21] RW ROOT_CTRL8_21_SPARE_PLL_CONTROL:
#define PERV_ROOT_CTRL8_SCOM (0x00050018)
// TOD: TX TTYPE
// TX TType triggering register.
// [0]    TX_TTYPE_2_TRIGGER: TX TTYPE trigger.
#define PERV_TOD_TX_TTYPE_2_REG (0x00040013)
#define PERV_TOD_TX_TTYPE_4_REG (0x00040015)
#define PERV_TOD_TX_TTYPE_5_REG (0x00040016)
// TOD: Load
// TOD-mod triggering register. This register sets the FSM in NOT_SET state.
// [0]    FSM_LOAD_TOD_MOD_TRIGGER: FSM: LOAD_TOD_MOD trigger.
#define PERV_TOD_LOAD_TOD_MOD_REG (0x00040018)
// TOD: Load Register TOD Incrementer: 60
// Bit TOD and 4-bit WOF on read: Returns all 0s when the TOD is not running.
// On write: go to wait for sync state when data bit 6) = '0' (load TOD).
// Otherwise, go to stopped state (load TOD data63).
// [0-59]  LOAD_TOD_VALUE: Internal path: load TOD value.
// [60-63] WOF: who's-on-first (WOF) incrementer.
#define PERV_TOD_LOAD_TOD_REG (0x00040021)
// TOD: Start TOD Triggering Register
// Goes to running state when data bit [02] = '0'.
// Otherwise, go to wait for sync state.
// [0]    FSM_START_TOD_TRIGGER: FSM: Start TOD trigger.
#define PERV_TOD_START_TOD_REG (0x00040022)
// TOD: FSM Register
// [0:3]  RWX I_PATH_FSM_STATE: Internal path.
//        TOD FSM state (TOD is running in the following states:
//        x'02', x'0A', x'0E'). 0000 = Error.
// [4]    ROX TOD_IS_RUNNING: TOD running indicator.
#define PERV_TOD_FSM_REG (0x00040024)
// TOD: Error and Interrupt Register
// [38] RWX RX_TTYPE_0: Status: received TType-0.
// [39] RWX RX_TTYPE_1: Status: received TType-1.
// [40] RWX RX_TTYPE_2: Status: received TType-2.
// [41] RWX RX_TTYPE_3: Status: received TType-3.
// [42] RWX RX_TTYPE_4: Status: received TType-4.
// [43] RWX RX_TTYPE_5: Status: received TType-5 when FSM is in running state.
#define PERV_TOD_ERROR_REG (0x00040030)
// TOD: Error Mask Register Mask: Error Reporting Component (C_ERR_RPT)
// TOD: Error mask register mask of the error reporting component (c_err_rpt)
// This register holds masks for the same bits
// as in the previous (PERV_TOD_ERROR_REG) register
#define PERV_TOD_ERROR_MASK_REG (0x00040032)
// Master/slave select: master path select: slave path select: step check setup
// [1]     RWX PRI_M_S_TOD_SELECT: Primary configuration: master-slave TOD select.
//         0 = TOD is slave.
//         1 = TOD is master.
// [2]     RWX PRI_M_S_DRAWER_SELECT: Primary configuration: master-slave drawer select.
//         0 = Drawer is slave.
//         1 = Drawer is master It is just used for TOD internal power gating.
// [9]     RWX SEC_M_S_TOD_SELECT: Secondary configuration: master-slave TOD select.
//         0 = TOD is slave.
//         1 = TOD is master.
// [10]    RWX SEC_M_S_DRAWER_SELECT: Secondary configuration: master-slave drawer select.
//         0 = Drawer is slave.
//         1 = Drawer is master. It is used for TOD internal power gating.
#define PERV_TOD_PSS_MSS_CTRL_REG (0x00040007)
// Control register 1 for the secondary configuration distribution port.
#define PERV_TOD_SEC_PORT_1_CTRL_REG (0x00040004)
// Control register 1 for the primary configuration distribution port.
#define PERV_TOD_PRI_PORT_1_CTRL_REG (0x00040002)
// TOD: Setup for Master Paths Control Register
// Used for oscillator validity, step alignment, sync pulse frequency, and step check.
// [0]     RW M_PATH_0_OSC_NOT_VALID: Indicates whether the oscillator attached to master path-0 is not valid.
//         0 = Valid oscillator is attached to master path-0.
//         1 = No valid oscillator is attached to master path-0.
// [1]     RW M_PATH_1_OSC_NOT_VALID: Indicates whether the oscillator attached to master path-1 is not valid.
//         0 = Valid oscillator is attached to master path-1.
//         1 = No valid oscillator is attached to master path-1.
// [2]     RW M_PATH_0_STEP_ALIGN_DISABLE: Master path-0. Indicates alignment of master path-0 step to master path-1 step is active
//         0 = Alignment of master path-0 step to master path-1 step is active.
//         1 = Alignment of master path-0 step to master path-1 Step is not active.
// [5:7]   RW M_PATH_SYNC_CREATE_SPS_SELECT: Master path: sync create: steps per sync (SPS) select: number of STEP pulses per SYNC pulse.
// [8:11]  RW M_PATH_0_STEP_CHECK_CPS_DEVIATION: Master path-0: step check: CPS deviation.
// [13:15] RW M_PATH_0_STEP_CHECK_VALIDITY_COUNT: Master path-0 step check. Specifies the number of
//         received steps before the step is declared as valid.
// [24:25] RW M_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR: Master path-01: step check: CPS deviation factor.
#define PERV_TOD_M_PATH_CTRL_REG (0x00040000)
// TOD: Internal Path Control Register
// [8:11]  RW I_PATH_STEP_CHECK_CPS_DEVIATION: Internal path: step check: CPS deviation.
// [13:15] RW I_PATH_STEP_CHECK_VALIDITY_COUNT: Internal path: step check: validity count.
//         Defines the number of received steps before the step is declared as valid.
// [32:39] RWX I_PATH_CPS: Internal path: CPS
//         In write mode, the value is used to load the CPS for the constant CPS for the step checker. In read mode
//         the value shows the actual CPS in the internal path.
#define PERV_TOD_I_PATH_CTRL_REG (0x00040006)
// TOD: Secondary Configuration Distribution Port Control Register 0
// [32:39] RW SEC_I_PATH_DELAY_VALUE: Internal path: secondary configuration: delay value.
#define PERV_TOD_SEC_PORT_0_CTRL_REG (0x00040003)
// TOD: Primary Configuration Distribution Port Control Register 0
// Same bit purpouse as in PERV_TOD_SEC_PORT_0_CTRL_REG
#define PERV_TOD_PRI_PORT_0_CTRL_REG (0x00040001)
// TOD: Chip Control Register
// [10:15] LOW_ORDER_STEP_VALUE: Low-order step value needed
//         for USE_TB_STEP_SYNC as the programmable
//         cycle counter for creating a step.
#define PERV_TOD_CHIP_CTRL_REG (0x00040010)
// TOD: Slave Path Control Register
// [26:27] RW S_PATH_REMOTE_SYNC_CHECK_CPS_DEVIATION_FACTOR: Slave path-01: remote sync: sync-step check: CPS deviation factor.
// [28:31] RW S_PATH_REMOTE_SYNC_CHECK_CPS_DEVIATION: Slave path-01: remote sync: sync-step check: CPS deviation.
// [32:39] RW S_PATH_REMOTE_SYNC_MISS_COUNT_MAX: Slave path-01: remote sync: maximum of SYNC miss counts: 0 - 255 syncs.
#define PERV_TOD_S_PATH_CTRL_REG (0x00040005)

#endif /* CPU_PPC64_ISTEP18_H */
