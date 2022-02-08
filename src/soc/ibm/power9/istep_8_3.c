/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <stdint.h>
#include <console/console.h>
#include <cpu/power/istep_8.h>
#include <cpu/power/proc.h>
#include <delay.h>

#include "fsi.h"

/* As long as we're not using SBE FIFO, we might not actually need this, but
 * it's trivial to implement */
static void send_fifo_reset(uint8_t chip)
{
	enum { SBE_FIFO_DNFIFO_RESET = 0x00002450 };

	/* Perform a write to the DNFIFO Reset to cleanup the FIFO */
	write_fsi(chip, SBE_FIFO_DNFIFO_RESET, 0xDEAD);
}

static void start_cbs(uint8_t chip)
{
	enum {
		PERV_SB_MSG_FSI = 0x00002809,

		PERV_CBS_CS_FSI = 0x00002801,
		PERV_CBS_CS_START_BOOT_SEQUENCER = 0,
		PERV_CBS_CS_OPTION_SKIP_SCAN0_CLOCKSTART = 2,
		PERV_CBS_CS_OPTION_PREVENT_SBE_START = 3,

		PERV_SB_CS_FSI = 0x00002808,
		PERV_SB_CS_START_RESTART_VECTOR0 = 12,
		PERV_SB_CS_START_RESTART_VECTOR1 = 13,

		PERV_CBS_ENVSTAT_FSI = 0x00002804,
		PERV_CBS_ENVSTAT_C4_VDN_GPOOD = 2,

		/* Observed Number of times CBS read for CBS_INTERNAL_STATE_VECTOR */
		P9_CFAM_CBS_POLL_COUNT = 20,
		/* unit is micro seconds [min : 64k x (1/100MHz) = 64k x 10(-8) = 640 us
		 *                        max : 64k x (1/50MHz) = 128k x 10(-8) = 1280 us]
		 */
		P9_CBS_IDLE_HW_US_DELAY = 640,

		CBS_IDLE_VALUE = 0x0002,

		PERV_FSB_FSB_DOWNFIFO_RESET_FSI = 0x00002414,
		FIFO_RESET = 0x80000000,

		PERV_FSI2PIB_STATUS_FSI = 0x00001007,
		PERV_FSI2PIB_STATUS_VDD_NEST_OBSERVE = 16,
	};

	int poll_count;
	uint64_t tmp;

	/* Clear Selfboot message register before every boot */
	write_cfam(chip, PERV_SB_MSG_FSI, 0);

	/* Configure Prevent SBE start option */
	tmp = PPC_SHIFT(read_cfam(chip, PERV_CBS_CS_FSI), 31);
	tmp |= PPC_BIT(PERV_CBS_CS_OPTION_PREVENT_SBE_START);
	write_cfam(chip, PERV_CBS_CS_FSI, tmp >> 32);

	/* Setup hreset to 0 */
	tmp = PPC_SHIFT(read_cfam(chip, PERV_SB_CS_FSI), 31);
	tmp &= ~PPC_BIT(PERV_SB_CS_START_RESTART_VECTOR0);
	tmp &= ~PPC_BIT(PERV_SB_CS_START_RESTART_VECTOR1);
	write_cfam(chip, PERV_SB_CS_FSI, tmp >> 32);

	/* Check for VDN_PGOOD */
	tmp = PPC_SHIFT(read_cfam(chip, PERV_CBS_ENVSTAT_FSI), 31);
	if (!(tmp & PPC_BIT(PERV_CBS_ENVSTAT_C4_VDN_GPOOD)))
		die("CBS startup: VDN_PGOOD is OFF, can't proceed\n");

	/* Reset CFAM Boot Sequencer (CBS) to flush value */
	tmp = PPC_SHIFT(read_cfam(chip, PERV_CBS_CS_FSI), 31);
	tmp &= ~PPC_BIT(PERV_CBS_CS_START_BOOT_SEQUENCER);
	tmp &= ~PPC_BIT(PERV_CBS_CS_OPTION_SKIP_SCAN0_CLOCKSTART);
	write_cfam(chip, PERV_CBS_CS_FSI, tmp >> 32);

	/* Trigger CFAM Boot Sequencer (CBS) to start (no read, we know register's contents) */
	tmp |= PPC_BIT(PERV_CBS_CS_START_BOOT_SEQUENCER);
	write_cfam(chip, PERV_CBS_CS_FSI, tmp >> 32);

	for (poll_count = 0; poll_count < P9_CFAM_CBS_POLL_COUNT; poll_count++) {
		/*
		 * PERV_CBS_CS_INTERNAL_STATE_VECTOR_START = 16
		 * PERV_CBS_CS_INTERNAL_STATE_VECTOR_LEN = 16
		 */
		uint16_t cbs_state = (read_cfam(chip, PERV_CBS_CS_FSI) & 0xFF);
		if (cbs_state == CBS_IDLE_VALUE)
			break;

		udelay(P9_CBS_IDLE_HW_US_DELAY);
	}

	if (poll_count == P9_CFAM_CBS_POLL_COUNT)
		die("CBS startup: CBS has not reached idle state!\n");

	/* Reset FIFO (ATTR_START_CBS_FIFO_RESET_SKIP is set only for some specific test) */
	write_cfam(chip, PERV_FSB_FSB_DOWNFIFO_RESET_FSI, FIFO_RESET);

	/* Setup up hreset (clear -> set -> clear again) */
	tmp = PPC_SHIFT(read_cfam(chip, PERV_SB_CS_FSI), 31);
	tmp &= ~PPC_BIT(PERV_SB_CS_START_RESTART_VECTOR0);
	write_cfam(chip, PERV_SB_CS_FSI, tmp >> 32);
	tmp |= PPC_BIT(PERV_SB_CS_START_RESTART_VECTOR0);
	write_cfam(chip, PERV_SB_CS_FSI, tmp >> 32);
	tmp &= ~PPC_BIT(PERV_SB_CS_START_RESTART_VECTOR0);
	write_cfam(chip, PERV_SB_CS_FSI, tmp >> 32);

	/* Check for VDD status */
	tmp = PPC_SHIFT(read_cfam(chip, PERV_FSI2PIB_STATUS_FSI), 31);
	if (!(tmp & PPC_BIT(PERV_FSI2PIB_STATUS_VDD_NEST_OBSERVE)))
		die("CBS startup: VDD is OFF!\n");
}

void istep_8_3(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 8.3\n");
	report_istep(8,3);

	/* Skipping master chip */
	for (uint8_t chip = 1; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip)) {
			/*
			 * Before starting the CBS (and thus the SBE) on slave
			 * procs make sure the SBE FIFO is clean by doing a full
			 * reset of the FIFO
			 */
			send_fifo_reset(chip);
			start_cbs(chip);
		}
	}

	printk(BIOS_EMERG, "ending istep 8.3\n");
}
