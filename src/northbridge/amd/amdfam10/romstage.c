/*/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <arch/romstage.h>
#include <southbridge/amd/sb700/sb700.h>

#include "raminit.h"
#include "amdfam10.h"

__weak void mainboard_early_init(int s3_resume)
{
}

__weak void mainboard_sysinfo_hook(struct sys_info *sysinfo)
{
}

__weak void mainboard_spd_info(struct sys_info *sysinfo)
{
}

__weak void mainboard_after_raminit(struct sys_info *sysinfo)
{
}

void mainboard_romstage_entry(void)
{
	u8 power_on_reset = 0;
	u32 bsp_apicid = 0;
	int s3resume;
	struct sys_info *sysinfo = get_sysinfo();

	mainboard_sysinfo_hook(sysinfo);
	
	bsp_apicid = initialize_aps(sysinfo);

	/* Setup sysinfo defaults */
	set_sysinfo_in_ram(0);

	if (!sb7xx_51xx_decode_last_reset())
		power_on_reset = 1;

	setup_bsp(sysinfo, power_on_reset);

	post_code(0x38);

	southbridge_early_setup();

	s3resume = acpi_is_wakeup_s3();

	mainboard_early_init(s3resume);

	early_cpu_finalize(sysinfo, bsp_apicid);

	southbridge_ht_init();

	post_code(0x3B);
	/* Wait for all APs to be stopped, otherwise RAM initialization may hang */
	if (CONFIG(LOGICAL_CPUS))
		wait_all_other_cores_stopped(bsp_apicid);

	/* raminit */
	mainboard_spd_info(sysinfo);
	post_code(0x3D);

	post_code(0x40);

	//raminit_amdmct(sysinfo);
	halt();

	mainboard_after_raminit(sysinfo);

	post_code(0x41);

	southbridge_before_pci_init();

	romstage_handoff_init(s3resume);

	amdmct_cbmem_store_info(sysinfo);
}