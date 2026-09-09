/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <arch/cpu.h>
#include <cbmem.h>
#include <console/console.h>
#include <cpu/intel/common/common.h>
#include <cpu/x86/cr.h>
#include <device/mmio.h>
#include <cpu/x86/msr.h>
#include <smp/node.h>
#if CONFIG(SOC_INTEL_COMMON_BLOCK_PMC)
#include <intelblocks/pmclib.h>
#endif
#if CONFIG(SOUTHBRIDGE_INTEL_COMMON_PMBASE)
#include <southbridge/intel/common/pmbase.h>
#include <southbridge/intel/common/pmutil.h>
#endif
#include <timer.h>
#include <types.h>

#include "txt.h"
#include "txtlib.h"
#include "txt_register.h"
#include "txt_getsec.h"

static bool is_txt_chipset(void)
{
	uint32_t eax;

	const bool success = getsec_capabilities(&eax);

	return success && eax & 1;
}

/* Print the bad news */
static void print_memory_is_locked(void)
{
	if (!CONFIG(INTEL_TXT_LOGGING))
		return;

	printk(BIOS_EMERG, "FATAL: Cannot run SCLEAN. Memory will remain locked.\n");
	printk(BIOS_EMERG, "\n");
	printk(BIOS_EMERG, "If you still want to boot, your options are:\n");
	printk(BIOS_EMERG, "\n");
	printk(BIOS_EMERG, "   1. Flash a coreboot image with a valid BIOS ACM.\n");
	printk(BIOS_EMERG, "      Then, try again and hope it works this time.\n");
	printk(BIOS_EMERG, "\n");
	printk(BIOS_EMERG, "   2. If possible, remove the TPM from the system.\n");
	printk(BIOS_EMERG, "      Reinstalling the TPM might lock memory again.\n");
	printk(BIOS_EMERG, "\n");
	printk(BIOS_EMERG, "   3. Disconnect all power sources, and RTC battery.\n");
	printk(BIOS_EMERG, "      This may not work on all TXT-enabled platforms.\n");
	printk(BIOS_EMERG, "\n");
}

void intel_txt_romstage_init(void)
{
	/* Bail early if the CPU doesn't support TXT */
	if (!is_txt_cpu()) {
		printk(BIOS_ERR, "TEE-TXT: CPU not TXT capable.\n");
		return;
	}

	/*
	 * We need to use GETSEC here, so enable it.
	 * CR4_SMXE is all we need to be able to call GETSEC[CAPABILITIES]
	 * or GETSEC[ENTERACCS] for SCLEAN.
	 */
	write_cr4(read_cr4() | CR4_SMXE);

	if (!is_txt_chipset()) {
		printk(BIOS_ERR, "TEE-TXT: Chipset not TXT capable.\n");
		return;
	}

	const uint8_t txt_ests = read8p(TXT_ESTS);

	const bool establishment = is_establishment_bit_asserted();
	const bool is_wake_error = !!(txt_ests & TXT_ESTS_WAKE_ERROR_STS);

	if (CONFIG(INTEL_TXT_LOGGING)) {
		printk(BIOS_INFO, "TEE-TXT: TPM established: %s\n",
		       establishment ? "true" : "false");
	}

	if (establishment && is_wake_error) {
		printk(BIOS_ERR, "TEE-TXT: Secrets remain in memory. SCLEAN is required.\n");

		if (txt_ests & TXT_ESTS_TXT_RESET_STS) {
			printk(BIOS_ERR, "TEE-TXT: TXT_RESET bit set, doing global reset!\n");
			txt_reset_platform();
		}

		/* FIXME: Clear SLP_TYP# */
#if CONFIG(SOC_INTEL_COMMON_BLOCK_PMC)
		pmc_disable_pm1_control(SLP_TYP);
#endif
#if CONFIG(SOUTHBRIDGE_INTEL_COMMON_PMBASE)
		write_pmbase32(PM1_CNT, read_pmbase32(PM1_CNT) & ~SLP_TYP);
#endif
		intel_txt_run_sclean();

		/* If running the BIOS ACM is impossible, manual intervention is required */
		print_memory_is_locked();

		/* FIXME: vboot A/B could be used to recover, but has not been tested */
		die("Could not execute BIOS ACM to unlock the memory.\n");
	}
}

/*
 * Size of a single sample and the distance between samples when checking that
 * DRAM really was cleared. Small enough not to add measurably to the boot
 * time, dense enough to hit every DIMM rank and every channel.
 */
#define DRAM_SAMPLE_SIZE	(4 * KiB)
#define DRAM_SAMPLE_STRIDE	(8 * MiB)

/*
 * Running CLEAR_SECRETS tells the platform that no secrets remain in DRAM, and
 * nothing but this check stands behind that claim: memory init was *asked* to
 * scrub DRAM, it does not report back whether it did.
 *
 * Only memory below the cbmem region is looked at. Everything above it belongs
 * to coreboot, to FSP and to the SMM and DPR ranges, all written after the
 * scrub. Memory above 4 GiB cannot be reached from a 32-bit romstage without
 * paging, so this is a sample and not a proof - but it does catch memory init
 * silently ignoring the request, which is the failure mode that matters here.
 */
