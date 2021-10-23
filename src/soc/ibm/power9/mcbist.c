/* SPDX-License-Identifier: GPL-2.0-only */

/* Debug is too slow here, hits timeouts */
#define SKIP_SCOM_DEBUG

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <timer.h>

#include "istep_13_scom.h"
#include "mcbist.h"

#define MCBIST_TESTS_PER_REG	4
/* 32 total, but last register is under non-consecutive SCOM address */
#define MAX_MCBIST_TESTS		28
#define MAX_MCBIST_TEST_REGS	(MAX_MCBIST_TESTS / MCBIST_TESTS_PER_REG)

/*
 * TODO: if we were to run both MCBISTs in parallel, we would need separate
 * instances of those...
 */
static uint64_t mcbist_memreg_cache;
static unsigned tests;

#define ECC_MODE	0x0008
#define DONE		0x0004

enum data_mode
{
	// MCBIST test data modes
	FIXED_DATA_MODE   = 0x0000,
	RAND_FWD_MODE     = 0x0010,
	RAND_REV_MODE     = 0x0020,
	RAND_FWD_MAINT    = 0x0030,
	RAND_REV_MAINT    = 0x0040,
	DATA_EQ_ADDR      = 0x0050,
	ROTATE_LEFT_MODE  = 0x0060,
	ROTATE_RIGHT_MODE = 0x0070,
};

enum op_type
{
	WRITE            = 0x0000, // fast, with no concurrent traffic
	READ             = 0x1000, // fast, with no concurrent traffic
	READ_WRITE       = 0x2000,
	WRITE_READ       = 0x3000,
	READ_WRITE_READ  = 0x4000,
	READ_WRITE_WRITE = 0x5000,
	RAND_SEQ         = 0x6000,
	READ_READ_WRITE  = 0x8000,
	SCRUB_RRWR       = 0x9000,
	STEER_RW         = 0xA000,
	ALTER            = 0xB000, // (W)
	DISPLAY          = 0xC000, // (R, slow)
	CCS_EXECUTE      = 0xF000,

	// if bits 9:11 (Data Mode bits)  = 000  (bits 4:8 used to specify which subtest to go to)
	// Refresh only cmd if bits 9:11 (Data Mode bits) /= 000
	GOTO_SUBTEST_N   = 0x7000,
};

static void commit_mcbist_memreg_cache(int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int reg = (tests - 1) / MCBIST_TESTS_PER_REG;

	if (reg < 0)
		die("commit_mcbist_memreg_cache() called without adding tests first!\n");

	if (reg >= MAX_MCBIST_TEST_REGS)
		die("Too many MCBIST instructions added\n");

	/* MC01.MCBIST.MBA_SCOMFIR.MCBMR<reg>Q */
	write_scom_for_chiplet(id, MCBMR0Q + reg, mcbist_memreg_cache);
	mcbist_memreg_cache = 0;
}

static void add_mcbist_test(int mcs_i, uint16_t test)
{
	int test_i = tests % MCBIST_TESTS_PER_REG;
	if (test_i == 0 && tests != 0)
		commit_mcbist_memreg_cache(mcs_i);

	/* This assumes cache is properly cleared. */
	mcbist_memreg_cache |= PPC_PLACE(test, test_i*16, 16);
	tests++;
}

/*
 * ECC Scrubbing - theory
 *
 * RAM cannot hold the data indefinitely. It uses capacitors to hold the bits,
 * which are constantly being drawn by leaks. To counteract this, memory has to
 * be periodically refreshed, which recharges the capacitors. However, sometimes
 * this happens too late, when state of capacitor has already changed (either
 * electric charge was depleted, or capacitor gained additional potential from
 * outside - rowhammer, radiation) up to the point where it passes the threshold
 * and 0 becomes 1 or vice versa. Refresh command in that case would only make
 * "borderline 1" into "strong 1", so it won't be able to fix the problem. This
 * is where ECC comes in.
 *
 * ECC is limited in number of changed bits it can fix and detect. Because of
 * that it is important that ECC is checked and possible errors are corrected
 * before too many bits have flipped, and corrected values are written back to
 * RAM. This is done by hardware, without software's interaction, but it can be
 * informed that ECC error has happened (machine check exception).
 *
 * ECC is checked every time data in RAM is accessed. To check every part of RAM
 * even when CPU doesn't need to read it, memory controller does the accesses in
 * the background. This is called ECC scrubbing.
 *
 * Note that it is enough for MC to just send read commands. When everything is
 * correct, data is still written back to DRAM because reading operation is
 * destructive - capacitors are discharged when read and have to be charged
 * again. This happens internally in DRAM, there is no need to send that data
 * through the memory bus when DRAM already has it. If there was an error, MC
 * automatically sends corrected data to be written.
 *
 * ECC scrubbing happens between RAM and MC. CPU doesn't participate in this
 * process, but it may be slowed down on memory intensive operations because
 * some of the bandwidth is used for scrubbing.
 *
 * TL;DR: ECC scrub is read operation with discarded results.
 */
