/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef DASHARO_OPTIONS_H
#define DASHARO_OPTIONS_H

#include <types.h>

enum cse_disable_mode {
	ME_MODE_ENABLE = 0,
	ME_MODE_DISABLE_HECI = 1,
	ME_MODE_DISABLE_HAP = 2,
};

/* Looks up "power_on_after_fail" option and Dasharo/"PowerFailureState"
 * variable.
 *
 * Result (same as MAINBOARD_POWER_STATE_*):
 *  - 0 - stay powered OFF
 *  - 1 - power ON
 *  - 2 - restore previous state
 */
uint8_t dasharo_get_power_on_after_fail(void);

/* Looks Dasharo/"PCIeResizeableBarsEnabled" variable if resizeable PCIe BARs
 * support was enabled at compile-time.
 *
 * Result:
 *  - true  - resizeable PCIe BARs are supported and enabled
 *  - false - resizeable PCIe BARs are disabled or not supported
 */
bool dasharo_resizeable_bars_enabled(void);

/* Looks Dasharo/"LockBios" variable if bootmedia lock was enabled at compile
 * time.
 *
 * Result:
 *  - true  - bootmedia lock is supported and enabled
 *  - false - bootmedia lock is disabled or not supported
 */
bool is_vboot_locking_permitted(void);

/* Looks Dasharo/"IommuConfig" variable if early DMA protection lock was
 * enabled at compile time.
 *
 * Result:
 *  - true  - DMA protection is supported and enabled
 *  - false - DMA protection is disabled or not supported
 */
bool dma_protection_enabled(void);

/* Looks Dasharo/"MeMode" variable if ME should be enabeld or disabled.
 *
 * Result:
 *  - 0 - ME enabled
 *  - 1 - ME soft disabled
 *  - 2 - ME HAP disabled
 */
uint8_t cse_get_me_disable_mode(void);

#endif /* DASHARO_OPTIONS_H */
