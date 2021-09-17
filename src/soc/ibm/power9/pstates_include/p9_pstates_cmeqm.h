/* SPDX-License-Identifier: Apache-2.0 */

/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/procedures/hwp/lib/p9_pstates_cmeqm.h $   */
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
/// @file  p9_pstates_cmeqm.h
/// @brief Pstate structures and support routines for CME Hcode
///
// *HWP HW Owner        : Rahul Batra <rbatra@us.ibm.com>
// *HWP HW Owner        : Michael Floyd <mfloyd@us.ibm.com>
// *HWP Team            : PM
// *HWP Level           : 1
// *HWP Consumed by     : CME:PGPE

#ifndef __P9_PSTATES_CME_H__
#define __P9_PSTATES_CME_H__

#include "p9_pstates_common.h"
//#include <p9_hcd_memmap_base.H>


/// @}

#ifndef __ASSEMBLER__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/// LocalParmsBlock Magic Number
///
/// This magic number identifies a particular version of the
/// PstateParmsBlock and its substructures.  The version number should be
/// kept up to date as changes are made to the layout or contents of the
/// structure.

#define LOCAL_PARMSBLOCK_MAGIC 0x434d455050423030ull /* CMEPPB00 */

/// Quad Manager Flags
///

typedef union
{
    uint16_t value;
    struct
    {
#ifdef _BIG_ENDIAN
        uint16_t    resclk_enable               : 1;
        uint16_t    ivrm_enable                 : 1;
        uint16_t    vdm_enable                  : 1;
        uint16_t    wof_enable                  : 1;
        uint16_t    dpll_dynamic_fmax_enable    : 1;
        uint16_t    dpll_dynamic_fmin_enable    : 1;
        uint16_t    dpll_droop_protect_enable   : 1;
        uint16_t    reserved                    : 9;
#else
        uint16_t    reserved                    : 9;
        uint16_t    dpll_droop_protect_enable   : 1;
        uint16_t    dpll_dynamic_fmin_enable    : 1;
        uint16_t    dpll_dynamic_fmax_enable    : 1;
        uint16_t    wof_enable                  : 1;
        uint16_t    vdm_enable                  : 1;
        uint16_t    ivrm_enable                 : 1;
        uint16_t    resclk_enable               : 1;
#endif // _BIG_ENDIAN
    } fields;

} QuadManagerFlags;

/// Resonant Clock Stepping Entry
///
typedef union
{
    uint16_t value;
    struct
    {
#ifdef _BIG_ENDIAN
        uint16_t    sector_buffer   : 4;
        uint16_t    spare1          : 1;
        uint16_t    pulse_enable    : 1;
        uint16_t    pulse_mode      : 2;
        uint16_t    resonant_switch : 4;
        uint16_t    spare4          : 4;
#else
        uint16_t    spare4          : 4;
        uint16_t    resonant_switch : 4;
        uint16_t    pulse_mode      : 2;
        uint16_t    pulse_enable    : 1;
        uint16_t    spare1          : 1;
        uint16_t    sector_buffer   : 4;
#endif // _BIG_ENDIAN
    } fields;

} ResonantClockingStepEntry;

#define RESCLK_FREQ_REGIONS 8
#define RESCLK_STEPS        64
#define RESCLK_L3_STEPS     4

typedef struct ResonantClockControl
{
    uint8_t resclk_freq[RESCLK_FREQ_REGIONS];    // Lower frequency of Resclk Regions

    uint8_t resclk_index[RESCLK_FREQ_REGIONS];   // Index into value array for the
    // respective Resclk Region

    /// Array containing the transition steps
    ResonantClockingStepEntry steparray[RESCLK_STEPS];

    /// Delay between steps (in nanoseconds)
    /// Maximum delay: 65.536us
    uint16_t    step_delay_ns;

    /// L3 Clock Stepping Array
    uint8_t     l3_steparray[RESCLK_L3_STEPS];

    /// Resonant Clock Voltage Threshold (in millivolts)
    /// This value is used to choose the appropriate L3 clock region setting.
    uint16_t l3_threshold_mv;

} ResonantClockingSetup;

