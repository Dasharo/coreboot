/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018 PC Engines GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _BIOS_KNOBS_H
#define _BIOS_KNOBS_H

#include <stdint.h>

u8 check_iommu(void);
u8 check_console(void);
u8 check_uartc(void);
u8 check_uartd(void);
u8 check_ehci0(void);
u8 check_mpcie2_clk(void);
u8 check_pciepm(void);
u8 check_pciereverse(void);
int check_com2(void);
int check_boost(void);
u8 check_sd3_mode(void);
u16 get_watchdog_timeout(void);

#endif
