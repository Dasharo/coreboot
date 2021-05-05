/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>

#include "istep_13_scom.h"

/*
 * Reset memory controller configuration written by SBE.
 * Close the MCS acker before enabling the real memory bars.
 *
 * Some undocumented registers, again. The registers use a stride I haven't seen
 * before (0x80), not sure if those are MCSs (including those not present on P9),
 * magic MCAs or something totally different. Hostboot writes to all possible
 * registers, regardless of how many ports/slots are populated.
 *
 * All register and field names come from code and comments only, except for the
 * first one.
 */
static void revert_mc_hb_dcbz_config(void)
{
	int mcs_i, i;
	uint64_t val;
	const uint64_t mul = 0x80;

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		chiplet_id_t nest = mcs_to_nest[mcs_ids[mcs_i]];

		/*
		 * Bit for MCS2/3 is documented, but for MCS0/1 it is "unused". Use what
		 * Hostboot uses - bit 10 for MCS0/1 and bit 9 for MCS2/3.
		 */
		/* TP.TCNx.Nx.CPLT_CTRL1, x = {1,3} */
		val = read_scom_for_chiplet(nest, NEST_CPLT_CTRL1);
		if ((mcs_i == 0 && val & PPC_BIT(10)) ||
		    (mcs_i == 1 && val & PPC_BIT(9)))
			continue;

		for (i = 0; i < 2; i++) {
			/* MCFGP -- mark BAR invalid & reset grouping configuration fields
			MCS_n_MCFGP    // undocumented, 0x0501080A, 0x0501088A, 0x0301080A, 0x0301088A for MCS{0-3}
				[0]     VALID =                                 0
				[1-4]   MC_CHANNELS_PER_GROUP =                 0
				[5-7]   CHANNEL_0_GROUP_MEMBER_IDENTIFICATION = 0   // CHANNEL_1_GROUP_MEMBER_IDENTIFICATION not cleared?
				[13-23] GROUP_SIZE =                            0
			*/
			scom_and_or_for_chiplet(nest, 0x0501080A + i * mul,
			                        ~(PPC_BITMASK(0, 7) | PPC_BITMASK(13, 23)),
			                        0);

			/* MCMODE1 -- enable speculation, cmd bypass, fp command bypass
			MCS_n_MCMODE1  // undocumented, 0x05010812, 0x05010892, 0x03010812, 0x03010892
				[32]    DISABLE_ALL_SPEC_OPS =      0
				[33-51] DISABLE_SPEC_OP =           0x40    // bit 45 (called DCBF_BIT in code) set because of HW414958
				[54-60] DISABLE_COMMAND_BYPASS =    0
				[61]    DISABLE_FP_COMMAND_BYPASS = 0
			*/
			scom_and_or_for_chiplet(nest, 0x05010812 + i * mul,
			                        ~(PPC_BITMASK(32, 51) | PPC_BITMASK(54, 61)),
			                        PPC_SHIFT(0x40, 51));

			/* MCS_MCPERF1 -- enable fast path
			MCS_n_MCPERF1  // undocumented, 0x05010810, 0x05010890, 0x03010810, 0x03010890
				[0]     DISABLE_FASTPATH =  0
			*/
			scom_and_or_for_chiplet(nest, 0x05010810 + i * mul,
			                        ~PPC_BIT(0),
			                        0);

			/* Re-mask MCFIR. We want to ensure all MCSs are masked until the
			 * BARs are opened later during IPL.
			MCS_n_MCFIRMASK_OR  // undocumented, 0x05010805, 0x05010885, 0x03010805, 0x03010885
				[all]   1
			*/
			write_scom_for_chiplet(nest, 0x05010805 + i * mul, ~0);
		}
	}
}

/*
 * TODO: right now, every port is a separate group. This is easier to code, but
 * will impact performance due to no interleaving.
 *
 * Even though documentation (POWER9 Processor User's Manual) says that only the
 * total amount of memory behind an MCU has to be the same, Hostboot doesn't
 * group 1Rx4 with 2Rx8 (both have 16GB), at least if they are on the different
 * sides of CPU. Case when they are on the same side was not tested yet.
 *
 * If that means MCAs from different sides cannot be grouped, groups bigger than
 * 2 ports are not possible, at least for Talos.
 *
 * TODO2: note that this groups _ports_, not _DIMMs_. One implication is that
 * total amount of memory doesn't have to be a power of 2 (different densities).
 * Group sizes written to the register however are based on log2 of size. This
 * means that either there will be a hole or some RAM won't be mapped. We do not
 * have a way of testing it right now, all our DIMMs have 8Gb density.
 */
struct mc_group {
	/* Multiple MCAs can be in one group, but not the other way around. */
	uint8_t port_mask;
	/* Encoded, 4GB = 0, 8GB = 1, 16GB = 3, 32GB = 7 ... */
	uint8_t group_size;
};

