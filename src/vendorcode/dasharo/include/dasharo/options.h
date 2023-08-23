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

#endif /* DASHARO_OPTIONS_H */
