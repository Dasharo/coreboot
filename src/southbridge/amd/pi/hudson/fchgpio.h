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
#include "FchPlatform.h"

#define GPIO_DEFINITION(gpio, function, outputenable, output, pullup, pulldown) \
{gpio, function,  (outputenable << 7) | (output << 6) | (pullup << 4) | (pulldown << 5) }

void 	HandleFchGpioTbl ( GPIO_CONTROL *pGpioTbl );
void 	ConfigureFchGpio(u8 gpio, u8 iomux_ftn, u8 setting);
u8 		ReadFchGpio(u8 gpio);
void 	WriteFchGpio(u8 gpio, u8 setting);
void 	DumpGpioConfiguration( void );

#define GPIO_DATA_LOW   0
#define GPIO_DATA_HIGH  1
#define GPIO_INVALID  	0xFF

#define MAX_GPIO	183						// Maximum GPIO supported

//
//  FCH MMIO Base (GPIO BANK0)
//    offset : 0x1500
//
#define FCH_GPIO_PIN_STS     		BIT0	// Bit 16
#define FCH_GPIO_PULL_UP_ENABLE     BIT4	// Bit 20
#define FCH_GPIO_PULL_DOWN_ENABLE   BIT5	// Bit 21
#define FCH_GPIO_OUTPUT_VALUE       BIT6	// Bit 22
//#define FCH_GPIO_OUTPUT_ENABLE      BIT7	// Bit 23
#define FCH_GPIO_ALL_CONFIG			(FCH_GPIO_PULL_UP_ENABLE | FCH_GPIO_PULL_DOWN_ENABLE |FCH_GPIO_OUTPUT_VALUE | FCH_GPIO_OUTPUT_ENABLE)