static bool dram_was_cleared(void)
{
	void *cbmem_base;
	size_t cbmem_size;

	if (cbmem_get_region(&cbmem_base, &cbmem_size)) {
		printk(BIOS_ERR, "TEE-TXT: No cbmem region to bound the DRAM check\n");
		return false;
	}

	const uintptr_t start = 1 * MiB;
	const uintptr_t end = (uintptr_t)cbmem_base;

	if (end < start + DRAM_SAMPLE_SIZE) {
		printk(BIOS_ERR, "TEE-TXT: Too little DRAM below cbmem to check\n");
		return false;
	}

	for (uintptr_t addr = start; addr + DRAM_SAMPLE_SIZE <= end;
	     addr += DRAM_SAMPLE_STRIDE) {
		const uint32_t *sample = (const uint32_t *)addr;

		for (size_t i = 0; i < DRAM_SAMPLE_SIZE / sizeof(*sample); i++) {
			if (!sample[i])
				continue;

			printk(BIOS_ERR, "TEE-TXT: DRAM at 0x%lx is not cleared: 0x%08x\n",
			       (unsigned long)(addr + i * sizeof(*sample)), sample[i]);
			return false;
		}
	}

	printk(BIOS_INFO, "TEE-TXT: DRAM below 0x%lx sampled as cleared\n",
	       (unsigned long)end);

	return true;
}

/*
 * Tear down a measured launch environment that was left behind by a previous
 * boot, by clearing TXT.E2STS.SECRET_STS with the BIOS ACM's CLEAR_SECRETS
 * function. To be called from romstage, right after memory init:
 *
 *  - The CBnT specification has Startup BIOS clear memory and then invoke this
 *    ACM function ("After memory is scrubbed properly, Startup BIOS invokes
 *    Clear Secrets ACM function to clear Secrets bit and resets platform").
 *  - GETSEC[ENTERACCS] requires INVD before entering the ACM for this
 *    function, and INVD raises #GP(0) once firmware has set
 *    MSR_BIOS_DONE.ENABLE_IA_UNTRUSTED, which happens during FSP-S. romstage
 *    is the last place where this can run at all.
 *  - Unlike SCLEAN, this function does not scrub memory itself, so it must run
 *    after memory init has done so - which is why it cannot share
 *    intel_txt_romstage_init()'s pre-raminit slot.
 *
 * Resets the platform on success. Returns, leaving the flag set, if the ACM
 * cannot be launched or if DRAM does not look scrubbed.
 */
void intel_txt_romstage_clear_secrets(void)
{
	if (!is_txt_cpu()) {
		printk(BIOS_ERR, "TEE-TXT: CPU not TXT capable.\n");
		return;
	}

	/* GETSEC[CAPABILITIES] and GETSEC[ENTERACCS] need CR4.SMXE. */
	write_cr4(read_cr4() | CR4_SMXE);

	if (!is_txt_chipset()) {
		printk(BIOS_ERR, "TEE-TXT: Chipset not TXT capable.\n");
		return;
	}

	if (read64p(TXT_SPAD) & ACMSTS_TXT_DISABLED) {
		printk(BIOS_INFO, "TEE-TXT: TXT disabled by BIOS policy in FIT.\n");
		return;
	}

	printk(BIOS_INFO, "TEE-TXT: Checking for secrets in memory...\n");

	if (!intel_txt_memory_has_secrets()) {
		printk(BIOS_INFO, "TEE-TXT: No secrets in memory\n");
		return;
	}

	/*
	 * No ACM can be launched with TXT.ESTS.WAKE_ERROR_STS set. Getting
	 * here with it set means intel_txt_romstage_init() could not run
	 * SCLEAN either, which is the only way out of that state.
	 */
	if (read8p(TXT_ESTS) & TXT_ESTS_WAKE_ERROR_STS) {
		printk(BIOS_ERR, "TEE-TXT: Fatal BIOS ACM error reported\n");
		return;
	}

	/*
	 * Only the BSP may call GETSEC[ENTERACCS], and the APs must be in
	 * wait-for-SIPI. Both hold in romstage, where the APs have not been
	 * started yet, so there is nothing to park.
	 */
	if (!boot_cpu()) {
		printk(BIOS_ERR, "TEE-TXT: BSP flag not set in APICBASE_MSR.\n");
		return;
	}

	if (!dram_was_cleared()) {
		printk(BIOS_ERR, "TEE-TXT: Memory init did not clear DRAM.\n");
		printk(BIOS_ERR, "TEE-TXT: Keeping the secrets flag set: it is what stops "
		       "the next boot from handing those secrets to the OS.\n");
		return;
	}

	printk(BIOS_INFO, "TEE-TXT: Wiping TEE...\n");

	/*
	 * Deliberately not intel_txt_prepare_txt_env(): it demands a locked
	 * IA32_FEATURE_CONTROL and resets the platform when it finds one that
	 * is not - and nothing locks it before CPU init in ramstage. Those
	 * checks guard the SCHECK/LOCK_CONFIG flows that run after FSP-S;
	 * ENTERACCS itself needs a TXT capable chipset, the BSP and CR4.SMXE,
	 * all checked above.
	 */
	intel_txt_run_clear_secrets();

	/* Only reached if the BIOS ACM could not be launched. */
	printk(BIOS_ERR, "TEE-TXT: Could not run the BIOS ACM to clear secrets\n");
	intel_txt_log_acm_error(read32p(TXT_BIOSACM_ERRORCODE));
}
