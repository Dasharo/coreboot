/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/procedures/hwp/lib/p9_pstates_occ.h $     */
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
/// @file  p9_pstates.h
/// @brief Pstate structures and support routines for OCC product firmware
///
// *HWP HW Owner        : Greg Still <stillgs@us.ibm.com>
// *HWP HW Owner        : Michael Floyd <mfloyd@us.ibm.com>
// *HWP FW Owner        : Martha Broyles <bilpatil@in.ibm.com>
// *HWP Team            : PM
// *HWP Level           : 1
// *HWP Consumed by     : HB:OCC

#ifndef __P9_PSTATES_OCC_H__
#define __P9_PSTATES_OCC_H__

#include "p9_pstates_common.h"
#include "p9_pstates_pgpe.h"

#ifndef __ASSEMBLER__
#ifdef __cplusplus
extern "C" {
#endif

/// PstateParmsBlock Magic Number
///
/// This magic number identifies a particular version of the
/// PstateParmsBlock and its substructures.  The version number should be
/// kept up to date as changes are made to the layout or contents of the
/// structure.

#define OCC_PARMSBLOCK_MAGIC 0x4f43435050423030ull /* OCCPPB00 */

/// IDDQ Reading Type
/// Each entry is 2 bytes. The values are in 6.25mA units; this allow for a
/// maximum value of 409.6A to be represented.
///
typedef uint16_t iddq_entry_t;

/// AvgTemp Reading Type
/// Each entry is 1 byte. The values are in 0.5degC units; this allow for a
/// maximum value of 127degC to be represented.
///
typedef uint8_t avgtemp_entry_t;

/// Iddq Table
///
/// A set of arrays of leakage values (Iddq) collected at various voltage
/// conditions during manufacturing test that will feed into the Workload
/// Optimized Frequency algorithms on the OCC.  These values are not installed
/// in any hardware facilities.
///
typedef struct
{

    /// IDDQ version
    uint8_t     iddq_version;

    /// Good Quads per Sort
    uint8_t     good_quads_per_sort;

    /// Good Normal Cores per Sort
    uint8_t     good_normal_cores_per_sort;

    /// Good Caches per Sort
    uint8_t     good_caches_per_sort;

    /// Good Normal Cores
    uint8_t     good_normal_cores[MAXIMUM_QUADS];

    /// Good Caches
    uint8_t     good_caches[MAXIMUM_QUADS];

    /// RDP to TDP Scaling Factor in 0.01% units
    uint16_t    rdp_to_tdp_scale_factor;

    /// WOF Iddq Margin (aging factor) in 0.01% units
    uint16_t    wof_iddq_margin_factor;

    /// VDD Temperature Scale Factor per 10C in 0.01% units
    uint16_t    vdd_temperature_scale_factor;

    /// VDN Temperature Scale Factor per 10C in 0.01% units
    uint16_t    vdn_temperature_scale_factor;

    /// Spare
    uint8_t     spare[8];

    /// IVDD ALL Good Cores ON; 5mA units
    iddq_entry_t ivdd_all_good_cores_on_caches_on[IDDQ_MEASUREMENTS];

    /// IVDD ALL Cores OFF; 5mA units
    iddq_entry_t ivdd_all_cores_off_caches_off[IDDQ_MEASUREMENTS];

    /// IVDD ALL Good Cores OFF; 5mA units
    iddq_entry_t ivdd_all_good_cores_off_good_caches_on[IDDQ_MEASUREMENTS];

    /// IVDD Quad 0 Good Cores ON, Caches ON; 5mA units
    iddq_entry_t ivdd_quad_good_cores_on_good_caches_on[MAXIMUM_QUADS][IDDQ_MEASUREMENTS];

    /// IVDDN; 5mA units
    iddq_entry_t ivdn[IDDQ_MEASUREMENTS];

    /// IVDD ALL Good Cores ON, Caches ON; 0.5C units
    avgtemp_entry_t avgtemp_all_good_cores_on[IDDQ_MEASUREMENTS];

    /// avgtemp ALL Cores OFF, Caches OFF; 0.5C units
    avgtemp_entry_t avgtemp_all_cores_off_caches_off[IDDQ_MEASUREMENTS];

    /// avgtemp ALL Good Cores OFF, Caches ON; 0.5C units
    avgtemp_entry_t avgtemp_all_good_cores_off[IDDQ_MEASUREMENTS];

    /// avgtemp Quad 0 Good Cores ON, Caches ON; 0.5C units
    avgtemp_entry_t avgtemp_quad_good_cores_on[MAXIMUM_QUADS][IDDQ_MEASUREMENTS];

    /// avgtempN; 0.5C units
    avgtemp_entry_t avgtemp_vdn[IDDQ_MEASUREMENTS];

    /// spare (per MVPD documentation
    ///
    /// NOTE:  The MVPD documentation defines 43 spare bytes to lead to a 255B structure.  However,
    /// some consuming code already assumed a 250B structure and the correction of this size was disruptive.
    /// This is not a problem until the IQ keyword actually defines these bytes at which time a keyword
    ///  version update will be need.  Thus, this structure will remain at 250B.
    uint8_t spare_1[38];
} IddqTable;



/// The layout of the data created by the Pstate table creation firmware for
/// comsumption by the OCC firmware.  This data will reside in the Quad
/// Power Management Region (QPMR).
///
/// This structure is aligned to 128B to allow for easy downloading using the
/// OCC block copy engine
///
typedef struct
{

    /// Magic Number
    uint64_t magic;  // the last byte of this number the structure's version.

    /// Operating points
    ///
    /// VPD operating points are stored without load-line correction.  Frequencies
    /// are in MHz, voltages are specified in units of 5mV, and currents are
    /// in units of 500mA.
    VpdOperatingPoint operating_points[NUM_OP_POINTS];

    /// Loadlines and Distribution values for the VDD rail
    SysPowerDistParms vdd_sysparm;

    /// Loadlines and Distribution values for the VDN rail
    SysPowerDistParms vdn_sysparm;

    /// Loadlines and Distribution values for the VCS rail
    SysPowerDistParms vcs_sysparm;

    /// Iddq Table
    IddqTable iddq;

    /// WOF Controls
    WOFElements wof;

    // Frequency Limits
    uint32_t frequency_min_khz;    // Comes from Safe Mode computation
    uint32_t frequency_max_khz;    // Comes from UltraTurbo #V point after biases
    uint32_t frequency_step_khz;   // Comes from refclk/dpll_divider attributes.

    // Minimum Pstate;  Maximum is always 0.
    uint32_t pstate_min;           // Pstate reflecting frequency_min_khz

    /// Nest frequency in Mhz. This is used by FIT interrupt
    uint32_t nest_frequency_mhz;

    //Nest leakage percentage used to calculate the Core leakage
    uint16_t nest_leakage_percent;

    uint16_t ceff_tdp_vdn;

    // AC tdp vdd turbo
    uint16_t lac_tdp_vdd_turbo_10ma;

    // AC tdp vdd nominal
    uint16_t lac_tdp_vdd_nominal_10ma;

    AvsBusTopology_t avs_bus_topology;

} __attribute__((aligned(128))) OCCPstateParmBlock;

#ifdef __cplusplus
} // end extern C
#endif
#endif    /* __ASSEMBLER__ */
#endif    /* __P9_PSTATES_OCC_H__ */