void add_scrub(int mcs_i, int port_dimm)
{
	uint16_t test = READ | ECC_MODE | (port_dimm << 9);
	add_mcbist_test(mcs_i, test);
}

void add_fixed_pattern_write(int mcs_i, int port_dimm)
{
	/* Use ALTER instead of WRITE to use maintenance pattern. ALTER is slow. */
	uint16_t test = WRITE | FIXED_DATA_MODE | ECC_MODE | (port_dimm << 9);
	add_mcbist_test(mcs_i, test);
}

/*
static void add_random_pattern_write(int port_dimm)
{
	uint16_t test = WRITE | RAND_FWD_MAINT | ECC_MODE | (port_dimm << 9);
	add_mcbist_test(test);
}
*/

/* TODO: calculate initial delays and timeouts */
void mcbist_execute(int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/* This is index of last instruction, not the new one. */
	int test_i = (tests - 1) % MCBIST_TESTS_PER_REG;
	uint64_t val;

	/*
	 * Nothing to do. Note that status register won't report "done", or will
	 * report state of previous program instead. According to docs this bits
	 * are writable, do we want to set them to simplify things?
	 *
	 * Another possibility would be to start MCBIST with single no-op test (goto
	 * with DONE bit set), but this may unnecessarily make things slower.
	 */
	if (tests == 0)
		return;

	/* Check if in progress */
	/* TODO: we could force it to stop, but dying will help with debugging */
	if ((val = read_scom_for_chiplet(id, MCB_CNTLSTATQ)) & PPC_BIT(MCB_CNTLSTATQ_MCB_IP))
		die("MCBIST in progress already (%#16.16llx), this shouldn't happen\n", val);

	/*
	 * Contrary to CCS, we don't add no-op instruction here. DONE bit has to be
	 * set for instruction that is already present. Perhaps DONE is poor name,
	 * is tells that MCBIST should stop after this test, but this is how it is
	 * named in the documentation.
	 */
	mcbist_memreg_cache |= PPC_BIT(13 + test_i*16);
	commit_mcbist_memreg_cache(mcs_i);

	/* MC01.MCBIST.MBA_SCOMFIR.MCB_CNTLQ
	 * [0] MCB_CNTLQ_MCB_START
	 */
	scom_and_or_for_chiplet(id, MCB_CNTLQ, ~0, PPC_BIT(MCB_CNTLQ_MCB_START));

	/* Wait for MCBIST to start. Test for IP and DONE, it may finish early. */
	if (((val = read_scom_for_chiplet(id, MCB_CNTLSTATQ)) &
	     (PPC_BIT(MCB_CNTLSTATQ_MCB_IP) | PPC_BIT(MCB_CNTLSTATQ_MCB_DONE))) == 0) {
		/*
		 * TODO: how long do we want to wait? Hostboot uses 10*100us polling,
		 * but so far it seems to always be already started on the first read.
		 */
		udelay(1);
		if (((val = read_scom_for_chiplet(id, MCB_CNTLSTATQ)) &
		     (PPC_BIT(MCB_CNTLSTATQ_MCB_IP) | PPC_BIT(MCB_CNTLSTATQ_MCB_DONE))) == 0)
			die("MCBIST failed (%#16.16llx) to start twice\n", val);

		/* Check if this is needed. Do not move before test, it impacts delay! */
		printk(BIOS_INFO, "MCBIST started after delay\n");
	}

	tests = 0;
}

/*
 * FIXME: 0x07012300[10] MCBIST_PROGRAM_COMPLETE should be checked instead. It
 * gets set when MCBIST is paused, while 0x070123DC[0] IP stays on in that case.
 * This may become a problem for 3DS DIMMs.
 */
int mcbist_is_done(int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	uint64_t val = val = read_scom_for_chiplet(id, MCB_CNTLSTATQ);

	/* Still in progress */
	if (val & PPC_BIT(MCB_CNTLSTATQ_MCB_IP))
		return 0;

	/* Not sure if DONE and FAIL can be set at the same time, check FAIL first */
	if ((val & PPC_BIT(MCB_CNTLSTATQ_MCB_FAIL)) || val == 0)
		die("MCBIST error (%#16.16llx)\n");

	/* Finished */
	if (val & PPC_BIT(MCB_CNTLSTATQ_MCB_DONE))
		return 1;

	/* Is it even possible to get here? */
	return 0;
}
