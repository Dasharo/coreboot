/*
 * This file is part of the coreboot project.

 *
 * Copyright (C) 2015 Eltan B.V.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <agesawrapper.h>

#define APU2_SPD_STRAP0_GPIO    	0x40				// GPIO49
#define APU2_SPD_STRAP0_FUNC    	Function2
#define APU2_SPD_STRAP1_GPIO    	0x41				// GPIO50
#define APU2_SPD_STRAP1_FUNC    	Function2
#define APU2_PE3_RST_L_GPIO		0x42				// GPIO51
#define APU2_PE3_RST_L_FUNC		Function2
#define APU2_PE4_RST_L_GPIO		0x43				// DEVSLP[0]/GPIO59
#define APU2_PE4_RST_L_FUNC		Function3
#define APU2_LED1_L_GPIO		0x44				// GPIO57
#define APU2_LED1_L_FUNC		Function1
#define APU2_LED2_L_GPIO		0x45				// GPIO58
#define APU2_LED2_L_FUNC		Function1
#define APU2_LED3_L_GPIO		0x46				// DEVSLP[1]/GPIO59
#define APU2_LED3_L_FUNC		Function3
#define APU2_PE3_WDIS_L_GPIO    	0x47				// GPIO64
#define APU2_PE3_WDIS_L_FUNC    	Function2
#define APU2_PE4_WDIS_L_GPIO    	0x48				// GPIO68
#define APU2_PE4_WDIS_L_FUNC	        Function0
#define APU2_SKR_GPIO			0x5B				// SPKR/GPIO66
#define APU2_SKR_FUNC			Function0
#define APU2_PROCHOT_GPIO		0x4D				// GPIO71
#define APU2_PROCHOT_FUNC		Function0
#define APU2_BIOS_CONSOLE_GPIO	        0x59				// GENINT1_L/GPIO32
#define APU2_BIOS_CONSOLE_FUNC	        Function0
#define APU3_SIMSWAP_GPIO		0x5A				// GENINT2_L/GPIO33
#define APU3_SIMSWAP_FUNC		Function0
#define APU5_SIMSWAP2_GPIO		0x59				// GENINT1_L/GPIO32
#define APU5_SIMSWAP2_FUNC		Function0
#define APU5_SIMSWAP3_GPIO		0x5A				// GENINT2_L/GPIO33
#define APU5_SIMSWAP3_FUNC		Function0
