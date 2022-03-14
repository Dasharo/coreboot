/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <types.h>
#include <soc/meminit.h>
#include <soc/romstage.h>

/* DDR4 specific swizzling data start */

/* Channel 0 PHY 0 to DUnit DQ mapping */
static const uint8_t swizzling_ch0_ddr4[] = {
	 1,  0,  5,  7,  2,  3,  6,  4, 24, 29, 28, 26, 25, 30, 31, 27,
	19, 18, 23, 22, 17, 16, 21, 20,  8,  9, 12, 13, 10, 15, 11, 14,
};

/* Channel 1 PHY 0 to DUnit DQ mapping */
static const uint8_t swizzling_ch1_ddr4[] = {
	 5,  6,  0,  2,  4,  7,  3,  1, 12, 13, 10, 11, 15, 14,  9,  8,
	21, 22, 18, 16, 23, 19, 17, 20, 28, 30, 27, 26, 29, 24, 31, 25,
};

/* DDR4 specific swizzling data end*/

static void fill_ddr4_params(FSP_M_CONFIG *cfg)
{
	cfg->Ch0_DeviceWidth = 0x00; /* Only for memor down */
	cfg->Ch0_DramDensity = 0x00; /* Only for memor down */
	cfg->Ch0_Mode2N = 0x00; /* Only for DDR3L */
	cfg->Ch0_OdtConfig = 0;
	cfg->Ch0_OdtLevels = 0; /* ODT connected to SoC */
	/* bit0 Rank Select Interleaving Enable ,
	   bit1 Bank Address Hashing enabled */
	cfg->Ch0_Option = 0x03;
	/* bit0 enable rank 0,
	 * bit1 enable rank 1.
	 * This is allowed maximum, it may be trimmed down by FSP based on SPD data.
	 */
	cfg->Ch0_RankEnable = 0x03;
	cfg->Ch0_TristateClk1 = 0x00;

	cfg->Ch1_DeviceWidth = 0x00;
	cfg->Ch1_DramDensity = 0x00;
	cfg->Ch1_Mode2N = 0x00;
	cfg->Ch1_OdtConfig = 0;
	cfg->Ch1_OdtLevels = 0;
	cfg->Ch1_Option = 0x03;
	cfg->Ch1_RankEnable = 0x03;
	cfg->Ch1_TristateClk1 = 0x00;

	cfg->Ch2_DeviceWidth = 0x00;
	cfg->Ch2_DramDensity = 0x00;
	cfg->Ch2_Mode2N = 0x00;
	cfg->Ch2_OdtConfig = 0;
	cfg->Ch2_OdtLevels = 0;
	cfg->Ch2_Option = 0x03;
	cfg->Ch2_RankEnable = 0x03;
	cfg->Ch2_TristateClk1 = 0x00;

	cfg->Ch3_DramDensity = 0x00;
	cfg->Ch3_Mode2N = 0x00;
	cfg->Ch3_OdtConfig = 0;
	cfg->Ch3_OdtLevels = 0;
	cfg->Ch3_Option = 0x03;
	cfg->Ch3_RankEnable = 0x03;
	cfg->Ch3_TristateClk1 = 0x00;

	cfg->ChannelHashMask = 0x00;
	cfg->ChannelsSlicesEnable = 0x00;
	cfg->DDR3LASR = 0x00; /* Only for memory down */
	cfg->DDR3LPageSize = 0x00; /* Only for memory down */
	cfg->DIMM0SPDAddress = 0xA0;
	cfg->DIMM1SPDAddress = 0xA4;
	cfg->DisableFastBoot = 0x00; /* Use saved training data if valid */
	cfg->DualRankSupportEnable = 0x01;
	cfg->eMMCTraceLen = 0x0;
	cfg->EnhancePort8xhDecoding = 1;
	cfg->HighMemoryMaxValue = 0x00;
	cfg->Igd = 0x01;
	cfg->IgdApertureSize = 0x1; /* 128MB */
	cfg->IgdDvmt50PreAlloc = 0x02; /* 64MB */
	cfg->GttSize = 0x3; /* 8MB */
	cfg->InterleavedMode = 0x02; /* Enable = 0x2? */
	cfg->LowMemoryMaxValue = 0x0000;
	cfg->MemoryDown = 0x0;
	cfg->MemorySizeLimit = 0x0000;
	cfg->MinRefRate2xEnable = 0x01;
	cfg->MrcDataSaving = 0x01;
	cfg->MrcFastBoot = 0x01;
	cfg->MsgLevelMask = 0x00000000;
	/* Specifies CA Mapping for all technologies. 
	 * 0 - SODIMM(Default)
	 * 1 - BGA
	 * 2 - BGA mirrored (LPDDR3 only)
	 * 3 - SODIMM/UDIMM with Rank 1 Mirrored (DDR3L)
	 */
	cfg->Package = 0x00;
	cfg->PreMemGpioTableEntryNum = 0;
	cfg->PreMemGpioTablePinNum[0] = 0;
	cfg->PreMemGpioTablePinNum[1] = 0;
	cfg->PreMemGpioTablePinNum[2] = 0;
	cfg->PreMemGpioTablePinNum[3] = 0;
	cfg->PreMemGpioTablePtr = 0x00000000;
	/*
	 * FSP headers say: 0x0:AUTO, 0x2:IGD, 0x3:PCI, but the Geminilake FSP
	 * code says: 0x0:IGD, 0x1:PCI
	 */
	cfg->PrimaryVideoAdaptor = 0x0;
	/* 
	 * Profiles:
	 * 0x01:LPDDR3_1333_10_12_12,
	 * 0x02:LPDDR3_1600_12_15_15,
	 * 0x03:LPDDR3_1866_14_17_17,
	 * 0x04:LPDDR4_1600_14_15_15,
	 * 0x05:LPDDR4_1866_20_17_17,
	 * 0x06:LPDDR4_2133_20_20_20,
	 * 0x07:LPDDR4_2400_24_22_22,
	 * 0x08:LPDDR4_2666_24_24_24,
	 * 0x09:LPDDR4_3200_28_29_29,
	 * 0x0A:DDR4_1600_10_10_10,
	 * 0x0B:DDR4_1600_11_11_11, 
	 * 0x0C:DDR4_1600_12_12_12,
	 * 0x0D:DDR4_1866_12_12_12,
	 * 0x0E:DDR4_1866_13_13_13,
	 * 0x0F:DDR4_1866_14_14_14,
	 * 0x10:DDR4_2133_14_14_14, 
	 * 0x11:DDR4_2133_15_15_15,
	 * 0x12:DDR4_2133_16_16_16, 
	 * 0x13:DDR4_2400_15_15_15, 
	 * 0x14:DDR4_2400_16_16_16, 
	 * 0x15:DDR4_2400_17_17_17 (default),
	 * 0x16:DDR4_2400_18_18_18,
	 * 0x17:DDR4_2666_17_17_17, 
	 * 0x18:DDR4_2666_18_18_18,
	 * 0x19:DDR4_2666_19_19_19,
	 * 0x1A:DDR4_2666_20_20_20
	 */
	cfg->Profile = 0x15;
	cfg->OemLoadingBase = 0;
	/* Tolerance percentage o failing margins */
	cfg->RmtMarginCheckScaleHighThreshold = 0x0000; 
	cfg->RmtMode = 0x00;
	cfg->ScramblerSupport = 0x01;
	cfg->SerialDebugPortAddress = 0x000003f8;
	cfg->SerialDebugPortDevice = 0x03; /* External device */
	cfg->SerialDebugPortStrideSize = 0x00; /* Stride 1 byte */
	cfg->SerialDebugPortType = 0x01; /* I/O */
	cfg->SliceHashMask = 0x00;
	cfg->SpdWriteEnable = 0x00;
	cfg->StartTimerTickerOfPfetAssert = 0x4E20;

	/* phy0 ch0 */
	memcpy(cfg->Ch0_Bit_swizzling, swizzling_ch0_ddr4, sizeof(swizzling_ch0_ddr4));
	/* phy0 ch1 */
	memcpy(cfg->Ch1_Bit_swizzling, swizzling_ch1_ddr4, sizeof(swizzling_ch1_ddr4));
	/* phy1 ch1 */
	memset(cfg->Ch2_Bit_swizzling, 0, sizeof(cfg->Ch2_Bit_swizzling));
	/* phy1 ch0 */
	memset(cfg->Ch3_Bit_swizzling, 0, sizeof(cfg->Ch3_Bit_swizzling));
}

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	FSP_M_CONFIG *cfg = &memupd->FspmConfig;
	fill_ddr4_params(cfg);
}

void mainboard_save_dimm_info(void)
{
	save_ddr4_dimm_info();
}
