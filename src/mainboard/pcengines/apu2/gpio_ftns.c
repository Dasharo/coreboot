/* SPDX-License-Identifier: GPL-2.0-only */

#include <gpio.h>
#include "gpio_ftns.h"

int get_spd_offset(void)
{
	u8 index = 0;
	/*
	 * One SPD file contains all 4 options, determine which index to
	 * read here, then call into the standard routines.
	 */
	if (gpio_get(GPIO_49))
		index |= 1 << 0;
	if (gpio_get(GPIO_50))
		index |= 1 << 1;

	return index;
}

void write_gpio(u32 gpio, u8 value)
{
	u32 *memptr = (u32 *)(ACPI_MMIO_BASE + GPIO_OFFSET + gpio);
	*memptr |= (value > 0) ? GPIO_OUTPUT_VALUE : 0;
}

int get_spd_offset(void)
{
	u8 index = 0;
	/* One SPD file contains all 4 options, determine which index to
	 * read here, then call into the standard routines.
	 */

	index = read_gpio(GPIO_49);
	index |= read_gpio(GPIO_50) << 1;

	return index;
}