/* Without proper documentation it's hard to tell if this is correct. */
/* The following array is MCS_MCFGP for MCA0 and MCS_MCFGPM for MCA1:
 *	MCS_MCFGP                                       // undocumented, 0x0501080A
 *	  [all]   0
 *	  [0]     VALID
 *	  [1-4]   MC_CHANNELS_PER_GROUP                           (*)
 *	  [5-7]   CHANNEL_0_GROUP_MEMBER_IDENTIFICATION           (*)
 *	  [8-10]  CHANNEL_1_GROUP_MEMBER_IDENTIFICATION           (*)
 *	  [13-23] GROUP_SIZE
 *	  [24-47] GROUP_BASE_ADDRESS
 *
 *	MCS_MCFGPM                                      // undocumented, 0x0501080C
 *	  [all]   0
 *	  [0]     VALID
 *	  [13-23] GROUP_SIZE
 *	  [24-47] GROUP_BASE_ADDRESS
 *
 * Fields marked with (*) are used only when there is more than 1 MCA in a group.
 */
static uint64_t mcfgp_regs[MCS_PER_PROC][MCA_PER_MCS];

/* Encodes size and keeps groups[] sorted. */
static void add_group(struct mc_group groups[MCA_PER_PROC], int size, uint8_t mask)
{
	int i;
	/*
	 * Size calculations are correct for size that is a power of 2. I have no
	 * idea what is the proper thing to do if it isn't.
	 */
	struct mc_group in = {mask, (size - 1) >> 2};

	if (size & (size - 1))
		die("Size of group %#2.2x (%d GB) is not a power of 2\n", mask, size);

	for (i = 0; i < MCA_PER_PROC; i++) {
		struct mc_group tmp = groups[i];

		if (tmp.group_size < in.group_size) {
			groups[i] = in;
			/* Shift the rest of elements */
			in = tmp;
		}

		/* Current element was empty */
		if (tmp.port_mask == 0)
			break;
	}

	if (in.port_mask != 0)
		die("Tried to add more groups than possible\n");
}

/* TODO: make groups with > 1 MCA possible */
static void fill_groups(void)
{
	int mcs_i, mca_i, i;
	struct mc_group groups[MCA_PER_PROC] = {0};
	/* This is in 4GB units, as expected by registers. */
	uint32_t cur_ba = 0;

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			/*
			 * Use the same format as in Hostboot, in case there can be more
			 * than 2 MCAs per MCS.
			 * mask = (MCS0/MCA0, MCS0/MCA1, 0, 0, MCS1/MCA0, MCS1/MCA1, 0, 0)
			 */
			uint8_t mask = PPC_BIT(mcs_i * 4 + mca_i) >> 56;
			/* Non-present DIMM will have a size of 0. */
			add_group(groups, mca->dimm[0].size_gb + mca->dimm[1].size_gb, mask);
		}
	}

	/* Now that all the groups are sorted by size, we can set base addresses. */
	for (i = 0; i < MCA_PER_PROC; i++) {
		uint8_t mask = groups[i].port_mask;
		if (mask == 0)
			break;

		/* A reminder for whoever implements this in add_group() but not here. */
		if (mask & (mask - 1))
			die("Multiple MCs in a group are not supported yet\n");

		/*
		 * Get MCS and MCA from mask, we expect bigger groups in the future. No
		 * else-ifs, bigger groups must set multiple registers (though that is
		 * not enough, there are also IDs to be set in MCS_MCFGP).
		 */
		if (mask & 0x80) {
			/* MCS = 0, MCA = 0 */
			mcfgp_regs[0][0] = PPC_BIT(0) | PPC_SHIFT(groups[i].group_size, 23) |
			                   PPC_SHIFT(cur_ba, 47);
		}
		if (mask & 0x40) {
			/* MCS = 0, MCA = 1 */
			mcfgp_regs[0][1] = PPC_BIT(0) | PPC_SHIFT(groups[i].group_size, 23) |
			                   PPC_SHIFT(cur_ba, 47);
		}
		if (mask & 0x08) {
			/* MCS = 1, MCA = 0 */
			mcfgp_regs[1][0] = PPC_BIT(0) | PPC_SHIFT(groups[i].group_size, 23) |
			                   PPC_SHIFT(cur_ba, 47);
		}
		if (mask & 0x04) {
			/* MCS = 1, MCA = 1 */
			mcfgp_regs[1][1] = PPC_BIT(0) | PPC_SHIFT(groups[i].group_size, 23) |
			                   PPC_SHIFT(cur_ba, 47);
		}

		cur_ba += groups[i].group_size + 1;
	}
	/*
	 * This would be a good place to check if we passed the start of PCIe MMIO
	 * range (2TB). In that case we probably should configure this memory hole
	 * somehow (MCFGPA?).
	 */
}

