/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/procedures/hwp/lib/p9_pstates_pgpe.h $    */
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
/// @file  p9_pstates_pgpe.h
/// @brief Pstate structures and support routines for PGPE Hcode
///
// *HWP HW Owner        : Rahul Batra <rbatra@us.ibm.com>
// *HWP HW Owner        : Michael Floyd <mfloyd@us.ibm.com>
// *HWP Team            : PM
// *HWP Level           : 1
// *HWP Consumed by     : PGPE:HS


#ifndef __P9_PSTATES_PGPE_H__
#define __P9_PSTATES_PGPE_H__

#include "p9_pstates_common.h"
#include "p9_pstates_cmeqm.h"

/// PstateParmsBlock Magic Number
///
/// This magic number identifies a particular version of the
/// PstateParmsBlock and its substructures.  The version number should be
/// kept up to date as changes are made to the layout or contents of the
/// structure.

#define PSTATE_PARMSBLOCK_MAGIC 0x5053544154453030ull /* PSTATE00 */

#ifndef __ASSEMBLER__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


/// Pad repurpose structure
typedef union
{
    uint32_t value;
    struct
    {
        // Reserve 3 bytes
        uint16_t reserved16;
        uint8_t reserved8;

        // The following is used by PGPE for the WOF algorithm that computes
        // vratio.  The placement here is, frankly, a bit of a hack but is
        // done to allievate cross-platform dependencies by not changing the
        // overall size of the Global Paramter Block structure.  In the future,
        // this field should be moved into the base Global Paramter Block
        // structure.
        uint8_t good_cores_in_sort;
    } fields;
} GPPBOptionsPadUse;

/// Standard options controlling Pstate setup and installation procedures
typedef struct
{

    /// Option flags; See \ref pstate_options
    uint32_t options;

    /// Pad structure to 8 bytes.  Could also be used for other options later.
    uint32_t pad;

} PGPEOptions;

/// UltraTurbo Segment VIDs by Core Count
typedef struct
{

    /// Number of Segment Pstates
    uint8_t     ut_segment_pstates;

    /// Maximum number of core possibly active
    uint8_t     ut_max_cores;

    /// VDD VID modification
    ///      1 core active  = offset 0
    ///      2 cores active = offset 1
    ///         ...
    ///      12 cores active = offset 11
    uint8_t ut_segment_vdd_vid[MAX_UT_PSTATES][NUM_ACTIVE_CORES];

    /// VCS VID modification
    ///      1 core active  = offset 0
    ///      2 cores active = offset 1
    ///         ...
    ///      12 cores active = offset 11
    uint8_t ut_segment_vcs_vid[MAX_UT_PSTATES][NUM_ACTIVE_CORES];

} VIDModificationTable;

/// Workload Optimized Frequency (WOF) Elements
///
/// Structure defining various control elements needed by the WOF algorithm
/// firmware running on the OCC.
///
typedef struct
{

    /// WOF Enablement
    uint8_t wof_enabled;

    /// TDP<>RDP Current Factor
    ///   Value read from ??? VPD
    ///   Defines the scaling factor that converts current (amperage) value from
    ///   the Thermal Design Point to the Regulator Design Point (RDP) as input
    ///   to the Workload Optimization Frequency (WOF) OCC algorithm.
    ///
    ///   This is a ratio value and has a granularity of 0.01 decimal.  Data
    ///   is held in hexidecimal (eg 1.22 is represented as 122 and then converted
    ///   to hex 0x7A).
    uint32_t tdp_rdp_factor;

    /// UltraTurbo Segment VIDs by Core Count
    VIDModificationTable ut_vid_mod;

} WOFElements;

/// VDM/Droop Parameter Block
///
typedef struct
{
    uint8_t  vid_compare_override_mv[VDM_DROOP_OP_POINTS];
    uint8_t  vdm_response;

    // For the following *_enable fields, bits are defined to indicate
    // which of the respective *override* array entries are valid.
    // bit 0: UltraTurbo; bit 1: Turbo; bit 2: Nominal; bit 3: PowSave

    // The respecitve *_enable above indicate which index values are valid
    uint8_t  droop_small_override[VDM_DROOP_OP_POINTS];
    uint8_t  droop_large_override[VDM_DROOP_OP_POINTS];
    uint8_t  droop_extreme_override[VDM_DROOP_OP_POINTS];
    uint8_t  overvolt_override[VDM_DROOP_OP_POINTS];
    uint16_t fmin_override_khz[VDM_DROOP_OP_POINTS];
    uint16_t fmax_override_khz[VDM_DROOP_OP_POINTS];

    /// Pad structure to 8-byte alignment
    /// @todo pad once fully structure is complete.
    // uint8_t pad[1];

} GP_VDMParmBlock;

