/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/procedures/hwp/lib/p9_pstates_common.h $  */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2019                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
/// @file  p9_pstates_common.h
/// @brief Common Pstate definitions
///
// *HWP HW Owner        : Rahul Batra <rbatra@us.ibm.com>
// *HWP HW Owner        : Michael Floyd <mfloyd@us.ibm.com>
// *HWP Team            : PM
// *HWP Level           : 1
// *HWP Consumed by     : PGPE:CME:HB:OCC


#ifndef __P9_PSTATES_COMMON_H__
#define __P9_PSTATES_COMMON_H__

/// The maximum Pstate (knowing the increasing Pstates numbers represent
/// decreasing frequency)
#define PSTATE_MAX 255

/// The minimum Pstate (knowing the increasing Pstates numbers represent
/// decreasing frequency)
#define PSTATE_MIN 0


/// Maximum number of Quads (4 cores plus associated caches)
#define MAXIMUM_QUADS 6

// Constants associated with VRM stepping
// @todo Determine what is needed here (eg Attribute mapping) and if any constants
// are warrented

/// VPD #V Data from keyword (eg VPD order)

#define NUM_JUMP_VALUES 4
#define NUM_THRESHOLD_POINTS 4

// @todo RTC 181607
// This is synchronization work-around to avoid a co-req update between CME Hcode
// and the Pstate Parameter Block.  The CME uses "IDX" while these use "INDEX".
// In the future, these should be common between the two platforms.
//
// As this file is included in both platforms, the definition below can be used
// in the CME Hcode and the "IDX" versions deprecated once this file version
// is included in both platforms.
#ifndef __ASSEMBLER__
typedef enum
{
    VDM_OVERVOLT_INDEX = 0,
    VDM_SMALL_INDEX    = 1,
    VDM_LARGE_INDEX    = 2,
    VDM_XTREME_INDEX   = 3
} VDM_THRESHOLD_INDEX;

typedef enum
{
    VDM_N_S_INDEX = 0,
    VDM_N_L_INDEX = 1,
    VDM_L_S_INDEX = 2,
    VDM_S_N_INDEX = 3
} VDM_JUMP_VALUE_INDEX;
#endif

#define NUM_OP_POINTS      4
#define VPD_PV_POWERSAVE   1
#define VPD_PV_NOMINAL     0
#define VPD_PV_TURBO       2
#define VPD_PV_ULTRA       3
#define VPD_PV_POWERBUS    4

#define VPD_PV_ORDER {VPD_PV_POWERSAVE, VPD_PV_NOMINAL, VPD_PV_TURBO, VPD_PV_ULTRA}
#define VPD_PV_ORDER_STR {"Nominal   ","PowerSave ", "Turbo     ", "UltraTurbo"}
#define VPD_THRESHOLD_ORDER_STR {"Overvolt", "Small", "Large", "Extreme" }

/// VPD #V Operating Points (eg Natural order)
#define POWERSAVE   0
#define NOMINAL     1
#define TURBO       2
#define ULTRA       3
#define POWERBUS    4
#define PV_OP_ORDER {POWERSAVE, NOMINAL, TURBO, ULTRA}
#define PV_OP_ORDER_STR {"PowerSave ", "Nominal   ","Turbo     ", "UltraTurbo"}

#define VPD_PV_CORE_FREQ_MHZ    0
#define VPD_PV_VDD_MV           1
#define VPD_PV_IDD_100MA        2
#define VPD_PV_VCS_MV           3
#define VPD_PV_ICS_100MA        4
#define VPD_PV_PB_FREQ_MHZ      0
#define VPD_PV_VDN_MV           1
#define VPD_PV_IDN_100MA        2

#define VPD_NUM_SLOPES_REGION       3
#define REGION_POWERSAVE_NOMINAL    0
#define REGION_NOMINAL_TURBO        1
#define REGION_TURBO_ULTRA          2
#define VPD_OP_SLOPES_REGION_ORDER {REGION_POWERSAVE_NOMINAL,REGION_NOMINAL_TURBO,REGION_TURBO_ULTRA}
#define VPD_OP_SLOPES_REGION_ORDER_STR {"POWERSAVE_NOMINAL", "NOMINAL_TURBO    ","TURBO_ULTRA      "}

// Different points considered for calculating slopes
#define NUM_VPD_PTS_SET             4
#define VPD_PT_SET_RAW              0
#define VPD_PT_SET_SYSP             1
#define VPD_PT_SET_BIASED           2
#define VPD_PT_SET_BIASED_SYSP      3
#define VPD_PT_SET_ORDER {VPD_PT_SET_RAW, VPD_PT_SET_SYSP, VPD_PT_SET_BIASED, VPD_PT_SET_BIASED_SYSP}
#define VPD_PT_SET_ORDER_STR {"Raw", "SysParam","Biased", "Biased/SysParam"}

