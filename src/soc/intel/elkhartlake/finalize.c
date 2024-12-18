/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <console/console.h>
#include <commonlib/console/post_codes.h>
#include <cpu/x86/smm.h>
#include <device/mmio.h>
#include <device/pci.h>
#include <intelblocks/cse.h>
#include <intelblocks/lpc_lib.h>
#include <intelblocks/pcr.h>
#include <intelblocks/pmclib.h>
#include <intelblocks/systemagent.h>
#include <intelblocks/tco.h>
#include <soc/p2sb.h>
#include <soc/pci_devs.h>
#include <soc/pcr_ids.h>
#include <soc/pm.h>
#include <soc/smbus.h>
#include <soc/soc_chip.h>
#include <soc/systemagent.h>
#include <spi-generic.h>

static void pch_finalize(void)
{
	/* TCO Lock down */
	tco_lockdown();

	/* TODO: Add Thermal Configuration */

	pmc_clear_pmcon_sts();
}

static void heci_finalize(void)
{
	bool heci_disable = false;

	heci_set_to_d0i3();

	if (is_cse_enabled()) {
		heci_disable = cse_is_hfs1_com_debug() ||
			       cse_is_hfs1_com_soft_temp_disable();
	}

	if (CONFIG(DISABLE_HECI1_AT_PRE_BOOT) || heci_disable)
		heci1_disable();
}

static void soc_finalize(void *unused)
{
	printk(BIOS_DEBUG, "Finalizing chipset.\n");

	pch_finalize();
	apm_control(APM_CNT_FINALIZE);
	if (CONFIG(USE_FSP_NOTIFY_PHASE_READY_TO_BOOT) &&
		 CONFIG(USE_FSP_NOTIFY_PHASE_END_OF_FIRMWARE))
		heci_finalize();

	/* Indicate finalize step with post code */
	post_code(POSTCODE_OS_BOOT);
}

BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, soc_finalize, NULL);
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, soc_finalize, NULL);
