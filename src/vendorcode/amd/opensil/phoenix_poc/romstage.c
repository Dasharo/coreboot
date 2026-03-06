/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include "opensil_console.h"
#include <xSIM-api.h>
#include <xPRF-api.h>

#include "../opensil.h"

uintptr_t opensil_get_low_usable_dram_address(void)
{
	SIL_CONTEXT SilContext = {
		.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS,
		.SilMemBaseAddress = 0 /* cbmem can't be ready now to allocate memory for OpenSIL */
	};

	uintptr_t low_usable_dram_addr = xPrfGetLowUsableDramAddress(&SilContext);
	printk(BIOS_DEBUG, "xPrfGetLowUsableDramAddress: 0x%lx\n", low_usable_dram_addr);

	return low_usable_dram_addr;
}
