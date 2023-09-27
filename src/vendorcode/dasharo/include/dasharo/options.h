/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef DASHARO_OPTIONS_H
#define DASHARO_OPTIONS_H

#include <types.h>

/* Looks up "power_on_after_fail" option and Dasharo/"PowerFailureState"
 * variable.
 *
 * Result (same as MAINBOARD_POWER_STATE_*):
 *  - 0 - stay powered OFF
 *  - 1 - power ON
 *  - 2 - restore previous state
 */
uint8_t dasharo_get_power_on_after_fail(void);

/* Looks up Dasharo/"PCIeResizeableBarsEnabled" variable if resizable PCIe BARs
 * support was enabled at compile-time.
 *
 * Result:
 *  - true  - resizeable PCIe BARs are supported and enabled
 *  - false - resizeable PCIe BARs are disabled or not supported
 */
bool dasharo_resizeable_bars_enabled(void);

/* Looks up Dasharo/"MemoryProfile" variable to know which memory profile to
 * enable.
 *
 * Result (values of FSP_M_CONFIG::SpdProfileSelected):
 *  - 0 - Default SPD Profile
 *  - 1 - Custom Profile
 *  - 2 - XMP Profile 1
 *  - 3 - XMP Profile 2
 *  - 4 - XMP Profile 3
 *  - 5 - XMP User Profile 4
 *  - 6 - XMP User Profile 5
 */
uint8_t dasharo_get_memory_profile(void);

  
void dasharo_stash_memory_profile(void);

  
void dasharo_unstash_memory_profile(void);

#endif /* DASHARO_OPTIONS_H */
