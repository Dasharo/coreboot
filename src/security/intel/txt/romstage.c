/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2019 9elements Agency GmbH
 * Copyright (C) 2019 Facebook Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/mmio.h>
#include <arch/cpu.h>
#include <console/console.h>
#include <stdint.h>
#include <cpu/x86/lapic.h>
#include <cpu/x86/msr.h>
#include <security/tpm/tis.h>
#include <cf9_reset.h>
#include <delay.h>

#include "txt.h"
#include "txt_register.h"
#include "txt_getsec.h"


static bool get_wake_error_status(void)
{
	const uint8_t error = read8((void *)TXT_ESTS);
	return !!(error & TXT_ESTS_WAKE_ERROR_STS);
}

static bool is_reset_set(void)
{
	const uint8_t error = read8((void *)TXT_ESTS);
	return !!(error & TXT_ESTS_TXT_RESET_STS);
}

static void config_aps(void)
{
	/* Keep in sync with txt_ap_entry.ld */
	const uint8_t sipi_vector = 0xef;
	uint32_t num_cpus;
	msr_t msr;

	/* Read MSR_CORE_THREAD_COUNT */
	msr = rdmsr(0x35);
	num_cpus = (msr.lo >> 0) & 0xffff;

	printk(BIOS_INFO, "TEE-TXT: TXT detected %llu registered threads\n",
	       read64((const volatile void *)TXT_THREADS_EXIST));
	printk(BIOS_INFO, "TEE-TXT: Preparing %d APs for TXT init\n", num_cpus - 1);

	if (num_cpus == 1)
		return;

	write32((void *)TXT_MLE_JOIN, 0);

	msr = rdmsr(LAPIC_BASE_MSR);

	/* Check for x2APIC mode */
	if ((msr.lo & 0xc00) == 0xc00) {
		printk(BIOS_INFO, "TEE-TXT: x2APIC mode detected\n");
		msr.hi = 0;
		msr.lo = 0x1ff;
		/* If x2APIC mode is enabled we need to enable APIC via IA32_X2APIC_SIVR */
		wrmsr(0x80f, msr);
	} else {
		lapic_write_around(LAPIC_SPIV, 0x1ff);
	}

	printk(BIOS_DEBUG, "TEE-TXT: Sending INIT\n");
	/* Send INIT IPI to all but self. */
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(0));
	lapic_write_around(LAPIC_ICR, LAPIC_DEST_ALLBUT | LAPIC_DM_INIT);
	printk(BIOS_DEBUG, "TEE-TXT: Delay 10 ms\n");
	mdelay(10);

	printk(BIOS_DEBUG, "TEE-TXT: Sending first SIPI\n");
	/* Send SIPI */
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(0));
	lapic_write_around(LAPIC_ICR, LAPIC_DEST_ALLBUT | LAPIC_DM_STARTUP | sipi_vector);
	printk(BIOS_DEBUG, "TEE-TXT: Delay 200 us\n");
	udelay(200);

	printk(BIOS_INFO, "TEE-TXT: Sending second SIPI\n");

	/* Send second SIPI */
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(0));
	lapic_write_around(LAPIC_ICR, LAPIC_DEST_ALLBUT | LAPIC_DM_STARTUP | sipi_vector);

	printk(BIOS_DEBUG, "TEE-TXT: Waiting till APs finish their work\n");

	/* Wait for APs to do their job */
	while (read32((void *)TXT_MLE_JOIN) != num_cpus - 1);

	printk(BIOS_DEBUG, "TEE-TXT: Putting APs in wati-for-SIPI state\n");

	/* Put APs in wait-for-SIPI state for ACM */
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(0));
	lapic_write_around(LAPIC_ICR, LAPIC_DEST_ALLBUT | LAPIC_DM_INIT);
	mdelay(10);
	printk(BIOS_DEBUG, "TEE-TXT: AP cofniguration finished\n");
}

/**
 * Log TXT startup errors, check all bits for TXT, run BIOSACM using
 * GETSEC[ENTERACCS].
 *
 * If a "TXT reset" is detected or "memory had secrets" is set, then we need to
 * run SCLEAN and/or do global reset.
 *
 */
void intel_txt_acm_sclean(void)
{
	const uint64_t status = read64((void *)TXT_SPAD);
	msr_t msr = {.lo = 0, .hi = 0 };

	if (status & ACMSTS_TXT_DISABLED)
		return;

	printk(BIOS_INFO, "TEE-TXT: Initializing TEE...\n");

	intel_txt_log_spad();

	if (CONFIG(INTEL_TXT_LOGGING)) {
		intel_txt_log_bios_acm_error();
		txt_dump_chipset_info();
	}

	printk(BIOS_INFO, "TEE-TXT: Validate TEE...\n");

	if (intel_txt_prepare_txt_env()) {
		printk(BIOS_ERR, "TEE-TXT: Failed to prepare TXT environment\n");
		return;
	}

	/* Prepare to run SCLEAN if TPM Establishment asserted and TXT Wake Error detected */
	if (tis_is_establishment_set() && get_wake_error_status()) {
		printk(BIOS_INFO, "TEE-TXT: TPM Establishment asserted or TXT wake error\n");
		printk(BIOS_INFO, "TEE-TXT: Preparing to run SCLEAN\n");
		if (is_reset_set()) {
			printk(BIOS_INFO, "TEE-TXT: TXT Reset set. Performing global reset\n");
			global_system_reset();
			return;
		}

		config_aps();

		printk(BIOS_INFO, "TEE-TXT: Calling SCLEAN\n");
		intel_txt_run_bios_acm(ACMINPUT_SCLEAN);
		global_full_reset();
	}

	/* We don't need to run SCLEAN, simply unlock the memory */
	wrmsr(TXT_UNLOCK_MEMORY_MSR, msr);
}
