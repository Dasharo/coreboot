#include <cpu/power/scom.h>

void istep_18_11(void);
void istep_18_12(void);

#define PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 (38)
#define PERV_TOD_ERROR_MASK_REG_RX_TTYPE_1 (39)
#define PERV_TOD_ERROR_MASK_REG_RX_TTYPE_2 (40)
#define PERV_TOD_ERROR_MASK_REG_RX_TTYPE_3 (41)
#define PERV_TOD_ERROR_MASK_REG_RX_TTYPE_4 (42)
#define PERV_TOD_ERROR_MASK_REG_RX_TTYPE_5 (43)

#define PERV_TOD_ERROR_REG_RX_TTYPE_2 (40)
#define PERV_TOD_ERROR_REG_RX_TTYPE_4 (42)
#define PERV_TOD_ERROR_REG_RX_TTYPE_5 (43)

#define PERV_TOD_FSM_REG_IS_RUNNING (4)
#define PERV_TOD_LOAD_TOD_MOD_REG_FSM_TRIGGER (0)
#define PERV_TOD_START_TOD_REG_FSM_TRIGGER (0)
#define PERV_TOD_TX_TTYPE_2_REG_TRIGGER (0)
#define PERV_TOD_TX_TTYPE_4_REG_TRIGGER (0)
#define PERV_TOD_TX_TTYPE_5_REG_TRIGGER (0)

#define MDMT_TOD_GRID_CYCLE_STAGING_DELAY (6)
#define FREQ_X_MHZ (0x708)
#define TOD_GRID_PS (400)
#define P9A_PERV_ROOT_CTRL8_TP_PLL_CLKIN_SEL9_DC (21)
#define PERV_TOD_M_PATH_CTRL_REG_STEP_CREATE_DUAL_EDGE_DISABLE (4)

