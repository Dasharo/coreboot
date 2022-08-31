/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <cpu/x86/lapic.h>
#include <program_loading.h>
#include <stdlib.h>
#include <security/tpm/tis.h>

void platform_prog_run(struct prog *prog)
{
	void *skl = NULL;

	/*
	 * APs have to be in wait-for-SIPI state for at least 1000 cycles before
	 * SKINIT. Send INIT now and assume that loading SKL from CBFS is long
	 * enough.
	 */
	lapic_send_ipi_others(LAPIC_INT_LEVELTRIG | LAPIC_INT_ASSERT | LAPIC_MT_INIT);

	skl = memalign(64*KiB, 64*KiB);

	cbfs_load(CONFIG_CBFS_PREFIX "/drtm_payload", skl, 64*KiB);

	/* TODO: fill SKL input data */

	asm volatile ("skinit" :: "a"(skl));
}
