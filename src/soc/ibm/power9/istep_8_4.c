/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <stdint.h>
#include <console/console.h>
#include <cpu/power/istep_8.h>
#include <cpu/power/proc.h>
#include <delay.h>

#include "fsi.h"

/*
 * 8.4 proc_check_slave_sbe_seeprom_complete : Check Slave SBE Complete
 *
 * - Check to make sure that the slave SBE engines have completed their IPL
 * - FW will poll for up to 1 second to see if the "done" signature is in
 *   the status reg (not tied to istep number)
 * - If "done" signature is not found then FW must extract FFDC from the SBE
 */

static bool sbe_run_extract_msg_reg(uint8_t chip)
{
	enum {
		PERV_SB_MSG_FSI = 0x00002809,

		/* SBE is in its operational (runtime) state */
		SBE_STATE_RUNTIME = 0x4,

		/*
		 * Much higher frequency of polling buys us about 333ms here.
		 * Can also wait with second precision at first (4 seconds) as SBE boots in
		 * 4.7s every time.
		 */
		SBE_RETRY_TIMEOUT_HW_MS = 60 * 1000,
		SBE_RETRY_NUM_LOOPS = 60 * 100, // 100 times per second
	};

	/* Each sbe gets 60s to respond with the fact that it's booted and at
	 * runtime (stable state). */
	uint64_t SBE_WAIT_SLEEP_MS = (SBE_RETRY_TIMEOUT_HW_MS / SBE_RETRY_NUM_LOOPS);

	/*
	 * Layout of the register:
	 *     [0] = SBE control loop initialized
	 *     [1] = async FFDC present on SBE
	 *   [2-3] = reserved
	 *   [4-7] = previous SBE state
	 *  [8-11] = current SBE state
	 * [12-19] = last major istep executed by the SBE
	 * [20-25] = last minor istep executed by the SBE
	 * [26-31] = reserved
	 */
	uint32_t msg_reg;

	for (uint64_t i = 0; i < SBE_RETRY_NUM_LOOPS; i++) {
		uint8_t curr_state;

		msg_reg = read_cfam(chip, PERV_SB_MSG_FSI);

		curr_state = (msg_reg >> 20) & 0xF;
		if (curr_state == SBE_STATE_RUNTIME)
			return true;

		/* Check async FFDC bit (indicates SBE is failing to boot) */
		if (msg_reg & (1 << 30))
			break;

		if ((i * SBE_WAIT_SLEEP_MS) % 1000 == 0)
			printk(BIOS_EMERG, "SBE for chip #%d is booting...\n", chip);

		/* Hostboot resets watchdog before sleeping, we might want to
		   do it too or just increase timer after experimenting */
		mdelay(SBE_WAIT_SLEEP_MS);
	}

	/* We reach this line only if something is wrong with SBE */

	printk(BIOS_ERR, "Message register: 0x%08x\n", msg_reg);

	if (msg_reg & (1 << 30))
		printk(BIOS_ERR, "SBE reports an error.\n");
	else
		printk(BIOS_ERR, "SBE takes too long to boot.\n");

	printk(BIOS_ERR, "SBE for chip #%d failed to boot!\n", chip);

	/* If SBE did boot (started its control loop) and then failed, can read
	   some debug information from it (p9_extract_sbe_rc() in Hostboot) */

	/*
	 * Might want to restart SBE here if boot failure is something that can
	 * happen under normal circumstances. Hostboot gives current SBE side
	 * two tries, switches sides and gives up if it also fails twice.
	 */

	return false;
}

void istep_8_4(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 8.4\n");
	report_istep(8, 4);

	/* Skipping master chip */
	for (uint8_t chip = 1; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip)) {
			if (!sbe_run_extract_msg_reg(chip))
				die("SBE for chip #%d did not boot properly.\n", chip);
		}
	}

	printk(BIOS_EMERG, "ending istep 8.4\n");
}
