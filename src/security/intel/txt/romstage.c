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
	struct cpuid_result res;
	res = cpuid(1);
	uint32_t num_cpus = (res.ebx >> 16) & 0xff;

	if (cpu_have_cpuid() && cpuid_get_max_func() >= 0xb) {
		uint32_t leaf_b_cores = 0, leaf_b_threads = 0;

		res = cpuid_ext(0xb, 1);
		leaf_b_cores = res.ebx;
		res = cpuid_ext(0xb, 0);
		leaf_b_threads = res.ebx;

		/* if hyperthreading is not available, pretend this is 1 */
		if (leaf_b_threads == 0)
			leaf_b_threads = 1;
		else
			printk(BIOS_INFO, "TEE-TXT: Hyperthreading capable CPU detected\n");

		num_cpus = leaf_b_cores;
	}

	printk(BIOS_INFO, "TEE-TXT: Preparing %d APs for TXT init\n", num_cpus - 1);

	if (num_cpus == 1)
		return;

	write32((void *)TXT_MLE_JOIN, 0);

	setup_lapic();

	printk(BIOS_DEBUG, "TEE-TXT: Asserting INIT\n");
	/* Send INIT IPI to all but self. */
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(0));
	lapic_write_around(LAPIC_ICR, LAPIC_DEST_ALLBUT | LAPIC_INT_LEVELTRIG
			   | LAPIC_INT_ASSERT | LAPIC_DM_INIT);
	printk(BIOS_DEBUG, "TEE-TXT: Delay 10 ms\n");
	mdelay(10);

	printk(BIOS_DEBUG, "TEE-TXT: Deasserting INIT\n");
	/* Send INIT IPI to all but self. */
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(0));
	lapic_write_around(LAPIC_ICR, LAPIC_DEST_ALLBUT | LAPIC_INT_LEVELTRIG
			    | LAPIC_DM_INIT);
	printk(BIOS_DEBUG, "TEE-TXT: Delay 10 ms\n");
	mdelay(10);

	printk(BIOS_DEBUG, "TEE-TXT: Sending first SIPI\n");
	/* Send SIPI */
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(0));
	lapic_write_around(LAPIC_ICR, LAPIC_DEST_ALLBUT | LAPIC_INT_ASSERT |
			   LAPIC_DM_STARTUP | sipi_vector);
	printk(BIOS_DEBUG, "TEE-TXT: Delay 200 us\n");
	udelay(200);

	printk(BIOS_INFO, "TEE-TXT: Sending second SIPI\n");

	/* Send second SIPI */
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(0));
	lapic_write_around(LAPIC_ICR, LAPIC_DEST_ALLBUT | LAPIC_INT_ASSERT |
			   LAPIC_DM_STARTUP | sipi_vector);

	printk(BIOS_DEBUG, "TEE-TXT: Waiting till APs finish their work\n");

	/* Wait for APs to do their job */
	while (read32((void *)TXT_MLE_JOIN) != num_cpus - 1);

	printk(BIOS_DEBUG, "TEE-TXT: Putting APs in wati-for-SIPI state\n");

	/* Put APs in wait-for-SIPI state for ACM */
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(0));
	lapic_write_around(LAPIC_ICR, LAPIC_DEST_ALLBUT | LAPIC_INT_ASSERT |
			   LAPIC_DM_INIT);
	mdelay(10);
	printk(BIOS_DEBUG, "TEE-TXT: AP cofniguration finished\n");
}

/**
 * Log TXT startup errors, check all bits for TXT, run BIOSACM using
 * GETSEC[ENTERACCS].
 *
 * If a "TXT reset" is detected or "memory had secrets" is set, then do nothing as
 * 1. Running ACMs will cause a TXT-RESET
 * 2. Memory will be scrubbed in BS_DEV_INIT
 * 3. TXT-RESET will be issued by code above later
 *
 */
void intel_txt_acm_check(void)
{
	const uint64_t status = read64((void *)TXT_SPAD);

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

	if (is_reset_set()) {
		printk(BIOS_INFO, "TEE-TXT: Performing global reset\n");
		global_system_reset();
		return;
	}

	config_aps();

	/*
	 * Framework TXT Reference Code Design Specification and Integration Guide:
	 * If Wake Error Status bit equals 1 - memory is locked.
	 */
	if (get_wake_error_status()) {
		printk(BIOS_ERR, "TEE-TXT: Calling SCLEAN\n");
		intel_txt_run_bios_acm(ACMINPUT_SCLEAN);
		global_full_reset();
		return;
	}

	printk(BIOS_INFO, "TEE-TXT: Testing BIOS ACM calling code...\n");

	/*
	 * Test BIOS ACM code.
	 * ACM should do nothing on reserved functions, and return an error code
	 * in TXT_BIOSACM_ERRORCODE. Tested show that this is not true.
	 * Use special function "NOP" that does 'nothing'.
	 */
	if (intel_txt_run_bios_acm(ACMINPUT_NOP) < 0) {
		printk(BIOS_ERR, "TEE-TXT: Error calling BIOS ACM.\n");
		return;
	}
}
