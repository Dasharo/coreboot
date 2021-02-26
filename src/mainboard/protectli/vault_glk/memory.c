/* SPDX-License-Identifier: GPL-2.0-only */

#include <gpio.h>
#include <soc/meminit.h>
#include <soc/gpio.h>
#include "variants.h"

size_t __weak variant_memory_sku(void)
{
	return 0;
}