// Power Bus Electrical Round Trip Delay Control Register
// Trip Delay Control Register
// [0:5]   WO_1P PB_ELINK_RT_DELAY_CTL_SET:
//         Setting a bit to 1 (auto reset to 0) causes the matching link
//         to attempt to do a round-trip delay calculation.
//         Results end up in the PB_ELINK_DLY_*_REG regs.
// [6:7]   WO_1P Reserved.
// [8:13]  RWX_WCLRPART PB_ELINK_RT_DELAY_CTL_STAT:
//         A write of the reg resets these bits. They get set to 1 when
//         a requested round-trip calculation completes.
#define PU_PB_ELINK_RT_DELAY_CTL_REG (0x05013419)
// DOCUMENTATION IS CLEARLY WRONG HERE
// Processor bus Electrical Link Delay 0123 Register
// [0:3]   RO Constant = 0b0000
// [4:15]  ROX Reserved.
// [16:19] RO Constant = 0b0000
// [20:31] ROX Reserved.
// [32:35] RO Constant = 0b0000
// [36:47] ROX Reserved.
// [48:51] RO Constant = 0b0000
// [52:63] ROX Reserved.
#define PU_PB_ELINK_DLY_0123_REG (0x0501340E)
// Root Control 8 Register
// [0]  RW TP_SS0_PLL_RESET:
// [1]  RW TP_SS0_PLL_BYPASS:
// [2]  RW TP_SS0_PLL_TEST_EN:
// [3]  RW ROOT_CTRL8_3_SPARE_SS_PLL_CONTROL:
// [4]  RW TP_FILT0_PLL_RESET:
// [5]  RW TP_FILT0_PLL_BYPASS:
// [6]  RW TP_FILT0_PLL_TEST_EN:
// [7]  RW SPARE_FILT0_PLL:
// [8]  RW TP_FILT1_PLL_RESET:
// [9]  RW TP_FILT1_PLL_BYPASS:
// [10] RW TP_FILT1_PLL_TEST_EN:
// [11] RW SPARE_FILT1_PLL:
// [12] RW TP_PLL_TEST_EN:
// [13] RW TP_PLL_FORCE_OUT_EN_DC:
// [14] RW ROOT_CTRL8_14_SPARE_PLL:
// [15] RW ROOT_CTRL8_15_SPARE_PLL:
// [16] RW TP_CLK_ASYNC_RESET_DC:
// [17] RW TP_CLK_DIV_BYPASS_EN_DC:
// [18] RW TP_CLK_PDLY_BYPASS1_EN_DC:
// [19] RW TP_CLK_PDLY_BYPASS2_EN_DC:
// [20] RW ROOT_CTRL8_20_SPARE_PLL_CONTROL:
// [21] RW ROOT_CTRL8_21_SPARE_PLL_CONTROL:
// [22] RW ROOT_CTRL8_22_SPARE_PLL_CONTROL:
// [23] RW ROOT_CTRL8_23_SPARE_PLL_CONTROL:
// [24] RW TP_FSI_CLKIN_SEL_DC:
// [25] RW ROOT_CTRL8_25_SPARE_CLKIN_CONTROL:
// [26] RW ROOT_CTRL8_26_SPARE_CLKIN_CONTROL:
// [27] RW ROOT_CTRL8_27_SPARE_CLKIN_CONTROL:
// [28] RW TP_PLL_CLKIN_SEL1_DC:
// [29] RW TP_PLL_CLKIN_SEL2_DC:
// [30] RW TP_PLL_CLKIN_SEL3_DC:
// [31] RW TP_PLL_CLKIN_SEL4_DC:
#define PERV_ROOT_CTRL8_SCOM (0x00050018)
// TOD: TX TTYPE
// [0]    TX_TTYPE_2_TRIGGER: TX TTYPE-2 trigger.
// [1-63] constant = 0
#define PERV_TOD_TX_TTYPE_2_REG (0x00040013)
// TOD: TX TTYPE
// TX TType triggering register 4.
// [0]    TX_TTYPE_4_TRIGGER: TX TTYPE-4 trigger.
// [1-63] constant = 0
#define PERV_TOD_TX_TTYPE_4_REG (0x00040015)
// TOD: TX TTYPE
// [0]    TX_TTYPE_5_TRIGGER: TX TTYPE-5 trigger.
// [1-63] constant = 0
#define PERV_TOD_TX_TTYPE_5_REG (0x00040016)
// TOD: Load
// TOD-mod triggering register. This register sets the FSM in NOT_SET state.
// [0]    FSM_LOAD_TOD_MOD_TRIGGER: FSM: LOAD_TOD_MOD trigger.
// [1]    FSM_LOAD_TOD_MOD_SYNC_ENABLE: FSM: LOAD_TOD_MOD sync enable
//        when the FSM is in NOT_SET state
//        0 = No sync sent to the core when the FSM is in NOT_SET state.
//        1 = Sync sent to the core when the FSM is in NOT_SET state.
// [2:63] constant = 0
#define PERV_TOD_LOAD_TOD_MOD_REG (0x00040018)
// TOD: Load Register TOD Incrementer: 60
// Bit TOD and 4-bit WOF on read: Returns all 0s when the TOD is not running.
// On write: go to wait for sync state when data bit 6) = ‘0’ (load TOD).
// Otherwise, go to stopped state (load TOD data63).
// [0-59]  LOAD_TOD_VALUE: Internal path: load TOD value.
// [60-63] WOF: who's-on-first (WOF) incrementer.
#define PERV_TOD_LOAD_TOD_REG (0x00040021)
// TOD: Start TOD Triggering Register
// Goes to running state when data bit [02] = ‘0’.
// Otherwise, go to wait for sync state.
// [0]    FSM_START_TOD_TRIGGER: FSM: Start TOD trigger.
// [1]    REG_0X22_SPARE_01: Spares.
// [2]    FSM_START_TOD_DATA02: FSM: Start TOD data 02 trigger.
// [3-7]  REG_0X22_SPARE_03_07: Spares.
// [8-63] constant = 0
#define PERV_TOD_START_TOD_REG (0x00040022)
// TOD: FSM Register
// [0:3]  RWX I_PATH_FSM_STATE: Internal path.
//        TOD FSM state (TOD is running in the following states:
//        x‘02’, x‘0A’, x‘0E’). 0000 = Error.
// [4]    ROX TOD_IS_RUNNING: TOD running indicator.
// [5:7]  RW REG_0X24_SPARE_05_07: Spares.
// [8:63] RO constant = 0
#define PERV_TOD_FSM_REG (0x00040024)
// TOD: Error and Interrupt Register
// [0] RWX REG_0X00_DATA_PARITY_ERROR: Error: master path control register (0x00): Data parity error.
// [1] RWX M_PATH_0_PARITY_ERROR: Error: master path-0: parity error.
// [2] RWX M_PATH_1_PARITY_ERROR: Error: master path-1: parity error.
// [3] RWX REG_0X01_DATA_PARITY_ERROR: Error: port-0 primary configuration register (0x01): data parity error.
// [4] RWX REG_0X02_DATA_PARITY_ERROR: Error: port-1 primary configuration register (0x02): data parity error.
// [5] RWX REG_0X03_DATA_PARITY_ERROR: Error: port-0 secondary configuration register (0x03): data parity error.
// [6] RWX REG_0X04_DATA_PARITY_ERROR: Error: port-1 secondary configuration register (0x04): data parity error.
// [7] RWX REG_0X05_DATA_PARITY_ERROR: Error: slave path control register (0x05): data parity error.
// [8] RWX REG_0X06_DATA_PARITY_ERROR: Error: internal path control register (0x06): data parity error.
// [9] RWX REG_0X07_DATA_PARITY_ERROR: Error: primary/secondary master/slave control register (0x07): data parity error.
// [10] RWX S_PATH_0_PARITY_ERROR: Error: slave path-0: parity error.
// [11] RWX REG_0X08_DATA_PARITY_ERROR: Error: primary/secondary master/slave status register (0x07): data parity error.
// [12] RWX REG_0X09_DATA_PARITY_ERROR: Error: master path status register (0x09): data parity error.
// [13] RWX REG_0X0A_DATA_PARITY_ERROR: Error: slave path status register (0x0a): data parity error.
// [14] RWX M_PATH_0_STEP_CHECK_ERROR: Error: master path-0: step check error.
// [15] RWX M_PATH_1_STEP_CHECK_ERROR: Error: master path-1: step check error.
// [16] RWX S_PATH_0_STEP_CHECK_ERROR: Error: slave path-0: step check error.
// [17] RWX I_PATH_STEP_CHECK_ERROR: Error: internal path: step check error.
// [18] RWX PSS_HAM: Error: PSS hamming distance.
// [19] RWX REG_0X0B_DATA_PARITY_ERROR: Error: miscellaneous, reset register (0x0b): data parity error.
// [20] RWX S_PATH_1_PARITY_ERROR: Error: slave path-0: parity error.
// [21] RWX S_PATH_1_STEP_CHECK_ERROR: Error: slave path-1: step check error.
// [22] RWX I_PATH_DELAY_STEP_CHECK_PARITY_ERROR: Error: internal path: delay, step check components: parity error.
// [23] RWX REG_0X0C_DATA_PARITY_ERROR: Error.
//      A data parity error of the following registers:
//      Probe Data Select Register (0x0c)
//      Timer Register (0x0d).
// [24] RWX REG_0X11_0X12_0X13_0X14_0X15_0X16_DATA_PARITY_ERROR: Error.
//      A data parity error occurred on one of the following TX-TType trigger registers:
//      TX TType-0 Triggering Register (0x11)
//      TX TType-1 Triggering Register (0x12)
//      TX TType-2 Triggering Register (0x13)
//      TX TType-3 Triggering Register (0x14)
//      TX TType-4 Triggering Register (0x15)
//      TX TType-5 Triggering Register (0x16)
// [25] RWX REG_0X17_0X18_0X21_0X22_DATA_PARITY_ERROR: Error
//      A data parity error on one of the following trigger registers:
//      Move-TOD-to-Timebase Triggering Register (0x17)
//      Load-TOD-Mod Register (0x18)
//      Load Register (0x21)
//      Start-TOD Register (0x22)
// [26] RWX REG_0X1D_0X1E_0X1F_DATA_PARITY_ERROR: Error:
//      A Data parity error on one of the following trace data registers:
//      Trace data set-1 register (0x1d)
//      Trace data set-2 register (0x1e)
//      Trace data set-3 register (0x1f)
// [27] RWX REG_0X20_DATA_PARITY_ERROR: Error: time value register (0x20): data parity error.
// [28] RWX REG_0X23_DATA_PARITY_ERROR: Error: low-order step register (0x23): data parity error.
// [29] RWX REG_0X24_DATA_PARITY_ERROR: Error: FSM register (0x24): data parity error.
// [30] RWX REG_0X29_DATA_PARITY_ERROR: Error: RX-TType control register: data parity error.
// [31] RWX REG_0X30_0X31_0X32_0X33_DATA_PARITY_ERROR: Error
//      A data parity error occurred on one of the following error handling registers:
//      Error register (0x30)
//      Error inject register (0x31)
//      Error mask register (0x32)
//      Core interrupt mask register (0x33)
// [32] RWX REG_0X10_DATA_PARITY_ERROR: Error: chip control register (0x10): data parity error.
// [33] RWX I_PATH_SYNC_CHECK_ERROR: Error: internal path: SYNC check.
// [34] RWX I_PATH_FSM_STATE_PARITY_ERROR: Error: internal path: FSM state parity error.
// [35] RWX I_PATH_TIME_REG_PARITY_ERROR: Error: internal path. Time register parity error.
// [36] RWX I_PATH_TIME_REG_OVERFLOW: Error: internal path. Time register overflow.
// [37] RWX WOF_LOW_ORDER_STEP_COUNTER_PARITY_ERROR: Error: WOF counter or low-order-step counter: parity error.
// [38] RWX RX_TTYPE_0: Status: received TType-0.
// [39] RWX RX_TTYPE_1: Status: received TType-1.
// [40] RWX RX_TTYPE_2: Status: received TType-2.
// [41] RWX RX_TTYPE_3: Status: received TType-3.
// [42] RWX RX_TTYPE_4: Status: received TType-4.
// [43] RWX RX_TTYPE_5: Status: received TType-5 when FSM is in running state.
// [44] RWX PIB_SLAVE_ADDR_INVALID_ERROR: Error: PIB slave: invalid address.
// [45] RWX PIB_SLAVE_WRITE_INVALID_ERROR: Error: PIB slave: invalid write.
// [46] RWX PIB_SLAVE_READ_INVALID_ERROR: Error: PIB slave: invalid read.
// [47] RWX PIB_SLAVE_ADDR_PARITY_ERROR: Error: PIB slave: Address parity error.
// [48] RWX PIB_SLAVE_DATA_PARITY_ERROR: Error: PIB slave: data parity error.
// [49] RWX REG_0X27_DATA_PARITY_ERROR: Error: TType control register(0x27): data parity error.
// [50:52] RWX PIB_MASTER_RSP_INFO_ERROR: Error: PIB master: response infoerror info = 000: no error info > 000: error
// [53] RWX RX_TTYPE_INVALID_ERROR: Error: received invalid TType ROM register 0x21.
// [54] RWX RX_TTYPE_4_DATA_PARITY_ERROR: Error: data parity error on received TType-4 from register 0x21.
// [55] RWX PIB_MASTER_REQUEST_ERROR: Error: PIB master; request while busy error.
// [56] RWX PIB_RESET_DURING_PIB_ACCESS_ERROR: Error: PIB reset received during a PIB master or PIB slave operation.
// [57] RWX EXTERNAL_XSTOP_ERROR: Error: TOD received an external checkstop.
// [58] RWX SPARE_ERROR_58: Error: spare.
// [59] RWX SPARE_ERROR_59: Error: spare.
// [60] RWX SPARE_ERROR_60: Error: spare.
// [61] RWX SPARE_ERROR_61: Error: spare.
// [62] RWX OSCSWITCH_INTERRUPT: Error: Interrupt from the oscillator switch.
// [63] RWX SPARE_ERROR_63: Error: spare.
#define PERV_TOD_ERROR_REG (0x00040030)
// TOD: Error Mask Register Mask: Error Reporting Component (C_ERR_RPT)
// TOD: Error mask register mask of the error reporting component (c_err_rpt)
// [0] RWX REG_0X00_DATA_PARITY_ERROR_MASK: Error mask: master path control register (0x00): data parity error.
// [1] RWX M_PATH_0_PARITY_ERROR_MASK: Error mask: master path-0: parity error.
// [2] RWX M_PATH_1_PARITY_ERROR_MASK: Error mask: master path-1: parity error.
// [3] RWX REG_0X01_DATA_PARITY_ERROR_MASK: Error mask: port-0 primary configuration register (0x01): data parity error.
// [4] RWX REG_0X02_DATA_PARITY_ERROR_MASK: Error mask: port-1 primary configuration register (0x02): data parity error.
// [5] RWX REG_0X03_DATA_PARITY_ERROR_MASK: Error mask: port-0 secondary configuration register (0x03): data parity error.
// [6] RWX REG_0X04_DATA_PARITY_ERROR_MASK: Error mask: port-1 secondary configuration register (0x04): data parity error.
// [7] RWX REG_0X05_DATA_PARITY_ERROR_MASK: Error mask: slave path control register (0x05): data parity error.
// [8] RWX REG_0X06_DATA_PARITY_ERROR_MASK: Error mask: internal path control register (0x06): data parity error.
// [9] RWX REG_0X07_DATA_PARITY_ERROR_MASK: Error mask: primary/secondary master/slave control register (0x07): data parity error.
// [10] RWX S_PATH_0_PARITY_ERROR_MASK: Error mask: slave path-0: parity error.
// [11] RWX REG_0X08_DATA_PARITY_ERROR_MASK: Error mask: primary/secondary master/slave status register (0x07): data parity error.
// [12] RWX REG_0X09_DATA_PARITY_ERROR_MASK: Error mask: master path status register (0x09): data parity error.
// [13] RWX REG_0X0A_DATA_PARITY_ERROR_MASK: Error mask: slave path status register (0x0a): data parity error.
// [14] RWX M_PATH_0_STEP_CHECK_ERROR_MASK: Error mask: master path-0: step check error.
// [15] RWX M_PATH_1_STEP_CHECK_ERROR_MASK: Error mask: master path-1: step check error.
// [16] RWX S_PATH_0_STEP_CHECK_ERROR_MASK: Error mask: slave path-0: step check error.
// [17] RWX I_PATH_STEP_CHECK_ERROR_MASK: Error mask: internal path: Step check error.
// [18] RWX PSS_HAM_MASK: Error mask: PSS hamming distance.
// [19] RWX REG_0X0B_DATA_PARITY_ERROR_MASK: Error mask: miscellaneous, reset register (0x0b): data parity error.
// [20] RWX S_PATH_1_PARITY_ERROR_MASK: Error mask: slave path-0: parity error.
// [21] RWX S_PATH_1_STEP_CHECK_ERROR_MASK: Error mask: slave path-1: step check error.
// [22] RWX I_PATH_DELAY_STEP_CHECK_PARITY_ERROR_MASK: Error mask: internal path: delay, step check components: parity error.
// [23] RWX REG_0X0C_DATA_PARITY_ERROR_MASK: Error mask: data parity error of the following registers:
//      Probe data select register (0x0c)
//      Timer register (0x0d)
// [24] RWX REG_0X11_0X12_0X13_0X14_0X15_0X16_DATA_PARITY_ERROR_MASK: Error mask: data parity error on one of the following TX-TType trigger registers:
//      TX TType-0 triggering register (0x11)
//      TX TType-1 triggering register (0x12)
//      TX TType-2 triggering register (0x13)
//      TX TType-3 triggering register (0x14)
//      TX TType-4 triggering register (0x15)
//      TX TType-5 triggering register (0x16)
// [25] RWX REG_0X17_0X18_0X21_0X22_DATA_PARITY_ERROR_MASK: Error mask: data parity error on one of the following trigger registers:
//      move-TOD-to-timebase triggering register (0x17)
//      TX TType-0 triggering register (0x11)load-TOD-mod register (0x18)
//      TX TType-0 triggering register (0x11)load register (0x21)
//      TX TType-0 triggering register (0x11)start-TOD register (0x22)
// [26] RWX REG_0X1D_0X1E_0X1F_DATA_PARITY_ERROR_MASK: Error mask: data parity error on one of the following trace data registers:
//      TX TType-0 triggering register (0x11)trace data set-1 register (0x1d)
//      TX TType-0 triggering register (0x11)trace data set-2 register (0x1e)
//      TX TType-0 triggering register (0x11)trace data set-3 register (0x1f).
// [27] RWX REG_0X20_DATA_PARITY_ERROR_MASK: Error mask: time value register (0x20): data parity error.
// [28] RWX REG_0X23_DATA_PARITY_ERROR_MASK: Error mask: low-order step register (0x23): data parity error.
// [29] RWX REG_0X24_DATA_PARITY_ERROR_MASK: Error mask: FSM register (0x24): data parity error.
// [30] RWX REG_0X29_DATA_PARITY_ERROR_MASK: Error mask: RX-TType control register: data parity error.
// [31] RWX REG_0X30_0X31_0X32_0X33_DATA_PARITY_ERROR_MASK: Error mask: data parity error on one of the following error handling registers:
//      Error register (0x30)
//      Error inject register (0x31)
//      Error mask register (0x32)
//      Error interrupt mask register (0x33)
// [32] RWX REG_0X10_DATA_PARITY_ERROR_MASK: Error mask: chip control register (0x10): data parity error.
// [33] RWX I_PATH_SYNC_CHECK_ERROR_MASK: Error mask: internal path: SYNC check.
// [34] RWX I_PATH_FSM_STATE_PARITY_ERROR_MASK: Error mask: internal path: FSM state parity error.
// [35] RWX I_PATH_TIME_REG_PARITY_ERROR_MASK: Error mask: internal path: time register: parity error.
// [36] RWX I_PATH_TIME_REG_OVERFLOW_MASK: Error mask: internal path: time register overflow.
// [37] RWX WOF_LOW_ORDER_STEP_COUNTER_PARITY_ERROR_MASK: Error mask: WOF counter or Low- Order-Step counter: parity error.
// [38] RWX RX_TTYPE_0_MASK: Error mask: received TType-0.
// [39] RWX RX_TTYPE_1_MASK: Error mask: received TType-1.
// [40] RWX RX_TTYPE_2_MASK: Error mask: received TType-2.
// [41] RWX RX_TTYPE_3_MASK: Error mask: received TType-3.
// [42] RWX RX_TTYPE_4_MASK: Error mask: received TType-4.
// [43] RWX RX_TTYPE_5_MASK: Error mask: received TType-5 when FSM is in running state.
// [44] RWX PIB_SLAVE_ADDR_INVALID_ERROR_MASK: Error mask: PIB slave: invalid address.
// [45] RWX PIB_SLAVE_WRITE_INVALID_ERROR_MASK: Error mask: PIB slave: invalid write.
// [46] RWX PIB_SLAVE_READ_INVALID_ERROR_MASK: Error mask: PIB slave: invalid read.
// [47] RWX PIB_SLAVE_ADDR_PARITY_ERROR_MASK: Error mask: PIB slave: Address parity error.
// [48] RWX PIB_SLAVE_DATA_PARITY_ERROR_MASK: Error mask: PIB slave: data parity error.
// [49] RWX REG_0X27_DATA_PARITY_ERROR_MASK: Error mask: TType control register(0x27): data parity error.
// [50:52] RWX PIB_MASTER_RSP_INFO_ERROR_MASK: Error mask: PIB master: response infoerror.
// [53] RWX RX_TTYPE_INVALID_ERROR_MASK: Error mask: received invalid TType from register 0x21.
// [54] RWX RX_TTYPE_4_DATA_PARITY_ERROR_MASK: Error mask: data parity error on received TType-4 from register 0x21.
// [55] RWX PIB_MASTER_REQUEST_ERROR_MASK: Error mask: PIB master; request while busy error.
// [56] RWX PIB_RESET_DURING_PIB_ACCESS_ERROR_MASK: Error mask: PIB reset received during a PIB master or PIB slave operation.
// [57] RWX EXTERNAL_XSTOP_ERROR_MASK: Error mask: TOD received an external checkstop.
// [58] RWX SPARE_ERROR_MASK_58: Error mask: spare.
// [59] RWX SPARE_ERROR_MASK_59: Error mask: spare.
// [60] RWX SPARE_ERROR_MASK_60: Error mask: spare.
// [61] RWX SPARE_ERROR_MASK_61: Error mask: spare.
// [62] RWX OSCSWITCH_INTERRUPT_MASK: Error mask: Interrupt from the oscillator switch.
// [63] RWX SPARE_ERROR_MASK_63: Error mask: spare.
#define PERV_TOD_ERROR_MASK_REG (0x00040032)
// Master/slave select: master path select: slave path select: step check setup
// [0]     RWX PRI_M_PATH_SELECT: Primary configuration: master path select.
//         0 = Master path-0 is selected.
//         1 = Master path-1 is selected.
// [1]     RWX PRI_M_S_TOD_SELECT: Primary configuration: master-slave TOD select.
//         0 = TOD is slave.
//         1 = TOD is master.
// [2]     RWX PRI_M_S_DRAWER_SELECT: Primary configuration: master-slave drawer select.
//         0 = Drawer is slave.
//         1 = Drawer is master It is just used for TOD internal power gating.
// [3]     RWX PRI_S_PATH_1_STEP_CHECK_ENABLE: Primary configuration: slave path-1: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [4]     RWX PRI_M_PATH_0_STEP_CHECK_ENABLE: Primary configuration: master path-0: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [5]     RWX PRI_M_PATH_1_STEP_CHECK_ENABLE: Primary configuration: master path-1: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [6]     RWX PRI_S_PATH_0_STEP_CHECK_ENABLE: Primary configuration: slave path-0: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [7]     RWX PRI_I_PATH_STEP_CHECK_ENABLE: Primary configuration: internal path: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [8]     RWX SEC_M_PATH_SELECT: Secondary configuration: master path select.
//         0 = Master path-0 is selected.
//         1 = Master path-1 is selected.
// [9]     RWX SEC_M_S_TOD_SELECT: Secondary configuration: master-slave TOD select.
//         0 = TOD is slave.
//         1 = TOD is master.
// [10]    RWX SEC_M_S_DRAWER_SELECT: Secondary configuration: master-slave drawer select.
//         0 = Drawer is slave.
//         1 = Drawer is master. It is used for TOD internal power gating.
// [11]    RWX SEC_S_PATH_1_STEP_CHECK_ENABLE: Secondary configuration: slave path-1: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [12]    RWX SEC_M_PATH_0_STEP_CHECK_ENABLE: Secondary configuration: master path-0: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [13]    RWX SEC_M_PATH_1_STEP_CHECK_ENABLE: Secondary configuration: master path-1: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [14]    RWX SEC_S_PATH_0_STEP_CHECK_ENABLE: Secondary configuration: slave path-0: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [15]    RWX SEC_I_PATH_STEP_CHECK_ENABLE: Secondary configuration: internal path: step check enable.
//         0 = Step check disabled.
//         1 = Step check enabled.
// [16]    RW PSS_SWITCH_SYNC_ERROR_DISABLE: Miscellaneous error sync hold mode.
//         0 = Gating of one sync on topology switch (type-01) to force TOD sync check error except for the backup TOD master.
//         1 = Disable sync-gating.
// [17]    RWX I_PATH_STEP_CHECK_CPS_DEVIATION_X_DISABLE: Internal path: Step check: enlarge CPS deviation: that is, CPS deviation factor = 8.
//         0 = Enabled.
//         1 = Disabled.
// [18]    RW STEP_CHECK_ENABLE_CHICKEN_SWITCH: Type-2: Step check enable: debug switch.
//         0 = New behavior
//         1 = Old behavior (POWER7).
// [19]    RW REG_0X07_SPARE_19: Spares.
// [20]    RW REG_0X07_SPARE_20: Spares.
// [21]    RW MISC_RESYNC_OSC_FROM_TOD: Miscellaneous
//         0 = Disable resynchronization of master OSC sync pulse from TOD synchronization bit.
//         1 = Enable resynchronization of master OSC sync pulse from TOD synchronization bit legacy.
// [22:31] RW REG_0X07_SPARE_22_31: Spares.
// [32:63] RO constant = 0
#define PERV_TOD_PSS_MSS_CTRL_REG (0x00040007)
// Control register 1 for the secondary configuration distribution port.
// [0:2]   RW SEC_PORT_1_RX_SELECT: Distribution: secondary configuration: port-1 RX select.
// [3]     RW REG_0X04_SPARE_03: Spares.
// [4:5]   RWX SEC_X0_PORT_1_TX_SELECT: Distribution: secondary configuration: X0 port-1 TX select.
// [6:7]   RWX SEC_X1_PORT_1_TX_SELECT: Distribution: secondary configuration: X1 port-1 TX select.
// [8:9]   RWX SEC_X2_PORT_1_TX_SELECT: Distribution: secondary configuration: X2 port-1 TX select.
// [10:11] RWX SEC_X3_PORT_1_TX_SELECT: Distribution: secondary configuration: X3 port-1 TX select.
// [12:13] RWX SEC_X4_PORT_1_TX_SELECT: Distribution: secondary configuration: X4 port-1 TX select.
// [14:15] RWX SEC_X5_PORT_1_TX_SELECT: Distribution: secondary configuration: X5 port-1 TX select.
// [16:17] RWX SEC_X6_PORT_1_TX_SELECT: Distribution: secondary configuration: X6 port-1 TX select.
// [18:19] RWX SEC_X7_PORT_1_TX_SELECT: Distribution: secondary configuration: X7 port-1 TX select.
// [20]    RW SEC_X0_PORT_1_TX_ENABLE: Distribution: secondary configuration: X0 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [21]    RW SEC_X1_PORT_1_TX_ENABLE: Distribution: secondary configuration: X1 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [22]    RW SEC_X2_PORT_1_TX_ENABLE: Distribution: secondary configuration: X2 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [23]    RW SEC_X3_PORT_1_TX_ENABLE: Distribution: secondary configuration: X3 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [24]    RW SEC_X4_PORT_1_TX_ENABLE: Distribution: secondary configuration: X4 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [25]    RW SEC_X5_PORT_1_TX_ENABLE: Distribution: secondary configuration: X5 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [26]    RW SEC_X6_PORT_1_TX_ENABLE: Distribution: secondary configuration: X6 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [27]    RW SEC_X7_PORT_1_TX_ENABLE: Distribution: secondary configuration: X7 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [28:31] RW REG_0X04_SPARE_28_31: Spares.
// [32:63] RO constant = 0
#define PERV_TOD_SEC_PORT_1_CTRL_REG (0x00040004)
// Control register 1 for the primary configuration distribution port.
// [0:2]   RW PRI_PORT_1_RX_SELECT: Distribution: primary configuration: port-1 RX select.
// [3]     RW REG_0X02_SPARE_03: Spares.
// [4:5]   RWX PRI_X0_PORT_1_TX_SELECT: Distribution: primary configuration: X0 port-1 TX select.
// [6:7]   RWX PRI_X1_PORT_1_TX_SELECT: Distribution: primary configuration: X1 port-1 TX select.
// [8:9]   RWX PRI_X2_PORT_1_TX_SELECT: Distribution: primary configuration: X2 port-1 TX select.
// [10:11] RWX PRI_X3_PORT_1_TX_SELECT: Distribution: primary configuration: X3 port-1 TX select.
// [12:13] RWX PRI_X4_PORT_1_TX_SELECT: Distribution: primary configuration: X4 port-1 TX select.
// [14:15] RWX PRI_X5_PORT_1_TX_SELECT: Distribution: primary configuration: X5 port-1 TX select.
// [16:17] RWX PRI_X6_PORT_1_TX_SELECT: Distribution: primary configuration: X6 port-1 TX select.
// [18:19] RWX PRI_X7_PORT_1_TX_SELECT: Distribution: primary configuration: X7 port-1 TX select.
// [20]    RW PRI_X0_PORT_1_TX_ENABLE: Distribution: primary configuration: X0 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [21]    RW PRI_X1_PORT_1_TX_ENABLE: Distribution: primary configuration: X1 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [22]    RW PRI_X2_PORT_1_TX_ENABLE: Distribution: primary configuration: X2 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [23]    RW PRI_X3_PORT_1_TX_ENABLE: Distribution: primary configuration: X3 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [24]    RW PRI_X4_PORT_1_TX_ENABLE: Distribution: primary configuration: X4 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [25]    RW PRI_X5_PORT_1_TX_ENABLE: Distribution: primary configuration: X5 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [26]    RW PRI_X6_PORT_1_TX_ENABLE: Distribution: primary configuration: X6 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [27]    RW PRI_X7_PORT_1_TX_ENABLE: Distribution: primary configuration: X7 port-1 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [28:31] RW REG_0X02_SPARE_28_31: Spares.
// [32:63] RO constant = 0b00000000000000000000000000000000
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
// [3]     RW M_PATH_1_STEP_ALIGN_DISABLE: Indicates whether alignment of master path-1 step to master path-0 step is active.
//         0 = Alignment of master path-1 step to master path-0 step is active.
//         1 = Alignment of master path-1 step to master path-0 step is not active.
// [4]     RW M_PATH_STEP_CREATE_DUAL_EDGE_DISABLE: For master path-01 step creation,
//         determines whether both edges or only the rising edge of the oscillator is sampled.
//         0 = Sample both edges of the oscillator.
//         1 = Sample only the rising edge of the oscillator.
// [5:7]   RW M_PATH_SYNC_CREATE_SPS_SELECT: Master path: sync create: steps per sync (SPS) select: number of STEP pulses per SYNC pulse.
// [8:11]  RW M_PATH_0_STEP_CHECK_CPS_DEVIATION: Master path-0: step check: CPS deviation.
// [12]    RW M_PATH_0_STEP_CHECK_CONSTANT_CPS_ENABLE: Determines whether measured CPS or constant
//         CPS is used for step check CPS deviation for master path 0.
//         0 = Measured CPS is used for the step check CPS deviation
//         1 = Constant CPS is used for the step check CPS deviation
// [13:15] RW M_PATH_0_STEP_CHECK_VALIDITY_COUNT: Master path-0 step check. Specifies the number of
//         received steps before the step is declared as valid.
// [16:19] RW M_PATH_1_STEP_CHECK_CPS_DEVIATION: The CPS deviation for master path-1 step check.
// [20]    RW M_PATH_1_STEP_CHECK_CONSTANT_CPS_ENABLE: Determines whether measured CPS or constant
//         CPS is used for step check CPS deviation for master path 1.
//         0 = Measured CPS is used for the step check CPS deviation
//         1 = Constant CPS is used for the step check CPS deviation.
// [21:23] RW M_PATH_1_STEP_CHECK_VALIDITY_COUNT: Master path-1: step check: validity count. Defines the
//         number of received steps before the step is declared as valid.
// [24:25] RW M_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR: Master path-01: step check: CPS deviation factor.
// [26]    RW M_PATH_0_LOCAL_STEP_MODE_ENABLE: Master path-0: local step mode: enable.
//         0 = Steps are generated from the 16 MHz oscillator-0.
//         1 = Steps are generated locally using the mesh clock.
// [27]    RW M_PATH_1_LOCAL_STEP_MODE_ENABLE: Master path-1: local step mode: enable.
//         0 = Steps are generated from the 16 MHz oscillator-0.
//         1 = Steps are generated locally using the mesh clock.
// [28]    RW M_PATH_0_STEP_STEER_ENABLE: Master path-0: step steering enable.
//         0 = Steering of master path-0 step is not active.
//         1 = Steering of master path-0 step is active.
// [29]    RW M_PATH_1_STEP_STEER_ENABLE: Master path-1: step steering enable.
//         0 = Steering of master path-1 step is not active.
//         1 = Steering of master path-1 step is active.
// [30:31] RW REG_0X00_SPARE_30_31: Spares.
// [32:63] RO constant = 0
#define PERV_TOD_M_PATH_CTRL_REG (0x00040000)
// TOD: Secondary Configuration Distribution Port Control Register 0
// [0:2]   RW SEC_PORT_0_RX_SELECT: Distribution: secondary configuration: port-0 RX select.
// [3]     RW REG_0X03_SPARE_03: Spares.
// [4:5]   RWX SEC_X0_PORT_0_TX_SELECT: Distribution: secondary configuration: X0 port-0 TX select.
// [6:7]   RWX SEC_X1_PORT_0_TX_SELECT: Distribution: secondary configuration: X1 port-0 TX select.
// [8:9]   RWX SEC_X2_PORT_0_TX_SELECT: Distribution: secondary configuration: X2 port-0 TX select.
// [10:11] RWX SEC_X3_PORT_0_TX_SELECT: Distribution: secondary configuration: X3 port-0 TX select.
// [12:13] RWX SEC_X4_PORT_0_TX_SELECT: Distribution: secondary configuration: X4 port-0 TX select.
// [14:15] RWX SEC_X5_PORT_0_TX_SELECT: Distribution: secondary configuration: X5 port-0 TX select.
// [16:17] RWX SEC_X6_PORT_0_TX_SELECT: Distribution: secondary configuration: X6 port-0 TX select.
// [18:19] RWX SEC_X7_PORT_0_TX_SELECT: Distribution: secondary configuration: X7 port-0 TX select.
// [20]    RW SEC_X0_PORT_0_TX_ENABLE: Distribution: secondary configuration: X0 port-0 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [21]    RW SEC_X1_PORT_0_TX_ENABLE: Distribution: secondary configuration: X1 port-0 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [22]    RW SEC_X2_PORT_0_TX_ENABLE: Distribution: secondary configuration: X2 port-0 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [23]    RW SEC_X3_PORT_0_TX_ENABLE: Distribution: secondary configuration: X3 port-0 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [24]    RW SEC_X4_PORT_0_TX_ENABLE: Distribution: secondary configuration: X4 port-0 TX enable
//         0 = Port configured as receiver
//         1 = Port configured as sender.
// [25]    RW SEC_X5_PORT_0_TX_ENABLE: Distribution: secondary configuration: X5 port-0 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [26]    RW SEC_X6_PORT_0_TX_ENABLE: Distribution: secondary configuration: X6 port-0 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [27]    RW SEC_X7_PORT_0_TX_ENABLE: Distribution: secondary configuration: X7 port-0 TX enable
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [28:31] RW REG_0X03_SPARE_28_31: Spares.
// [32:39] RW SEC_I_PATH_DELAY_VALUE: Internal path: secondary configuration: delay value.
// [40:63] RO constant = 0
#define PERV_TOD_SEC_PORT_0_CTRL_REG (0x00040003)
// TOD: Internal Path Control Register
// [0]     RW I_PATH_DELAY_DISABLE: Internal path: delay disable.
//         0 = Delay enabled.
//         1 = Delay disabled.
// [1]     RW I_PATH_DELAY_ADJUST_DISABLE: Internal path: delay adjust disable.
//         0 = Delay adjust enabled.
//         1 = Delay adjust disabled.
// [2:4]   RW REG_0X06_SPARE_02_04: Spares.
// [5]     RW I_PATH_STEP_CHECK_STEP_SELECT: Internal path: step check: step select.
//         0 = Step from master or slave path is checked.
//         1 = Step sent to the core is checked.
// [6:7]   RWX I_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR: Internal path: step check: CPS deviation factor.
// [8:11]  RW I_PATH_STEP_CHECK_CPS_DEVIATION: Internal path: step check: CPS deviation.
// [12]    RW I_PATH_STEP_CHECK_CONSTANT_CPS_ENABLE: Internal path: step check: constant CPS enable.
//         0 = Measured CPS is used for the step check CPS deviation.
//         1 = Constant CPS is used for the step check CPS deviation.
// [13:15] RW I_PATH_STEP_CHECK_VALIDITY_COUNT: Internal path: step check: validity count.
//         Defines the number of received steps before the step is declared as valid.
// [16:21] RW REG_0X06_SPARE_16_21: Spares.
// [22:31] ROX I_PATH_DELAY_ADJUST_VALUE: Internal path: adjusted delay value.
//         If the adjustment is enable, the value indicates the adjusted delay value otherwise it indicates the raw delay
//         (primary or secondary).
// [32:39] RWX I_PATH_CPS: Internal path: CPS
//         In write mode, the value is used to load the CPS for the constant CPS for the step checker. In read mode
//         the value shows the actual CPS in the internal path.
// [40:63] RO constant = 0
#define PERV_TOD_I_PATH_CTRL_REG (0x00040006)
#define PERV_TOD_CHIP_CTRL_REG (0x00040010)