/*
 * This function is different than all previous FIR unmasking. It doesn't touch
 * Action0 register. It also doesn't modify Action1, it just writes the value
 * discarding the old one. As these registers are not documented, I can't even
 * tell whether it sets checkstop, recoverable error or something else.
 */
static void fir_unmask(int mcs_i)
{
	chiplet_id_t nest = mcs_to_nest[mcs_ids[mcs_i]];
	/* Stride discovered by trial and error due to lack of documentation. */
	uint64_t mul = 0x80;

	/* MCS_MCFIRACT1                   // undocumented, 0x05010807
		[all]   0
		[0]     MC_INTERNAL_RECOVERABLE_ERROR = 1
		[8]     COMMAND_LIST_TIMEOUT =          1
	*/
	write_scom_for_chiplet(nest, 0x05010807 + mcs_i * mul,
	                       PPC_BIT(0) | PPC_BIT(8));

	/* MCS_MCFIRMASK (AND)             // undocumented, 0x05010804
		[all]   1
		[0]     MC_INTERNAL_RECOVERABLE_ERROR =     0
		[1]     MC_INTERNAL_NONRECOVERABLE_ERROR =  0
		[2]     POWERBUS_PROTOCOL_ERROR =           0
		[4]     MULTIPLE_BAR =                      0
		[5]     INVALID_ADDRESS =                   0
		[8]     COMMAND_LIST_TIMEOUT =              0
	*/
	write_scom_for_chiplet(nest, 0x05010804 + mcs_i * mul,
	                       ~(PPC_BIT(0) | PPC_BIT(1) | PPC_BIT(2) |
	                         PPC_BIT(4) | PPC_BIT(5) | PPC_BIT(8)));
}

static void mcd_fir_mask(void)
{
	/* These are set always for N1 chiplet only. */
	write_scom_for_chiplet(N1_CHIPLET_ID, MCD1_FIR_MASK_REG, ~0);
	write_scom_for_chiplet(N1_CHIPLET_ID, MCD0_FIR_MASK_REG, ~0);
}

/*
 * 14.5 proc_setup_bars: Setup Memory BARs
 *
 * a) p9_mss_setup_bars.C (proc chip) -- Nimbus
 * b) p9c_mss_setup_bars.C (proc chip) -- Cumulus
 *    - Same HWP interface for both Nimbus and Cumulus, input target is
 *      TARGET_TYPE_PROC_CHIP; HWP is to figure out if target is a Nimbus (MCS)
 *      or Cumulus (MI) internally.
 *    - Prior to setting the memory bars on each processor chip, this procedure
 *      needs to set the centaur security protection bit
 *      - TCM_CHIP_PROTECTION_EN_DC is SCOM Addr 0x03030000
 *      - TCN_CHIP_PROTECTION_EN_DC is SCOM Addr 0x02030000
 *      - Both must be set to protect Nest and Mem domains
 *    - Based on system memory map
 *      - Each MCS has its mirroring and non mirrored BARs
 *      - Set the correct checkerboard configs. Note that chip flushes to
 *        checkerboard
 *      - need to disable memory bar on slave otherwise base flush values will
 *        ack all memory accesses
 * c) p9_setup_bars.C
 *    - Sets up Powerbus/MCD, L3 BARs on running core
 *      - Other cores are setup via winkle images
 *    - Setup dSMP and PCIe Bars
 *      - Setup PCIe outbound BARS (doing stores/loads from host core)
 *        - Addresses that PCIE responds to on powerbus (PCI init 1-7)
 *      - Informing PCIe of the memory map (inbound)
 *        - PCI Init 8-15
 *    - Set up Powerbus Epsilon settings
 *      - Code is still running out of L3 cache
 *      - Use this procedure to setup runtime epsilon values
 *      - Must be done before memory is viable
 */
void istep_14_5(void)
{
	int mcs_i;
	printk(BIOS_EMERG, "starting istep 14.5\n");
	report_istep(14,5);

	/* Start MCS reset */
	revert_mc_hb_dcbz_config();

	fill_groups();

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		chiplet_id_t nest = mcs_to_nest[mcs_ids[mcs_i]];

		if (!mem_data.mcs[mcs_i].functional)
			continue;

		fir_unmask(mcs_i);

		/*
		 * More undocumented registers. First two are described before
		 * 'mcfgp_regs', last two are for setting up memory hole and SMF, they
		 * are unused now.
		 */
		write_scom_for_chiplet(nest, 0x0501080A, mcfgp_regs[mcs_i][0]);
		write_scom_for_chiplet(nest, 0x0501080C, mcfgp_regs[mcs_i][1]);
		write_scom_for_chiplet(nest, 0x0501080B, 0);
		write_scom_for_chiplet(nest, 0x0501080D, 0);
	}

	mcd_fir_mask();

	printk(BIOS_EMERG, "ending istep 14.5\n");
}
