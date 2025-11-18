/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootblock_common.h>
#include <option.h>
#include <intelblocks/fast_spi.h>
#include <intelblocks/rtc.h>
#include <intelblocks/systemagent.h>
#include <intelblocks/tco.h>
#include <intelblocks/uart.h>
#include <reset.h>
#include <soc/bootblock.h>
#include <device/pci_ops.h>

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

static void set_top_swap(void) {
	uint8_t cmos_slotb_option, topswap_control_bit;
	cmos_slotb_option = get_uint_option("attempt_slot_b", 0);
	topswap_control_bit = get_rtc_buc_top_swap_status();
	printk(BIOS_DEBUG, "Top Swap: CMOS option attempt_slot_b: %d\n", cmos_slotb_option);
	printk(BIOS_DEBUG, "Top Swap: RTC BUC control bit: %d\n", topswap_control_bit);
	if (cmos_slotb_option != topswap_control_bit) {
		configure_rtc_buc_top_swap(cmos_slotb_option);
		printk(BIOS_DEBUG, "Top Swap: RTC BUC control bit set to: %d, platform reset is necessary\n", get_rtc_buc_top_swap_status());
		board_reset();
	}
}

void bootblock_soc_init(void)
{
	report_platform_info();
	bootblock_pch_init();

	/* Programming TCO_BASE_ADDRESS and TCO Timer Halt */
	tco_configure();

	if (CONFIG(INTEL_TOP_SWAP_OPTION_CONTROL)) {
		set_top_swap();
	}
}
