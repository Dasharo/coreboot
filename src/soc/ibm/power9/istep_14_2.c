#include <cpu/power/istep_14.h>
#include <cpu/power/istep_13.h>

static void thermalInit(void)
{
	for (size_t MCSIndex = 0; MCSIndex < MCS_PER_PROC; ++MCSIndex) {
		for (size_t MCAIndex = 0; MCAIndex < MCA_PER_MCS; ++MCAIndex) {
				mca_and_or(
					mcs_ids[MCSIndex], MCAIndex, MCA_MBA_FARB3Q,
					~PPC_BITMASK(0, 45),
					PPC_BIT(10) | PPC_BIT(25) | PPC_BIT(37));
		}
		scom_and_for_chiplet(mcs_to_nest[mcs_ids[MCSIndex]], MCS_MCMODE0 + 0x80 * MCSIndex, PPC_BIT(21));
	}
}

static void progMCMode0(chiplet_id_t nestTarget, size_t index)
{
	uint64_t l_scomMask =
		PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
		| PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
	uint64_t l_scomData =
		PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
		| PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
	scom_and_or_for_chiplet(
		nestTarget, MCS_MCMODE0 + 0x80 * index,
		~l_scomMask, l_scomData & l_scomMask);
}

static void throttleSync(void)
{
	for(size_t MCSIndex = 0; MCSIndex < MCS_PER_PROC; ++MCSIndex)
	{
		progMCMode0(mcs_to_nest[mcs_ids[MCSIndex]], MCSIndex);
	}
	scom_and_for_chiplet(N3_CHIPLET_ID, MCS_MCSYNC, ~PPC_BIT(MCS_MCSYNC_SYNC_GO_CH0));
	scom_and_or_for_chiplet(N3_CHIPLET_ID, MCS_MCSYNC, ~PPC_BIT(SUPER_SYNC_BIT), PPC_BITMASK(0, 16));
	scom_and_for_chiplet(N3_CHIPLET_ID, MCS_MCSYNC, ~PPC_BIT(MBA_REFRESH_SYNC_BIT));
}

void istep_14_2(void)
{
	report_istep(14, 2);
	thermalInit();
	throttleSync();
}
