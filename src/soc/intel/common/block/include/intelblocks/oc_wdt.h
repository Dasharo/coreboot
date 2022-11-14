/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdbool.h>

void wdt_reload_and_start(void);
void wdt_disable(void);
bool is_wdt_failure(void);
void wdt_allow_known_reset(void);
bool is_wdt_required(void);
bool is_wdt_enabled(void);
void wdt_reset_check(void);