// Control register 0 for the primary configuration distribution port.
// [0:2]   RW PRI_PORT_0_RX_SELECT: Distribution: primary configuration: port-0 RX select.
// [3]     RW REG_0X01_SPARE_03: Spares.
// [4:5]   RWX PRI_X0_PORT_0_TX_SELECT: Distribution: primary configuration: X0 port-0 TX select.
// [6:7]   RWX PRI_X1_PORT_0_TX_SELECT: Distribution: primary configuration: X1 port-0 TX select.
// [8:9]   RWX PRI_X2_PORT_0_TX_SELECT: Distribution: primary configuration: X2 port-0 TX select.
// [10:11] RWX PRI_X3_PORT_0_TX_SELECT: Distribution: primary configuration: X3 port-0 TX select.
// [12:13] RWX PRI_X4_PORT_0_TX_SELECT: Distribution: primary configuration: X4 port-0 TX select.
// [14:15] RWX PRI_X5_PORT_0_TX_SELECT: Distribution: primary configuration: X5 port-0 TX select.
// [16:17] RWX PRI_X6_PORT_0_TX_SELECT: Distribution: primary configuration: X6 port-0 TX select.
// [18:19] RWX PRI_X7_PORT_0_TX_SELECT: Distribution: primary configuration: X7 port-0 TX select.
// [20]    RW PRI_X0_PORT_0_TX_ENABLE: Distribution: primary configuration: X0 port-0 TX enable.
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [21]    RW PRI_X1_PORT_0_TX_ENABLE: Distribution: primary configuration: X1 port-0 TX enable.
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [22]    RW PRI_X2_PORT_0_TX_ENABLE: Distribution: primary configuration: X2 port-0 TX enable.
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [23]    RW PRI_X3_PORT_0_TX_ENABLE: Distribution: primary configuration: X3 port-0 TX enable.
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [24]    RW PRI_X4_PORT_0_TX_ENABLE: Distribution: primary configuration: X4 port-0 TX enable.
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [25]    RW PRI_X5_PORT_0_TX_ENABLE: Distribution: primary configuration: X5 port-0 TX enable.
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [26]    RW PRI_X6_PORT_0_TX_ENABLE: Distribution: primary configuration: X6 port-0 TX enable.
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [27]    RW PRI_X7_PORT_0_TX_ENABLE: Distribution: primary configuration: X7 port-0 TX enable.
//         0 = Port configured as receiver.
//         1 = Port configured as sender.
// [28:31] RW REG_0X01_SPARE_28_31: Spares.
// [32:39] RW PRI_I_PATH_DELAY_VALUE: Internal path: primary configuration: delay value.
// [40:63] RO constant = 0
#define PERV_TOD_PRI_PORT_0_CTRL_REG (0x00040001)
// TOD: Slave Path Control Register
// [0]     RW PRI_S_PATH_SELECT: Primary configuration: slave path select.
// [1]     RW REG_0X05_SPARE_01: Spares.
// [2]     RW S_PATH_M_PATH_CPS_ENABLE: Slave path-01: use of master path CPS: enable.
//         0 = Do not use master path CPS.
//         1 = Use master path CPS.
// [3]     RW S_PATH_REMOTE_SYNC_DISABLE: Slave path-01: remote sync: disable.
//         0 = Use Syncs from master TOD. Steps are generated locally.
//         1 = Use steps-syncs from master TOD.
// [4]     RW SEC_S_PATH_SELECT: Secondary configuration: slave path select.
// [5]     RW REG_0X05_SPARE_05: Spares.
// [6:7]   RW S_PATH_STEP_CHECK_CPS_DEVIATION_FACTOR: Slave path-01: step check: CPS deviation factor.
// [8:11]  RW S_PATH_0_STEP_CHECK_CPS_DEVIATI1 = Slave path-0: step check: CPS deviation.
// [12]    RW S_PATH_0_STEP_CHECK_CONSTANT_CPS_ENABLE: Slave path-0: step check: constant CPS enable
//         0 = Measured CPS is used for the step check CPS deviation
//         1 = Constant CPS is used for the step check CPS deviation
// [13:15] RW S_PATH_0_STEP_CHECK_VALIDITY_COUNT: Slave path-0: step check: validity count. Defines the number of received steps before the step is declared as valid is enabled.
// [16:19] RW S_PATH_1_STEP_CHECK_CPS_DEVIATION: Slave path-1: Step check: CPS deviation.
// [20]    RW S_PATH_1_STEP_CHECK_CONSTANT_CPS_ENABLE: Slave path-1: Step check: constant CPS enable
//         0 = Measured CPS is used for the step check CPS deviation.
//         1 = Constant CPS is used for the step check CPS deviation.
// [21:23] RW S_PATH_1_STEP_CHECK_VALIDITY_COUNT: Slave path-1: Step check: validity count. Defines the number of received steps before the step is declared as valid.
// [24]    RW S_PATH_REMOTE_SYNC_ERROR_DISABLE: Slave path-01: Remote sync: Error disable.
//         0 = Enable remote sync error as a step error.
//         1 = Do not enable remote sync error as a step error.
// [25]    RW S_PATH_REMOTE_SYNC_CHECK_M_CPS_DISABLE: Slave path-01: Remote sync: sync-step check: disable use of master path CPS.
//         0 = Use master path CPS.
//         1 = Do not use master path CPS.
// [26:27] RW S_PATH_REMOTE_SYNC_CHECK_CPS_DEVIATION_FACTOR: Slave path-01: remote sync: sync-step check: CPS deviation factor.
// [28:31] RW S_PATH_REMOTE_SYNC_CHECK_CPS_DEVIATION: Slave path-01: remote sync: sync-step check: CPS deviation.
// [32:39] RW S_PATH_REMOTE_SYNC_MISS_COUNT_MAX: Slave path-01: remote sync: maximum of SYNC miss counts: 0 - 255 syncs.
// [40:63] RO constant = 0
#define PERV_TOD_S_PATH_CTRL_REG (0x00040005)
