/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_INTEL_COMMON_OC_WDT_H
#define SOC_INTEL_COMMON_OC_WDT_H

#include <stdbool.h>

void wdt_reload_and_start(void);
void wdt_disable(void);
bool is_wdt_enabled(void);
uint16_t wdt_get_current_timeout(void);

#endif
