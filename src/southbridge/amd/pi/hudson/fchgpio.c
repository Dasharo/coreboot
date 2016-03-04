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

#include <stdint.h>
#include <arch/io.h>
#include "hudson.h"
#include "fchgpio.h"

/*----------------------------------------------------------------------------------------*/
/**
 * ProgramFchGpioTbl - FCH Gpio table (8 bits data)
 *
 * @param[in] pGpioTbl   - Table data pointer, this table should be terminated by a 0xFF byte
 *
 */
void
HandleFchGpioTbl (
  GPIO_CONTROL  *pGpioTbl
  )
{
  if (pGpioTbl != NULL) {

    while (pGpioTbl->GpioPin != 0xFF) {

      ConfigureFchGpio( pGpioTbl->GpioPin, pGpioTbl->PinFunction, pGpioTbl->CfgByte );
      pGpioTbl++;
    }
  }
}

/*----------------------------------------------------------------------------------------*/
/**
 * ConfigureFchGpio - FCH Gpio table (8 bits data)
 *
 * @param[in] pGpioTbl   - Table data pointer, this table should be terminated by a 0xFF byte
 *
 */
void ConfigureFchGpio(u8 gpio, u8 iomux_ftn, u8 setting)
{
	if ( gpio <= MAX_GPIO ) {		// Simply ignore invalid GPIO

		u8 bdata;
		u8 *memptr;

		//
		// First we prepare the GPIO setting and then we switch the MUX to the correct setting to prevent glitches
		//

		// The GPIO register is 4 bytes. We use bit 16 to 23 for the actual GPIO function
		memptr = (u8 *)(ACPI_MMIO_BASE + GPIO_BANK0_BASE + (gpio << 2) + 2 );
		bdata = *memptr;
		bdata |= (setting & FCH_GPIO_OUTPUT_VALUE);
		*memptr = bdata;							// First write out the data value to prevent glitches

		bdata |= (setting & FCH_GPIO_ALL_CONFIG); /* set direction and data value */
		*memptr = bdata;

		memptr = (u8 *)(ACPI_MMIO_BASE + IOMUX_BASE + gpio);
		*memptr = iomux_ftn;

	}
}

u8 ReadFchGpio(u8 gpio)
{
	if ( gpio <= MAX_GPIO ) {

		u8 *memptr = (u8 *)(ACPI_MMIO_BASE + GPIO_BANK0_BASE + (gpio << 2) + 2 );
		return (*memptr & FCH_GPIO_PIN_STS) ? 1 : 0;

	} else {

		return GPIO_INVALID;			// Indicate an invalid GPIO was requested
	}
}

void WriteFchGpio( u8 gpio, u8 value)
{
	if ( gpio <= MAX_GPIO ) {	// Simply ignore invalid GPIO

		u8 bdata;
		u8 *memptr = (u8 *)(ACPI_MMIO_BASE + GPIO_BANK0_BASE + (gpio << 2) + 2 );
		bdata = *memptr;
		bdata &= ~FCH_GPIO_OUTPUT_VALUE;
		if ( value ) bdata |= FCH_GPIO_OUTPUT_VALUE;
		*memptr = bdata;
	}
}

void DumpGpioConfiguration( void )
{
	int i;

	printk(BIOS_INFO, "GPIO Bank 0 Control Registers\n");

	for ( i = 0 ; i <= 0xf8 ; i = i + 4 ) {

		printk(BIOS_INFO, "GPIOx%03x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + GPIO_BANK0_BASE + i )));
	}

	printk(BIOS_INFO, "GPIO Bank 1 Control Registers\n");

	for ( i = 0x100 ; i <= 0x1fc ; i = i + 4 ) {

		printk(BIOS_INFO, "GPIOx%03x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + GPIO_BANK0_BASE + i )));
	}

	printk(BIOS_INFO, "GPIO Bank 2 Control Registers\n");

	for ( i = 0x200 ; i <= 0x2dc ; i = i + 4 ) {

		printk(BIOS_INFO, "GPIOx%03x = 0x%08x \n", i, *((u32 *)(ACPI_MMIO_BASE + GPIO_BANK0_BASE + i )));
	}

	printk(BIOS_INFO, "GPIO MUX Registers\n");

	for ( i = 0x00 ; i <= 0x26 ; i++ ) printk(BIOS_INFO, "IOMUXx%02x = %d\n", i, *((u8 *)(ACPI_MMIO_BASE + IOMUX_BASE + i )));
	for ( i = 0x40 ; i <= 0x49 ; i++ ) printk(BIOS_INFO, "IOMUXx%02x = %d\n", i, *((u8 *)(ACPI_MMIO_BASE + IOMUX_BASE + i )));
	for ( i = 0x4C ; i <= 0x4D ; i++ ) printk(BIOS_INFO, "IOMUXx%02x = %d\n", i, *((u8 *)(ACPI_MMIO_BASE + IOMUX_BASE + i )));
	for ( i = 0x54 ; i <= 0x55 ; i++ ) printk(BIOS_INFO, "IOMUXx%02x = %d\n", i, *((u8 *)(ACPI_MMIO_BASE + IOMUX_BASE + i )));
	for ( i = 0x57 ; i <= 0x65 ; i++ ) printk(BIOS_INFO, "IOMUXx%02x = %d\n", i, *((u8 *)(ACPI_MMIO_BASE + IOMUX_BASE + i )));
	for ( i = 0x71 ; i <= 0x74 ; i++ ) printk(BIOS_INFO, "IOMUXx%02x = %d\n", i, *((u8 *)(ACPI_MMIO_BASE + IOMUX_BASE + i )));
	for ( i = 0x7E ; i <= 0x7E ; i++ ) printk(BIOS_INFO, "IOMUXx%02x = %d\n", i, *((u8 *)(ACPI_MMIO_BASE + IOMUX_BASE + i )));
	for ( i = 0x81 ; i <= 0x84 ; i++ ) printk(BIOS_INFO, "IOMUXx%02x = %d\n", i, *((u8 *)(ACPI_MMIO_BASE + IOMUX_BASE + i )));
}