// #W data points (version 2)
typedef struct
{
    uint16_t ivdd_tdp_ac_current_10ma;
    uint16_t ivdd_tdp_dc_current_10ma;
    uint8_t  vdm_overvolt_small_thresholds;
    uint8_t  vdm_large_extreme_thresholds;
    uint8_t  vdm_normal_freq_drop;   // N_S and N_L Drop
    uint8_t  vdm_normal_freq_return; // L_S and S_N Return
    uint8_t  vdm_vid_compare_ivid;
    uint8_t  vdm_spare;
} poundw_entry_t;

typedef struct
{
    uint16_t r_package_common;
    uint16_t r_quad;
    uint16_t r_core;
    uint16_t r_quad_header;
    uint16_t r_core_header;
} resistance_entry_t;

typedef struct __attribute__((packed))
{
    uint16_t r_package_common;
    uint16_t r_quad;
    uint16_t r_core;
    uint16_t r_quad_header;
    uint16_t r_core_header;
    uint8_t  r_vdm_cal_version;
    uint8_t  r_avg_min_scale_fact;
    uint16_t r_undervolt_vmin_floor_limit;
    uint8_t  r_min_bin_protect_pc_adder;
    uint8_t  r_min_bin_protect_bin_adder;
    uint8_t  r_undervolt_allowed;
    uint8_t  reserve[10];
}
resistance_entry_per_quad_t;

typedef struct
{
    poundw_entry_t poundw[NUM_OP_POINTS];
    resistance_entry_t resistance_data;
    uint8_t undervolt_tested;
    uint8_t reserved;
    uint64_t reserved1;
    uint8_t reserved2; //This field was added to keep the size of struct same when undervolt_tested field was added
} PoundW_data;

/// VDM/Droop Parameter Block
///
typedef struct
{
    PoundW_data vpd_w_data;
} LP_VDMParmBlock;

typedef struct __attribute__((packed))
{
    uint16_t ivdd_tdp_ac_current_10ma;
    uint16_t ivdd_tdp_dc_current_10ma;
    uint8_t  vdm_overvolt_small_thresholds;
    uint8_t  vdm_large_extreme_thresholds;
    uint8_t  vdm_normal_freq_drop;   // N_S and N_L Drop
    uint8_t  vdm_normal_freq_return; // L_S and S_N Return
    uint8_t  vdm_vid_compare_per_quad[MAX_QUADS_PER_CHIP];
    uint8_t  vdm_cal_state_avg_min_per_quad[MAX_QUADS_PER_CHIP];
    uint16_t vdm_cal_state_vmin;
    uint8_t  vdm_cal_state_avg_core_dts;
    uint16_t vdm_cal_state_avg_core_current;
    uint16_t vdm_spare;
}
poundw_entry_per_quad_t;

typedef struct __attribute__((packed))
{
    poundw_entry_per_quad_t poundw[NUM_OP_POINTS];
    resistance_entry_per_quad_t resistance_data;
}
PoundW_data_per_quad;


typedef struct
{
    PoundW_data_per_quad vpd_w_data;
} LP_VDMParmBlock_PerQuad;

/// The layout of the data created by the Pstate table creation firmware for
/// comsumption by the Pstate GPE.  This data will reside in the Quad
/// Power Management Region (QPMR).
///

/// Standard options controlling Pstate setup procedures

/// System Power Distribution Paramenters
///
/// Parameters set by system design that influence the power distribution
/// for a rail to the processor module.  This values are typically set in the
/// system machine readable workbook and are used in the generation of the
/// Global Pstate Table.  This values are carried in the Pstate SuperStructure
/// for use and/or reference by OCC firmware (eg the WOF algorithm)


