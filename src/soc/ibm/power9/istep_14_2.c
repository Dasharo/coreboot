/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_14.h>
#include <cpu/power/istep_13.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>

#define MCS_MCMODE0 (0x5010811)
#define MCS_MCSYNC (0x5010815)
#define MCA_MBA_FARB3Q (0x7010916)

#define MCS_MCSYNC_SYNC_GO_CH0 (16)
#define SUPER_SYNC_BIT (14)
#define MBA_REFRESH_SYNC_BIT (8)
#define MCS_MCMODE0_DISABLE_MC_SYNC (27)
#define MCS_MCMODE0_DISABLE_MC_PAIR_SYNC (28)

static void thermal_init(uint8_t chip)
{
	for (size_t mcs_i = 0; mcs_i < MCS_PER_PROC; ++mcs_i) {
		for (size_t mca_i = 0; mca_i < MCA_PER_MCS; ++mca_i) {
			mca_and_or(chip, mcs_ids[mcs_i], mca_i, MCA_MBA_FARB3Q,
				   ~PPC_BITMASK(0, 45),
				   PPC_BIT(10) | PPC_BIT(25) | PPC_BIT(37));
		}
		scom_and_for_chiplet(chip, mcs_to_nest[mcs_ids[mcs_i]],
				     MCS_MCMODE0 + 0x80 * mcs_i,
				     PPC_BIT(21));
	}
}

static void prog_mc_mode0(uint8_t chip, chiplet_id_t nest_target, size_t index)
{
	uint64_t mask = PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
		      | PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
	uint64_t data = PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
		      | PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
	scom_and_or_for_chiplet(chip, nest_target, MCS_MCMODE0 + 0x80 * index, ~mask,
				data & mask);
}

static void throttle_sync(uint8_t chip)
{
	for (size_t mcs_i = 0; mcs_i < MCS_PER_PROC; ++mcs_i)
		prog_mc_mode0(chip, mcs_to_nest[mcs_ids[mcs_i]], mcs_i);
	scom_and_for_chiplet(chip, N3_CHIPLET_ID, MCS_MCSYNC,
			     ~PPC_BIT(MCS_MCSYNC_SYNC_GO_CH0));
	scom_and_or_for_chiplet(chip, N3_CHIPLET_ID, MCS_MCSYNC, ~PPC_BIT(SUPER_SYNC_BIT),
				PPC_BITMASK(0, 16));
	scom_and_for_chiplet(chip, N3_CHIPLET_ID, MCS_MCSYNC, ~PPC_BIT(MBA_REFRESH_SYNC_BIT));
}

void istep_14_2(uint8_t chips)
{
	uint8_t chip;

	report_istep(14, 2);
	printk(BIOS_EMERG, "starting istep 14.2\n");

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip)) {
			thermal_init(chip);
			throttle_sync(chip);
		}
	}

	printk(BIOS_EMERG, "ending istep 14.2\n");
}
