/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <arch/io.h>
#include <acpi/acpi.h>
#include "SBPLATFORM.h"

int acpi_get_sleep_type(void)
{
	u16 tmp = inw(PM1_CNT_BLK_ADDRESS);
	tmp = ((tmp & (7 << 10)) >> 10);
	return (int)tmp;
}