/// IVRM Parameter Block
///
/// @todo Major work item.  Largely will seed the CME Quad Manager to perform
/// iVRM voltage calculations

#define IVRM_ARRAY_SIZE 64
typedef struct iVRMInfo
{

    /// Pwidth from 0.03125 to 1.96875 in 1/32 increments at Vin=Vin_Max
    uint8_t strength_lookup[IVRM_ARRAY_SIZE];   // Each entry is a six bit value, right justified

    /// Scaling factor for the Vin_Adder calculation.
    uint8_t vin_multiplier[IVRM_ARRAY_SIZE];     // Each entry is from 0 to 255.

    /// Vin_Max used in Vin_Adder calculation (in millivolts)
    uint16_t    vin_max_mv;

    /// Delay between steps (in nanoseconds)
    /// Maximum delay: 65.536us
    uint16_t    step_delay_ns;

    /// Stabilization delay once target voltage has been reached (in nanoseconds)
    /// Maximum delay: 65.536us
    uint16_t    stablization_delay_ns;

    /// Deadzone (in millivolts)
    /// Maximum: 255mV.  If this value is 0, 50mV is assumed.
    uint8_t    deadzone_mv;

    /// Pad to 8B
    uint8_t    pad;

} IvrmParmBlock;

typedef uint8_t CompareVIDPoints;

/// The layout of the data created by the Pstate table creation firmware for
/// comsumption by the CME Quad Manager.  This data will reside in the Core
/// Power Management Region (CPMR).
///
typedef struct
{

    /// Magic Number
    uint64_t magic;     // the last byte of this number the structure's version.

    // QM Flags
    QuadManagerFlags qmflags;

    /// Operating points
    ///
    /// VPD operating points are stored without load-line correction.  Frequencies
    /// are in MHz, voltages are specified in units of 5mV, and currents are
    /// in units of 500mA.
    VpdOperatingPoint operating_points[NUM_OP_POINTS];

    /// Loadlines and Distribution values for the VDD rail
    SysPowerDistParms vdd_sysparm;

    /// External Biases
    ///
    /// Biases applied to the VPD operating points prior to load-line correction
    /// in setting the external voltages.  This is used to recompute the Vin voltage
    /// based on the Global Actual Pstate .
    /// Values in 0.5%
    VpdBias ext_biases[NUM_OP_POINTS];

    /// Internal Biases
    ///
    /// Biases applied to the VPD operating points that are used for interpolation
    /// in setting the internal voltages (eg Vout to the iVRMs) as part of the
    /// Local Actual Pstate.
    /// Values in 0.5%
    VpdBias int_biases[NUM_OP_POINTS];

    /// IVRM Data
    IvrmParmBlock ivrm;

    /// Resonant Clock Grid Management Setup
    ResonantClockingSetup resclk;

    /// VDM Data
    LP_VDMParmBlock vdm;

    /// DPLL pstate 0 value
    uint32_t dpll_pstate0_value;

    // Biased Compare VID operating points
    CompareVIDPoints vid_point_set[NUM_OP_POINTS];

    // Biased Threshold operation points
    uint8_t threshold_set[NUM_OP_POINTS][NUM_THRESHOLD_POINTS];

    //pstate-volt compare slopes
    int16_t PsVIDCompSlopes[VPD_NUM_SLOPES_REGION];

    //pstate-volt threshold slopes
    int16_t PsVDMThreshSlopes[VPD_NUM_SLOPES_REGION][NUM_THRESHOLD_POINTS];

    //Jump value operating points
    uint8_t jump_value_set[NUM_OP_POINTS][NUM_JUMP_VALUES];

    //Jump-value slopes
    int16_t PsVDMJumpSlopes[VPD_NUM_SLOPES_REGION][NUM_JUMP_VALUES];

} LocalPstateParmBlock;

#ifdef __cplusplus
} // end extern C
#endif
#endif    /* __ASSEMBLER__ */
#endif    /* __P9_PSTATES_CME_H__ */
