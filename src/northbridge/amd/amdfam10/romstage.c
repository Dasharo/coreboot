/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <arch/romstage.h>
#include <console/console.h>
#include <cpu/x86/mtrr.h>
#include <southbridge/amd/sb700/sb700.h>
#include <cpu/amd/family_10h-family_15h/family_10h_15h.h>
#include <program_loading.h>
#include <romstage_handoff.h>
#include <cbmem.h>
#include <halt.h>

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

static struct sys_info sysinfo;

struct sys_info *get_sysinfo(void)
{
	return &sysinfo;
}

static void ap_romstage_entry(void)
{
	printk(BIOS_WARNING, "sysinfo range: [%p,%p]\n", get_sysinfo(), get_sysinfo() + 1);

	mainboard_sysinfo_hook(get_sysinfo());

	initialize_cores(get_sysinfo());
}

static void save_ap_romstage_ptr(void)
{
	save_bios_ram_data((u32)ap_romstage_entry, 4, BIOSRAM_AP_ENTRY);
}

void mainboard_romstage_entry(void)
{
	u8 power_on_reset = 0;
	u32 bsp_apicid = 0;
	int s3resume;
	int cbmem_initted = 0;
	struct postcar_frame pcf;

	save_ap_romstage_ptr();

	printk(BIOS_WARNING, "sysinfo range: [%p,%p]\n", get_sysinfo(), get_sysinfo() + 1);

	mainboard_sysinfo_hook(&sysinfo);

	bsp_apicid = initialize_cores(&sysinfo);

	/* Setup sysinfo defaults */
	set_sysinfo_in_ram(0);

	if (!sb7xx_51xx_decode_last_reset())
		power_on_reset = 1;

	setup_bsp(&sysinfo, power_on_reset);
	post_code(0x38);

	southbridge_early_setup();

	s3resume = acpi_is_wakeup_s3();
	printk(BIOS_WARNING, "S3 resume state: %d\n", s3resume);

	mainboard_early_init(s3resume);

	early_cpu_finalize(&sysinfo, bsp_apicid);

	southbridge_ht_init();

	post_code(0x3B);
	/* Wait for all APs to be stopped, otherwise RAM initialization may hang */
	if (CONFIG(LOGICAL_CPUS))
		wait_all_other_cores_stopped(bsp_apicid);

	/* raminit */
	post_code(0x3D);
	mainboard_spd_info(&sysinfo);
	post_code(0x40);
	raminit_amdmct(&sysinfo);
	mainboard_after_raminit(&sysinfo);
	post_code(0x41);

	display_mtrrs();

	southbridge_before_pci_init();

	cbmem_initted = !cbmem_recovery(s3resume);
	romstage_handoff_init(cbmem_initted && s3resume);
	amdmct_cbmem_store_info(&sysinfo);

	set_sysinfo_in_ram(1);

	prepare_and_run_postcar(&pcf);
}

void fill_postcar_frame(struct postcar_frame *pcf)
{
	/* Cache 8 MiB region below the top of RAM to cover cbmem. */
	uintptr_t top_of_ram = (uintptr_t)cbmem_top();
	postcar_frame_add_mtrr(pcf, top_of_ram - 8*MiB, 8*MiB, MTRR_TYPE_WRBACK);
	/* Cache the memory-mapped boot media. */
	pcf->skip_common_mtrr = 0;
}