#define VID_SLOPE_FP_SHIFT          13 //TODO: Remove this. RTC 174743
#define VID_SLOPE_FP_SHIFT_12       12
#define THRESH_SLOPE_FP_SHIFT       12

// 0 = PowerSave, 1 = Nominal; 2 = Turbo; 3 = UltraTurbo; 4 = Enable
#define VDM_DROOP_OP_POINTS         5


#define PSTATE_LT_PSTATE_MIN 0x00778a03
#define PSTATE_GT_PSTATE_MAX 0x00778a04
#define ACTIVE_QUADS 6

/// IDDQ readings,
#define IDDQ_MEASUREMENTS 6
#define IDDQ_ARRAY_VOLTAGES     { 0.60 ,  0.70 ,  0.80 ,  0.90 ,  1.00 ,  1.10}
#define IDDQ_ARRAY_VOLTAGES_STR {"0.60", "0.70", "0.80", "0.90", "1.00", "1.10"}

/// WOF Items
#define NUM_ACTIVE_CORES 24
#define MAX_UT_PSTATES   64     // Oversized



#ifndef __ASSEMBLER__
#ifdef __cplusplus
extern "C" {
#endif

/// A Pstate type
///
/// Pstates are unsigned but, to avoid bugs, Pstate register fields should
/// always be extracted to a variable of type Pstate.  If the size of Pstate
/// variables ever changes we will have to revisit this convention.
typedef uint8_t Pstate;

/// A DPLL frequency code
///
/// DPLL frequency codes (Fmax and Fmult) are 15 bits
typedef uint16_t DpllCode;

/// An AVS VID code
typedef uint16_t VidAVS;

/// A VPD operating point
///
/// VPD operating points are stored without load-line correction.  Frequencies
/// are in MHz, voltages are specified in units of 1mV, and characterization
/// currents are specified in units of 100mA.
///
typedef struct
{
    uint32_t vdd_mv;
    uint32_t vcs_mv;
    uint32_t idd_100ma;
    uint32_t ics_100ma;
    uint32_t frequency_mhz;
    uint8_t  pstate;        // Pstate of this VpdOperating
    uint8_t  pad[3];        // Alignment padding
} VpdOperatingPoint;

//Defined same as #V vpd points used to validate
typedef struct
{
    uint32_t frequency_mhz;
    uint32_t vdd_mv;
    uint32_t idd_100ma;
    uint32_t vcs_mv;
    uint32_t ics_100ma;
} VpdPoint;
/// VPD Biases.
///
/// Percent bias applied to VPD operating points prior to interolation
///
/// All values on in .5 percent (half percent -> hp)
typedef struct
{

    int8_t vdd_ext_hp;
    int8_t vdd_int_hp;
    int8_t vdn_ext_hp;
    int8_t vcs_ext_hp;
    int8_t frequency_hp;

} VpdBias;

/// System Power Distribution Paramenters
///
/// Parameters set by system design that influence the power distribution
/// for a rail to the processor module.  This values are typically set in the
/// system machine readable workbook and are used in the generation of the
/// Global Pstate Table.  This values are carried in the Pstate SuperStructure
/// for use and/or reference by OCC firmware (eg the WOF algorithm)

typedef struct
{

    /// Loadline
    ///   Impedance (binary microOhms) of the load line from a processor VDD VRM
    ///   to the Processor Module pins.
    uint32_t loadline_uohm;

    /// Distribution Loss
    ///   Impedance (binary in microOhms) of the VDD distribution loss sense point
    ///   to the circuit.
    uint32_t distloss_uohm;

    /// Distribution Offset
    ///   Offset voltage (binary in microvolts) to apply to the rail VRM
    ///   distribution to the processor module.
    uint32_t distoffset_uv;

} SysPowerDistParms;

/// AVSBUS Topology
///
/// AVS Bus and Rail numbers for VDD, VDN, VCS, and VIO
///
typedef struct
{
    uint8_t vdd_avsbus_num;
    uint8_t vdd_avsbus_rail;
    uint8_t vdn_avsbus_num;
    uint8_t vdn_avsbus_rail;
    uint8_t vcs_avsbus_num;
    uint8_t vcs_avsbus_rail;
    uint8_t vio_avsbus_num;
    uint8_t vio_avsbus_rail;
} AvsBusTopology_t;

//
// WOF Voltage, Frequency Ratio Tables
//
//VFRT calculation part
#define SYSTEM_VERSION_FRQUENCY(VFRT)    (1000 + (16.67 * VFRT))
#define SYSTEM_VFRT_VALUE(FREQ)          ((FREQ - 1000)/16.67)


#define HOMER_VFRT_VALUE(FREQ,BSF)       ((BSF - FREQ)/16.67)
#define HOMER_VERSION_FREQUENCY(VFRT,BSF) (BSF - (16.67 * VFRT))


//VFRT Header fields
typedef struct __attribute__((packed)) VFRTHeaderLayout
{
    // VFRT Magic code "VT"
    uint16_t magic_number;

    uint16_t reserved;
    // 0:System type, 1:Homer type (0:3)
    // if version 1: VFRT size is 12 row(voltage) X 11 column(freq) of size uint8_t
    // (4:7)
    // if version 2: VFRT size is 24 row(Voltage) X 5 column (Freq) of size uint8_t
    uint8_t  type_version;
    //Identifies the Vdn assumptions tht went in this VFRT (0:7)
    uint8_t res_vdnId;
    //Identifies the Vdd assumptions tht went in this VFRT (0:7)
    uint8_t VddId_QAId;
    //Identifies the Quad Active assumptions tht went in this VFRT (5:7)
    uint8_t rsvd_QAId;
} VFRTHeaderLayout_t;// WOF Tables Header

typedef struct __attribute__((packed, aligned(128))) WofTablesHeader
{

    /// Magic Number
    ///   Set to ASCII  "WFTH___x" where x is the version of the VFRT structure
    uint32_t magic_number;

    /// Reserved version
    /// version 1 - mode is reserved (0)
    /// version 2 - mode is SET to 1 or 2
    union
    {
        uint32_t reserved_version;
        struct
        {
            unsigned reserved_bits: 20;
            unsigned mode: 4;  /// new to version 2 (1 = Nominal, 2 = Turbo)
            uint8_t  version;
        } PACKED;
    };

    /// VFRT Block Size
    ///    Length, in bytes, of a VFRT
    uint16_t vfrt_block_size;

    /// VFRT block header size
    uint16_t vfrt_block_header_size;

    /// VFRT Data Size
    ///    Length, in bytes, of the data field.
    uint16_t vfrt_data_size;

    /// Quad Active Size
    ///    Total number of Active Quads
    uint8_t quads_active_size;

    /// Core count
    uint8_t core_count;

    /// Ceff Vdn Start
    ///    CeffVdn value represented by index 0 (in 0.01%)
    uint16_t vdn_start;

    /// Ceff Vdn Step
    ///    CeffVdn step value for each CeffVdn index (in 0.01%)
    uint16_t vdn_step;

    /// Ceff Vdn Size
    ///    Number of CeffVdn indexes
    uint16_t vdn_size;

    /// Ceff Vdd Start
    ///    CeffVdd value represented by index 0 (in 0.01%)
    uint16_t vdd_start;

    /// Ceff Vdd Step
    ///    CeffVdd step value for each CeffVdd index (in 0.01%)
    uint16_t vdd_step;

    /// Ceff Vdd Size
    ///    Number of CeffVdd indexes
    uint16_t vdd_size;

    /// Vratio Start
    ///    Vratio value represented by index 0 (in 0.01%)
    uint16_t vratio_start;

    /// Vratio Step
    ///   Vratio step value for each CeffVdd index (in 0.01%)
    uint16_t vratio_step;

    /// Vratio Size
    ///    Number of Vratio indexes
    uint16_t vratio_size;

    /// Fratio Start
    ///    Fratio value represented by index 0 (in 0.01%)
    uint16_t fratio_start;

    /// Fratio Step
    ///   Fratio step value for each CeffVdd index (in 0.01%)
    uint16_t fratio_step;

    /// Fratio Size
    ///    Number of Fratio indexes
    uint16_t fratio_size;

    /// Future usage
    uint16_t Vdn_percent[8];

    /// Socket Power (in Watts) for the WOF Tables
    uint16_t socket_power_w;

    /// Nest Frequency (in MHz) used in building the WOF Tables
    uint16_t nest_frequency_mhz;

    /// Core Sort Power Target Frequency (in MHz) – The #V frequency associated
    /// with the sort power target for this table set. This will be either the
    /// Nominal or Turbo #V frequency
    uint16_t sort_power_freq_mhz;

    /// Regulator Design Point Capacity (in Amps)
    uint16_t rdp_capacity;

    /// Up to 8 ASCII characters to be defined by the Table generation team to
    /// back reference table sources
    char wof_table_source_tag[8];

    /// Up to 16 ASCII characters as a Package designator
    char package_name[16];

    // Padding to 128B is left to the compiler via the following attribute.

} WofTablesHeader_t;


// Data is provided in 1/24ths granularity with adjustments for integer
// representation
#define VFRT_VRATIO_SIZE 24

// 5 steps down from 100% is Fratio_step sizes
#define VFRT_FRATIO_SIZE 5


// HOMER VFRT Layout
typedef struct __attribute__((packed, aligned(256))) HomerVFRTLayout
{
    VFRTHeaderLayout_t vfrtHeader;
    uint8_t vfrt_data[VFRT_FRATIO_SIZE][VFRT_VRATIO_SIZE];
    uint8_t padding[128];
} HomerVFRTLayout_t;


#ifdef __cplusplus
} // end extern C
#endif
#endif    /* __ASSEMBLER__ */
#endif    /* __P9_PSTATES_COMMON_H__ */
