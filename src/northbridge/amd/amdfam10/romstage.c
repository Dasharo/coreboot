/*/* SPDX-License-Identifier: GPL-2.0-only */

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

static void recover_postcar_frame(struct postcar_frame *pcf, int s3resume)
{
	msr_t base, mask;
	int i;

	/* Replicate non-UC MTRRs */
	for (i = 0; i < pcf->ctx.max_var_mtrrs; i++) {
		mask = rdmsr(MTRR_PHYS_MASK(i));
		base = rdmsr(MTRR_PHYS_BASE(i));
		u32 size = ~(mask.lo & ~0xfff) + 1;
		u8 type = base.lo & 0x7;
		base.lo &= ~0xfff;

		if (!(mask.lo & MTRR_PHYS_MASK_VALID) ||
			(type == MTRR_TYPE_UNCACHEABLE))
			continue;

		postcar_frame_add_mtrr(pcf, base.lo, size, type);
	}
}


void mainboard_romstage_entry(void)
{
	u8 power_on_reset = 0;
	u32 bsp_apicid = 0;
	int s3resume;
	struct postcar_frame pcf;
	uintptr_t top_of_ram;

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

	southbridge_before_pci_init();

	if (s3resume && cbmem_recovery(s3resume))
		printk(BIOS_EMERG, "Unable to recover CBMEM\n");

	halt();

	romstage_handoff_init(s3resume);
	amdmct_cbmem_store_info(&sysinfo);
	postcar_frame_init(&pcf, 0);
	recover_postcar_frame(&pcf, s3resume);

	/*
	 * We need to make sure ramstage will be run cached. At this point exact
	 * location of ramstage in cbmem is not known. Instruct postcar to cache
	 * 16 megs under cbmem top which is a safe bet to cover ramstage.
	 */
	top_of_ram = (uintptr_t) cbmem_top();
	postcar_frame_add_mtrr(&pcf, top_of_ram - 16*MiB, 16*MiB, MTRR_TYPE_WRBACK);
	/* Cache the memory-mapped boot media. */
	postcar_frame_add_romcache(&pcf, MTRR_TYPE_WRPROT);
	run_postcar_phase(&pcf);
}