/// Global Pstate Parameter Block
///
/// The GlobalPstateParameterBlock is an abstraction of a set of voltage/frequency
/// operating points along with hardware limits.  Besides the hardware global
/// Pstate table, the abstract table contains enough extra information to make
/// it the self-contained source for setting up and managing voltage and
/// frequency in either Hardware or Firmware Pstate mode.
///
/// When installed in PMC, Global Pstate table indices are adjusted such that
/// the defined Pstates begin with table entry 0. The table need not be full -
/// the \a pmin and \a entries fields define the minimum and maximum Pstates
/// represented in the table.  However at least 1 entry must be defined to
/// create a legal table.
///
/// Note that Global Pstate table structures to be mapped into PMC hardware
/// must be 1KB-aligned.  This requirement is fullfilled by ensuring that
/// instances of this structure are 1KB-aligned.
typedef struct
{


    /// Magic Number
    uint64_t magic;     // the last byte of this number the structure's version.

    /// Pstate options
    ///
    /// The options are included as part of the GlobalPstateTable so that they
    /// are available to upon PGPE initialization.
    PGPEOptions options;

    /// The frequency associated with Pstate[0] in KHz
    uint32_t reference_frequency_khz;

    /// The frequency step in KHz
    uint32_t frequency_step_khz;

    /// Operating points
    ///
    /// VPD operating points are stored without load-line correction.  Frequencies
    /// are in MHz, voltages are specified in units of 5mV, and currents are
    /// in units of 500mA.
    /// \todo Remove this. RTC: 174743
    VpdOperatingPoint operating_points[NUM_OP_POINTS];

    /// Biases
    ///
    /// Biases applied to the VPD operating points prior to load-line correction
    /// in setting the external voltages.
    /// Values in 0.5%
    VpdBias ext_biases[NUM_OP_POINTS];

    /// Loadlines and Distribution values for the VDD rail
    SysPowerDistParms vdd_sysparm;

    /// Loadlines and Distribution values for the VCS rail
    SysPowerDistParms vcs_sysparm;

    /// Loadlines and Distribution values for the VDN rail
    SysPowerDistParms vdn_sysparm;

    /// The "Safe" Voltage
    ///
    /// A voltage to be used when safe-mode is activated
    uint32_t safe_voltage_mv;

    /// The "Safe" Frequency
    ///
    /// A voltage to be used when safe-mode is activated
    uint32_t safe_frequency_khz;

    /// The exponent of the exponential encoding of Pstate stepping delay
    uint8_t vrm_stepdelay_range;

    /// The significand of the exponential encoding of Pstate stepping delay
    uint8_t vrm_stepdelay_value;

    /// VDM Data
    GP_VDMParmBlock vdm;

    /// The following are needed to generated the Pstate Table to HOMER.

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

    /// Time b/w ext VRM detects write voltage cmd and when voltage begins to move
    uint32_t ext_vrm_transition_start_ns;

    /// Transition rate for an increasing VDD voltage excursion
    uint32_t ext_vrm_transition_rate_inc_uv_per_us;

    /// Transition rate for an decreasing VDD voltage excursion
    uint32_t ext_vrm_transition_rate_dec_uv_per_us;

    /// Delay to account for VDD rail setting
    uint32_t ext_vrm_stabilization_time_us;

    /// External VRM transition step size
    uint32_t ext_vrm_step_size_mv;

    /// Nest frequency in Mhz. This is used by FIT interrupt
    uint32_t nest_frequency_mhz;

    //Maximum performance loss threshold when undervolting(in 0.1%, tenths of percent)
    uint8_t wov_underv_perf_loss_thresh_pct;

    //WOV undervolting increment percentage(in 0.1%, tenths of percent)
    uint8_t wov_underv_step_incr_pct;

    //WOV undervolting decrement percentage(in 0.1%, tenths of percent)
    uint8_t wov_underv_step_decr_pct;

    //WOV undervolting max percentage(in 0.1%, tenths of percent)
    uint8_t wov_underv_max_pct;

    //When undervolting, if this value is non-zero, then voltage will never be set
    //below this value. If it is zero, then the minimum voltage is only bounded by
    //wov_underv_max_pct.
    uint16_t wov_underv_vmin_mv;

    //When overvolting, then voltage will never be set above this value
    uint16_t wov_overv_vmax_mv;

    //WOV overvolting increment percentage(in 0.1%, tenths of percent)
    uint8_t wov_overv_step_incr_pct;

    //WOV overvolting decrement percentage(in 0.1%, tenths of percent)
    uint8_t wov_overv_step_decr_pct;

    //WOV overvolting max percentage(in 0.1%, tenths of percent)
    uint8_t wov_overv_max_pct;

    uint8_t pad;

    //Determine how often to call the wov algorithm with respect
    //to PGPE FIT ticks
    uint32_t wov_sample_125us;

    //Maximum performance loss(in 0.1%, tenths of percent). We should never be at
    //this level, but we check using this value inside PGPE to make sure that this
    //is reported if it ever happens
    uint32_t wov_max_droop_pct;

    uint32_t pad1;

    /// All operating points
    VpdOperatingPoint operating_points_set[NUM_VPD_PTS_SET][NUM_OP_POINTS];

    //DPLL pstate 0 value
    uint32_t dpll_pstate0_value;

    /// Precalculated Pstate-Voltage Slopes
    uint16_t PStateVSlopes[NUM_VPD_PTS_SET][VPD_NUM_SLOPES_REGION];

    /// Precalculated Voltage-Pstates Slopes
    uint16_t VPStateSlopes[NUM_VPD_PTS_SET][VPD_NUM_SLOPES_REGION];

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

    uint8_t pad2[2];

    //AvsBusTopology
    AvsBusTopology_t avs_bus_topology;

    // @todo DPLL Droop Settings.  These need communication to SGPE for STOP

} __attribute__((packed, aligned(1024))) GlobalPstateParmBlock;


#ifdef __cplusplus
} // end extern C
#endif
#endif    /* __ASSEMBLER__ */
#endif    /* __P9_PSTATES_PGPE_H__ */
