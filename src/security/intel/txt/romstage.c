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
#include <console/console.h>
#include <stdint.h>

#include "txt.h"
#include "txt_register.h"
#include "txt_getsec.h"


static bool get_wake_error_status(void)
{
	const uint8_t error = read8((void *)TXT_ESTS);
	return !!(error & TXT_ESTS_WAKE_ERROR_STS);
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

	/* Check for fatal ACM error and TXT reset */
	if (get_wake_error_status()) {
		/* Can't run ACMs with TXT_ESTS_WAKE_ERROR_STS set */
		printk(BIOS_ERR, "TEE-TXT: Failed to prepare TXT environment\n");
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
