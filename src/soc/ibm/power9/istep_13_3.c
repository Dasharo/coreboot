/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <timer.h>

#include "istep_13_scom.h"

#define RING_ID_1866	0x6B
#define RING_ID_2133	0x6C
#define RING_ID_2400	0x6D
#define RING_ID_2666	0x6E

/*
 * 13.3 mem_pll_initf: PLL Initfile for MBAs
 *
 * a) p9_mem_pll_initf.C (proc chip)
 *    - This step is a no-op on cumulus
 *    - This step is a no-op if memory is running in synchronous mode since the
 *      MCAs are using the nest PLL, HWP detect and exits
 *    - MCA PLL setup
 *      - Note that Hostboot doesn't support twiddling bits, Looks up which
 *        "bucket" (ring) to use from attributes set during mss_freq
 *      - Then request the SBE to scan ringId with setPulse
 *        - SBE needs to support 5 RS4 images
 *        - Data is stored as a ring image in the SBE that is frequency specific
 *        - 5 different frequencies (1866, 2133, 2400, 2667, EXP)
 */
void istep_13_3(void)
{
	printk(BIOS_EMERG, "starting istep 13.3\n");
	uint64_t ring_id;
	int mcs_i;

	report_istep(13,3);

	/* Assuming MC doesn't run in sync mode with Fabric, otherwise this is no-op */

	switch (mem_data.speed) {
		case 2666:
			ring_id = RING_ID_2666;
			break;
		case 2400:
			ring_id = RING_ID_2400;
			break;
		case 2133:
			ring_id = RING_ID_2133;
			break;
		case 1866:
			ring_id = RING_ID_1866;
			break;
		default:
			die("Unsupported memory speed (%d MT/s)\n", mem_data.speed);
	}

	/*
	 * This is the only place where Hostboot does `putRing()` on Nimbus, but
	 * because Hostboot tries to be as generic as possible, there are many tests
	 * and safeties in place. We do not have to worry about another threads or
	 * out of order command/response pair. Just fill a buffer, send it and make
	 * sure the receiver (SBE) gets it. If you still want to know the details,
	 * start digging here: https://github.com/open-power/hostboot/blob/master/src/usr/scan/scandd.C#L169
	 *
	 * TODO: this is the only place where `putRing()` is called, but it isn't
	 * the only place where PSU commands are used (see 16.1-16.2). Consider
	 * making a function from this.
	 */
	// TP.TPCHIP.PIB.PSU.PSU_SBE_DOORBELL_REG
	if (read_scom(PSU_SBE_DOORBELL_REG) & PPC_BIT(0))
		die("MBOX to SBE busy, this should not happen\n");


	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		long time;

		if (!mem_data.mcs[mcs_i].functional)
			continue;

		/* https://github.com/open-power/hostboot/blob/master/src/include/usr/sbeio/sbe_psudd.H#L418 */
		// TP.TPCHIP.PIB.PSU.PSU_HOST_SBE_MBOX0_REG
		/* REQUIRE_RESPONSE, PSU_PUT_RING_FROM_IMAGE_CMD, CMD_CONTROL_PUTRING */
		/*
		 * TODO: there is also a sequence ID (bits 32-47) which should be unique. It
		 * has a value of 9 at this point in Hostboot logs, meaning there were
		 * probably earlier messages to SBE. In that case, we may also need a static
		 * variable for it, which probably implies wrapping this into a function and
		 * moving it to separate file.
		 */
		write_scom(PSU_HOST_SBE_MBOX0_REG, 0x000001000000D301);

		// TP.TPCHIP.PIB.PSU.PSU_HOST_SBE_MBOX0_REG
		/* TARGET_TYPE_PERV, chiplet ID = 0x07, ring ID, RING_MODE_SET_PULSE_NSL */
		write_scom(PSU_HOST_SBE_MBOX1_REG, 0x0002000000000004 | PPC_SHIFT(ring_id, 47) |
		           PPC_SHIFT(mcs_ids[mcs_i], 31));

		// Ring the host->SBE doorbell
		// TP.TPCHIP.PIB.PSU.PSU_SBE_DOORBELL_REG_OR
		write_scom(PSU_SBE_DOORBELL_REG_WOR, PPC_BIT(0));

		// Wait for response
		/*
		 * Hostboot registers an interrupt handler in a thread that is demonized. We
		 * do not want nor need to implement a whole OS just for this purpose, we
		 * can just busy-wait here, there isn't anything better to do anyway.
		 *
		 * The original timeout is 90 seconds, but that seems like eternity. After
		 * thorough testing we probably should trim it.
		 */
		// TP.TPCHIP.PIB.PSU.PSU_HOST_DOORBELL_REG
		time = wait_ms(90 * MSECS_PER_SEC, read_scom(PSU_HOST_DOORBELL_REG) & PPC_BIT(0));

		if (!time)
			die("Timed out while waiting for SBE response\n");

		/* This may depend on the requested frequency, but for current setup in our
		 * lab this is ~3ms both for coreboot and Hostboot. */
		printk(BIOS_EMERG, "putRing took %ld ms\n", time);

		// Clear SBE->host doorbell
		// TP.TPCHIP.PIB.PSU.PSU_HOST_DOORBELL_REG_AND
		write_scom(PSU_HOST_DOORBELL_REG_WAND, ~PPC_BIT(0));
	}

	printk(BIOS_EMERG, "ending istep 13.3\n");
}
