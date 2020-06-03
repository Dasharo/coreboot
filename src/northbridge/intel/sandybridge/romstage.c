/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cf9_reset.h>
#include <device/pci_ops.h>
#include <cpu/x86/lapic.h>
#include <timestamp.h>
#include "sandybridge.h"
#include <arch/romstage.h>
#include <device/pci_def.h>
#include <device/device.h>
#include <cpu/intel/common/common.h>
#include <northbridge/intel/sandybridge/chip.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <southbridge/intel/common/pmclib.h>
#include <security/intel/txt/txt.h>
#include <security/intel/txt/txt_getsec.h>
#include <security/intel/txt/txt_register.h>
#include <security/tpm/tspi.h>
#include <elog.h>

__weak void mainboard_early_init(int s3_resume)
{
}

__weak void mainboard_late_rcba_config(void)
{
}

static void early_pch_reset_pmcon(void)
{
	/* Reset RTC power status */
	pci_and_config8(PCH_LPC_DEV, GEN_PMCON_3, ~(1 << 2));
}

static void dump_txt_capabilities(void)
{
	uint32_t version_mask;
	uint32_t version_numbers_supported;
	uint32_t max_size_acm_area;
	uint32_t memory_type_mask;
	uint32_t senter_function_disable;
	uint32_t txt_feature_flags;

	if (!getsec_parameter(&version_mask, &version_numbers_supported,
			      &max_size_acm_area, &memory_type_mask,
			      &senter_function_disable, &txt_feature_flags)) {
		printk(BIOS_WARNING, "Could not obtain GETSEC parameters\n");
		return;
	}
	printk(BIOS_DEBUG, "TXT: ACM Version comparison mask: %08x\n", version_mask);
	printk(BIOS_DEBUG, "TXT: ACM Version numbers supported: %08x\n", version_numbers_supported);
	printk(BIOS_DEBUG, "TXT: Max size of authenticated code execution area: %08x\n",
		max_size_acm_area);
	printk(BIOS_DEBUG, "TXT: External memory types supported during AC mode: %08x\n",
		memory_type_mask);
	printk(BIOS_DEBUG, "TXT: Selective SENTER functionality control: %08x\n",
		senter_function_disable);
	printk(BIOS_DEBUG, "TXT: TXT Feature Extensions Flags: %08x\n", txt_feature_flags);
	printk(BIOS_DEBUG, "TXT: \tS-CRTM Capability rooted in: ");
	if (txt_feature_flags & GETSEC_PARAMS_TXT_EXT_CRTM_SUPPORT) {
		printk(BIOS_DEBUG, "processor\n");
	} else {
		printk(BIOS_DEBUG, "BIOS\n");
	}
	printk(BIOS_DEBUG, "TXT: \tMachine Check Register: ");
	if (txt_feature_flags & GETSEC_PARAMS_TXT_EXT_MACHINE_CHECK) {
		printk(BIOS_DEBUG, "preserved\n");
	} else {
		printk(BIOS_DEBUG, "must be clear\n");
	}
}

/* The romstage entry point for this platform is not mainboard-specific, hence the name */
void mainboard_romstage_entry(void)
{
	int s3resume = 0;

	if (MCHBAR16(SSKPD_HI) == 0xCAFE)
		system_reset();

	enable_lapic();

	/* Init LPC, GPIO, BARs, disable watchdog ... */
	early_pch_init();

	/* When using MRC, USB is initialized by MRC */
	if (CONFIG(USE_NATIVE_RAMINIT)) {
		early_usb_init(mainboard_usb_ports);
	}

	/* Perform some early chipset init needed before RAM initialization can work */
	systemagent_early_init();
	printk(BIOS_DEBUG, "Back from systemagent_early_init()\n");

	s3resume = southbridge_detect_s3_resume();

	elog_boot_notify(s3resume);

	post_code(0x38);

	mainboard_early_init(s3resume);

	post_code(0x39);

	if (CONFIG(LEGACY_INTEL_TXT)) {
		if (tpm_setup(s3resume) != TPM_SUCCESS)
			printk(BIOS_WARNING, "TPM setup failed\n");
		set_vmx_and_lock();
		if (intel_txt_prepare_txt_env())
			printk(BIOS_WARNING, "Preparing Intel TXT failed\n");
		dump_txt_capabilities();
		intel_txt_acm_check();
	}

	perform_raminit(s3resume);

	timestamp_add_now(TS_AFTER_INITRAM);

	post_code(0x3b);
	/* Perform some initialization that must run before stage2 */
	early_pch_reset_pmcon();
	post_code(0x3c);

	southbridge_configure_default_intmap();
	southbridge_rcba_config();
	mainboard_late_rcba_config();

	post_code(0x3d);

	northbridge_romstage_finalize(s3resume);

	post_code(0x3f);
}
