/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <intelblocks/fast_spi.h>
#include <intelblocks/oc_wdt.h>
#include <intelblocks/systemagent.h>
#include <intelblocks/tco.h>
#include <intelblocks/uart.h>
#include <soc/bootblock.h>

asmlinkage void bootblock_c_entry(uint64_t base_timestamp)
{
	/* Call lib/bootblock.c main */
	bootblock_main_with_basetime(base_timestamp);
}

void bootblock_soc_early_init(void)
{
	bootblock_systemagent_early_init();
	bootblock_pch_early_init();
	fast_spi_cache_bios_region();
	pch_early_iorange_init();
	if (CONFIG(INTEL_LPSS_UART_FOR_CONSOLE))
		uart_bootblock_init();
}

void bootblock_soc_init(void)
{
	report_platform_info();
	bootblock_pch_init();

	/* Programming TCO_BASE_ADDRESS and TCO Timer Halt */
	tco_configure();

	is_wdt_failure();
	wdt_reset_check();
	if (CONFIG(SOC_INTEL_COMMON_OC_WDT_ENABLE)) {
		wdt_reload_and_start(CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT);
		is_wdt_enabled();
		/* Workaround for FSP that will also program OC WDT */
		wdt_allow_known_reset();
	} else {
		wdt_disable();
	}
}
