/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_10.h>

#include <console/console.h>
#include <cpu/power/istep_13.h>
#include <cpu/power/powerbus.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>

static void mcs_scom(uint8_t chip, chiplet_id_t chiplet)
{
	uint64_t data;

	{
		data = read_scom_for_chiplet(chip, chiplet, 0x5010810);

		PPC_INSERT(data, 25, 32, 7);
		PPC_INSERT(data, 0x7, 46, 4);
		PPC_INSERT(data, 0xF, 55, 6);
		/* MC01_PBI01_SCOMFIR_MCPERF1_ENABLE_PF_DROP_CMDLIST_ON */
		data |= PPC_BIT(61);
		PPC_INSERT(data, 0x0, 62, 1);
		/* MC01_PBI01_SCOMFIR_MCPERF1_ENABLE_PREFETCH_PROMOTE_ON */
		data |= PPC_BIT(63);

		write_scom_for_chiplet(chip, chiplet, 0x5010810, data);
	}

	{
		data = read_scom_for_chiplet(chip, chiplet, 0x5010811);

		/* MC01_PBI01_SCOMFIR_MCMODE0_ENABLE_CENTAUR_SYNC_ON */
		data |= PPC_BIT(20);
		/* MC01_PBI01_SCOMFIR_MCMODE0_ENABLE_64_128B_READ_ON */
		data |= PPC_BIT(9);
		/* MC01_PBI01_SCOMFIR_MCMODE0_ENABLE_DROP_FP_DYN64_ACTIVE_ON */
		data |= PPC_BIT(8);
		/* MC01_PBI01_SCOMFIR_MCMODE0_CENTAURP_ENABLE_ECRESP_OFF */
		data &= ~PPC_BIT(7);
		/* MC01_PBI01_SCOMFIR_MCMODE0_DISABLE_MC_SYNC_ON */
		data |= PPC_BIT(27);
		/* MC01_PBI01_SCOMFIR_MCMODE0_DISABLE_MC_PAIR_SYNC_ON */
		data |= PPC_BIT(28);
		/* MC01_PBI01_SCOMFIR_MCMODE0_FORCE_COMMANDLIST_VALID_ON */
		data |= PPC_BIT(17);

		write_scom_for_chiplet(chip, chiplet, 0x5010811, data);
	}
	{
		data = read_scom_for_chiplet(chip, chiplet, 0x5010812);

		/* MC01_PBI01_SCOMFIR_MCMODE1_DISABLE_FP_M_BIT_ON */
		data |= PPC_BIT(10);
		PPC_INSERT(data, 0x40, 33, 19);

		write_scom_for_chiplet(chip, chiplet, 0x5010812, data);
	}
	{
		data = read_scom_for_chiplet(chip, chiplet, 0x5010813);
		PPC_INSERT(data, 0x8, 24, 16);
		write_scom_for_chiplet(chip, chiplet, 0x5010813, data);
	}
	{
		data = read_scom_for_chiplet(chip, chiplet, 0x501081B);

		/* MC01_PBI01_SCOMFIR_MCTO_SELECT_PB_HANG_PULSE_ON */
		data |= PPC_BIT(0);
		/* MC01_PBI01_SCOMFIR_MCTO_SELECT_LOCAL_HANG_PULSE_OFF */
		data &= ~PPC_BIT(1);
		/* MC01_PBI01_SCOMFIR_MCTO_ENABLE_NONMIRROR_HANG_ON */
		data |= PPC_BIT(32);
		/* MC01_PBI01_SCOMFIR_MCTO_ENABLE_APO_HANG_ON */
		data |= PPC_BIT(34);
		PPC_INSERT(data, 0x1, 2, 2);
		PPC_INSERT(data, 0x1, 24, 8);
		PPC_INSERT(data, 0x7, 5, 3);

		write_scom_for_chiplet(chip, chiplet, 0x501081B, data);
	}
}

static void fbc_ioo_tl_scom(uint8_t chip)
{
	uint64_t data;

	/* PB_IOO_SCOM_A0_MODE_BLOCKED */
	scom_or(chip, 0x501380A, PPC_BIT(20) | PPC_BIT(25) | PPC_BIT(52) | PPC_BIT(57));

	/* PB_IOO_SCOM_A1_MODE_BLOCKED */
	scom_or(chip, 0x501380B, PPC_BIT(20) | PPC_BIT(25) | PPC_BIT(52) | PPC_BIT(57));

	/* PB_IOO_SCOM_A2_MODE_BLOCKED */
	scom_or(chip, 0x501380C, PPC_BIT(20) | PPC_BIT(25) | PPC_BIT(52) | PPC_BIT(57));

	/* PB_IOO_SCOM_A3_MODE_BLOCKED */
	scom_or(chip, 0x501380D, PPC_BIT(20) | PPC_BIT(25) | PPC_BIT(52) | PPC_BIT(57));

	/* 0x5013810, 0x5013811, 0x5013812 and 0x5013813 are not modified */

	data = read_scom(chip, 0x5013823);

	data &= ~PPC_BIT(0);  // PB_IOO_SCOM_PB_CFG_IOO01_IS_LOGICAL_PAIR_OFF
	data &= ~PPC_BIT(1);  // PB_IOO_SCOM_PB_CFG_IOO23_IS_LOGICAL_PAIR_OFF
	data &= ~PPC_BIT(2);  // PB_IOO_SCOM_PB_CFG_IOO45_IS_LOGICAL_PAIR_OFF
	data &= ~PPC_BIT(3);  // PB_IOO_SCOM_PB_CFG_IOO67_IS_LOGICAL_PAIR_OFF
	data &= ~PPC_BIT(8);  // PB_IOO_SCOM_LINKS01_TOD_ENABLE_OFF
	data &= ~PPC_BIT(9);  // PB_IOO_SCOM_LINKS23_TOD_ENABLE_OFF
	data &= ~PPC_BIT(10); // PB_IOO_SCOM_LINKS45_TOD_ENABLE_OFF
	data &= ~PPC_BIT(11); // PB_IOO_SCOM_LINKS67_TOD_ENABLE_OFF

	write_scom(chip, 0x5013823, data);

	/* 0x5013824 is not modified */
}

static void nx_scom(uint8_t chip, uint8_t dd)
{
	uint64_t data;

	{
		data = read_scom(chip, 0x2011041);

		data |= PPC_BIT(63); // NX_DMA_CH0_EFT_ENABLE_ON
		data |= PPC_BIT(62); // NX_DMA_CH1_EFT_ENABLE_ON
		data |= PPC_BIT(58); // NX_DMA_CH2_SYM_ENABLE_ON
		data |= PPC_BIT(57); // NX_DMA_CH3_SYM_ENABLE_ON
		data |= PPC_BIT(61); // NX_DMA_CH4_GZIP_ENABLE_ON

		write_scom(chip, 0x2011041, data);
	}
	{
		data = read_scom(chip, 0x2011042);

		PPC_INSERT(data, 0xF, 8, 4);  // NX_DMA_GZIPCOMP_MAX_INRD_MAX_15_INRD
		PPC_INSERT(data, 0xF, 12, 4); // NX_DMA_GZIPDECOMP_MAX_INRD_MAX_15_INRD
		PPC_INSERT(data, 0x3, 25, 4); // NX_DMA_SYM_MAX_INRD_MAX_3_INRD
		PPC_INSERT(data, 0xF, 33, 4); // NX_DMA_EFTCOMP_MAX_INRD_MAX_15_INRD = 0xf;
		PPC_INSERT(data, 0xF, 37, 4); // NX_DMA_EFTDECOMP_MAX_INRD_MAX_15_INRD

		data |= PPC_BIT(23);  // NX_DMA_EFT_COMP_PREFETCH_ENABLE_ON
		data |= PPC_BIT(24);  // NX_DMA_EFT_DECOMP_PREFETCH_ENABLE_ON
		data |= PPC_BIT(16);  // NX_DMA_GZIP_COMP_PREFETCH_ENABLE_ON
		data |= PPC_BIT(17);  // NX_DMA_GZIP_DECOMP_PREFETCH_ENABLE_ON
		data &= ~PPC_BIT(56); // NX_DMA_EFT_SPBC_WRITE_ENABLE_OFF

		write_scom(chip, 0x2011042, data);
	}
	{
		data = read_scom(chip, 0x201105C);

		PPC_INSERT(data, 0x9, 1, 4);  // NX_DMA_CH0_WATCHDOG_REF_DIV_DIVIDE_BY_512
		PPC_INSERT(data, 0x9, 6, 4);  // NX_DMA_CH1_WATCHDOG_REF_DIV_DIVIDE_BY_512
		PPC_INSERT(data, 0x9, 11, 4); // NX_DMA_CH2_WATCHDOG_REF_DIV_DIVIDE_BY_512
		PPC_INSERT(data, 0x9, 16, 4); // NX_DMA_CH3_WATCHDOG_REF_DIV_DIVIDE_BY_512
		PPC_INSERT(data, 0x9, 21, 4); // NX_DMA_CH4_WATCHDOG_REF_DIV_DIVIDE_BY_512
		PPC_INSERT(data, 0x8, 26, 4); // NX_DMA_DMA_HANG_TIMER_REF_DIV_DIVIDE_BY_1024

		data |= PPC_BIT(0);  // NX_DMA_CH0_WATCHDOG_TIMER_ENBL_ON
		data |= PPC_BIT(5);  // NX_DMA_CH1_WATCHDOG_TIMER_ENBL_ON
		data |= PPC_BIT(10); // NX_DMA_CH2_WATCHDOG_TIMER_ENBL_ON
		data |= PPC_BIT(15); // NX_DMA_CH3_WATCHDOG_TIMER_ENBL_ON
		data |= PPC_BIT(20); // NX_DMA_CH4_WATCHDOG_TIMER_ENBL_ON
		data |= PPC_BIT(25); // NX_DMA_DMA_HANG_TIMER_ENBL_ON

		write_scom(chip, 0x201105C, data);
	}
	{
		data = read_scom(chip, 0x2011087);

		data &= ~0x93EFDFFF3FF00000;
		data |= 0x48102000C0000000;

		if (dd == 0x20)
			data &= ~0x2400000000000000;
		else
			data |= 0x2400000000000000;

		write_scom(chip, 0x2011087, data);
	}
	{
		data = read_scom(chip, 0x2011095);

		data |= PPC_BIT(24);  // NX_PBI_CQ_WRAP_NXCQ_SCOM_SKIP_G_ON
		data |= PPC_BIT(1);   // NX_PBI_CQ_WRAP_NXCQ_SCOM_DMA_WR_DISABLE_GROUP_ON
		data |= PPC_BIT(5);   // NX_PBI_CQ_WRAP_NXCQ_SCOM_DMA_RD_DISABLE_GROUP_ON
		data |= PPC_BIT(9);   // NX_PBI_CQ_WRAP_NXCQ_SCOM_UMAC_WR_DISABLE_GROUP_ON
		data |= PPC_BIT(13);  // NX_PBI_CQ_WRAP_NXCQ_SCOM_UMAC_RD_DISABLE_GROUP_ON
		data |= PPC_BIT(2);   // NX_PBI_CQ_WRAP_NXCQ_SCOM_DMA_WR_DISABLE_VG_NOT_SYS_ON
		data |= PPC_BIT(6);   // NX_PBI_CQ_WRAP_NXCQ_SCOM_DMA_RD_DISABLE_VG_NOT_SYS_ON
		data |= PPC_BIT(10);  // NX_PBI_CQ_WRAP_NXCQ_SCOM_UMAC_WR_DISABLE_VG_NOT_SYS_ON
		data |= PPC_BIT(14);  // NX_PBI_CQ_WRAP_NXCQ_SCOM_UMAC_RD_DISABLE_VG_NOT_SYS_ON
		data |= PPC_BIT(22);  // NX_PBI_CQ_WRAP_NXCQ_SCOM_RD_GO_M_QOS_ON
		data &= ~PPC_BIT(23); // NX_PBI_CQ_WRAP_NXCQ_SCOM_ADDR_BAR_MODE_OFF

		PPC_INSERT(data, 0x0, 56, 4); // TGT1_ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID
		PPC_INSERT(data, 0x0, 60, 3); // TGT1_ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID
		PPC_INSERT(data, 0x1, 25, 2);
		PPC_INSERT(data, 0xFC, 40, 8);
		PPC_INSERT(data, 0xFC, 48, 8);

		write_scom(chip, 0x2011095, data);
	}
	{
		data = read_scom(chip, 0x20110D6);

		PPC_INSERT(data, 0x2, 9, 3);
		data |= PPC_BIT(6); // NX_PBI_DISABLE_PROMOTE_ON

		write_scom(chip, 0x20110D6, data);
	}
	{
		data = read_scom(chip, 0x2011107);

		data &= ~0xF0839FFFC2FFC000;
		data |= 0x0A7400003D000000;

		if (dd == 0x20)
			data &= ~0x0508600000000000;
		else
			data |= 0x0508600000000000;

		write_scom(chip, 0x2011107, data);
	}

	scom_and_or(chip, 0x2011083, ~0xEEF8FF9CFD000000, 0x1107006302F00000);
	scom_and(chip, 0x2011086, ~0xFFFFFFFFFFF00000);
	scom_and_or(chip, 0x20110A8, ~0x0FFFF00000000000, 0x0888800000000000);
	scom_and_or(chip, 0x20110C3, ~0x0000001F00000000, 0x0000000080000000);
	scom_and_or(chip, 0x20110C4, ~PPC_BITMASK(27, 35), PPC_PLACE(0x8, 27, 9));
	scom_and_or(chip, 0x20110C5, ~PPC_BITMASK(27, 35), PPC_PLACE(0x8, 27, 9));
	scom_or(chip, 0x20110D5, PPC_BIT(1)); // NX_PBI_PBI_UMAC_CRB_READS_ENBL_ON
	scom_and_or(chip, 0x2011103, ~0xCF7DEF81BF003000, 0x3082107E40FFC000);
	scom_and(chip, 0x2011106, ~0xFFFFFFFFFFFFC000);
}

static void cxa_scom(uint8_t chip, uint8_t dd)
{
	uint64_t data;

	data = read_scom(chip, 0x2010803);
	data &= ~PPC_BITMASK(0, 52);
	data |= (dd == 0x20 ? 0x801B1F98C8717000 : 0x801B1F98D8717000);
	write_scom(chip, 0x2010803, data);

	data = read_scom(chip, 0x2010818);
	data &= ~PPC_BIT(1);          // CAPP0_CXA_TOP_CXA_APC0_APCCTL_ADR_BAR_MODE_OFF
	data |= PPC_BIT(6);           // CAPP0_CXA_TOP_CXA_APC0_APCCTL_SKIP_G_ON
	data &= ~PPC_BITMASK(21, 24); // ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID
	data &= ~PPC_BITMASK(25, 27); // ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID
	data |= PPC_BIT(4);           // CAPP0_CXA_TOP_CXA_APC0_APCCTL_DISABLE_G_ON
	data |= PPC_BIT(3);           // CAPP0_CXA_TOP_CXA_APC0_APCCTL_DISABLE_VG_NOT_SYS_ON
	write_scom(chip, 0x2010818, data);

	scom_and(chip, 0x2010806, ~PPC_BITMASK(0, 52));
	scom_or(chip, 0x2010807, PPC_BIT(2) | PPC_BIT(8) | PPC_BIT(34) | PPC_BIT(44));
	scom_and(chip, 0x2010819, ~PPC_BITMASK(4, 7));
	scom_and_or(chip, 0x201081B,
		    ~PPC_BITMASK(45, 51), PPC_PLACE(0x7, 45, 3) | PPC_PLACE(0x2, 48, 4));
	scom_and_or(chip, 0x201081C, ~PPC_BITMASK(18, 21), PPC_PLACE(0x1, 18, 4));
}

static void int_scom(uint8_t chip, uint8_t dd)
{
	/*
	 * [0] = 0
	 * [1] = 1
	 * [5-8]  ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID
	 * [9-11] ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID
	 */
	scom_and_or(chip, 0x501300A, ~(PPC_BITMASK(0, 1) | PPC_BITMASK(5, 11)), PPC_BIT(1));

	scom_or(chip, 0x5013021,
		PPC_BIT(46) | // INT_CQ_PBO_CTL_DISABLE_VG_NOT_SYS_ON
		PPC_BIT(47) | // INT_CQ_PBO_CTL_DISABLE_G_ON
		PPC_BIT(49));

	if (dd <= 0x20)
		write_scom(chip, 0x5013033, 0x2000005C040281C3);
	else
		write_scom(chip, 0x5013033, 0x0000005C040081C3);

	write_scom(chip, 0x5013036, 0);
	write_scom(chip, 0x5013037, 0x9554021F80110E0C);

	scom_and_or(chip, 0x5013130,
		    ~(PPC_BITMASK(2, 7) | PPC_BITMASK(10, 15)),
		    PPC_PLACE(0x18, 2, 6) | PPC_PLACE(0x18, 10, 6));

	write_scom(chip, 0x5013140, 0x050043EF00100020);
	write_scom(chip, 0x5013141, 0xFADFBB8CFFAFFFD7);
	write_scom(chip, 0x5013178, 0x0002000610000000);

	scom_and_or(chip, 0x501320E, ~PPC_BITMASK(0, 47), PPC_PLACE(0x626222024216, 0, 48));
	scom_and_or(chip, 0x5013214, ~PPC_BITMASK(16, 31), PPC_PLACE(0x5BBF, 16, 16));
	scom_and_or(chip, 0x501322B, ~PPC_BITMASK(58, 63), PPC_PLACE(0x18, 58, 6));

	if (dd == 0x20) {
		scom_and_or(chip, 0x5013272,
			    ~PPC_BITMASK(0, 43), PPC_PLACE(0x0002C018006, 0, 44));
		scom_and_or(chip, 0x5013273,
			    ~PPC_BITMASK(0, 43), PPC_PLACE(0xFFFCFFEFFFA, 0, 44));
	}
}

static void vas_scom(uint8_t chip, uint8_t dd)
{
	uint64_t data;

	scom_and_or(chip, 0x3011803, ~PPC_BITMASK(0, 53), 0x00210102540D7C00);
	scom_and(chip, 0x3011806, ~PPC_BITMASK(0, 53));

	data = read_scom(chip, 0x3011807);
	data &= ~PPC_BITMASK(0, 53);
	data |= (dd == 0x20 ? 0x00DD020180000000 : 0x00DF020180000000);
	write_scom(chip, 0x3011807, data);

	/*
	 * [0-3] ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID
	 * [4-6] ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID
	 */
	scom_and(chip, 0x301184D, ~PPC_BITMASK(0, 6));

	data = read_scom(chip, 0x301184E);
	data &= ~PPC_BIT(13); // SOUTH_VA_EG_SCF_ADDR_BAR_MODE_OFF
	data |= PPC_BIT(14);  // SOUTH_VA_EG_SCF_SKIP_G_ON
	data |= PPC_BIT(1);   // SOUTH_VA_EG_SCF_DISABLE_G_WR_ON
	data |= PPC_BIT(5);   // SOUTH_VA_EG_SCF_DISABLE_G_RD_ON
	data |= PPC_BIT(2);   // SOUTH_VA_EG_SCF_DISABLE_VG_WR_ON
	data |= PPC_BIT(6);   // SOUTH_VA_EG_SCF_DISABLE_VG_RD_ON
	PPC_INSERT(data, 0xFC, 20, 8);
	PPC_INSERT(data, 0xFC, 28, 8);
	write_scom(chip, 0x301184E, data);

	if (dd == 0x20)
		scom_or(chip, 0x301184F, PPC_BIT(0));
}

static void chiplet_scominit(uint8_t chip, uint8_t dd)
{
	enum {
		PU_PB_CENT_SM0_PB_CENT_FIR_REG = 0x05011C00,
		PU_PB_CENT_SM0_PB_CENT_FIR_MASK_REG_SPARE_13 = 13,

		PU_PB_IOE_FIR_MASK_REG_OR = 0x05013405,
		PU_PB_CENT_SM1_EXTFIR_MASK_REG_OR = 0x05011C33,

		FBC_IOE_TL_FIR_MASK_X0_NF = 0x00C00C0C00000880,
		FBC_IOE_TL_FIR_MASK_X2_NF = 0x000300C0C0000220,
		FBC_EXT_FIR_MASK_X0_NF    = 0x8000000000000000,
		FBC_EXT_FIR_MASK_X1_NF    = 0x4000000000000000,
		FBC_EXT_FIR_MASK_X2_NF    = 0x2000000000000000,

		PU_NMMU_MM_EPSILON_COUNTER_VALUE = 0x5012C1D,
	};

	const struct powerbus_cfg *pb_cfg = powerbus_cfg(chip);

	int mcs_i;

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++)
		mcs_scom(chip, mcs_to_nest[mcs_ids[mcs_i]]);

	/*
	 * Read spare FBC FIR bit -- if set, SBE has configured XBUS FIR resources for all
	 * present units, and code here will be run to mask resources associated with
	 * non-functional units.
	 */
	if (read_scom(chip, PU_PB_CENT_SM0_PB_CENT_FIR_REG) &
	    PPC_BIT(PU_PB_CENT_SM0_PB_CENT_FIR_MASK_REG_SPARE_13)) {
		/* Masking XBUS FIR resources for unused links */

		/* XBUS0 FBC TL */
		write_scom(chip, PU_PB_IOE_FIR_MASK_REG_OR, FBC_IOE_TL_FIR_MASK_X0_NF);
		/* XBUS0 EXTFIR */
		write_scom(chip, PU_PB_CENT_SM1_EXTFIR_MASK_REG_OR, FBC_EXT_FIR_MASK_X0_NF);

		/* XBUS2 FBC TL */
		write_scom(chip, PU_PB_IOE_FIR_MASK_REG_OR, FBC_IOE_TL_FIR_MASK_X2_NF);
		/* XBUS2 EXTFIR */
		write_scom(chip, PU_PB_CENT_SM1_EXTFIR_MASK_REG_OR, FBC_EXT_FIR_MASK_X2_NF);
	}

	fbc_ioo_tl_scom(chip);
	nx_scom(chip, dd);
	cxa_scom(chip, dd); // CAPP
	int_scom(chip, dd);
	vas_scom(chip, dd);

	/* Setup NMMU epsilon write cycles */
	scom_and_or(chip, PU_NMMU_MM_EPSILON_COUNTER_VALUE,
		    ~(PPC_BITMASK(0, 11) | PPC_BITMASK(16, 27)),
		    PPC_PLACE(pb_cfg->eps_w[0], 0, 12) | PPC_PLACE(pb_cfg->eps_w[1], 16, 12));
}

static void psi_scom(uint8_t chip)
{
	scom_or(chip, 0x4011803, PPC_BITMASK(0, 6));
	scom_and(chip, 0x4011806, ~PPC_BITMASK(0, 6));
	scom_and(chip, 0x4011807, ~PPC_BITMASK(0, 6));

	scom_and_or(chip, 0x5012903, ~PPC_BITMASK(0, 28), PPC_PLACE(0x7E040DF, 0, 29));
	scom_and_or(chip, 0x5012906, ~PPC_BITMASK(0, 28), PPC_PLACE(0x0, 0, 29));
	scom_and_or(chip, 0x5012907, ~PPC_BITMASK(0, 28), PPC_PLACE(0x18050020, 0, 29));

	scom_and(chip, 0x501290F, ~(PPC_BITMASK(16, 27) | PPC_BITMASK(48, 52)));
}

void istep_10_6(uint8_t chips)
{
	uint8_t dd = get_dd(); // XXX: this should probably be chip-specific

	printk(BIOS_EMERG, "starting istep 10.6\n");
	report_istep(10,6);

	for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip)) {
			chiplet_scominit(chip, dd);
			psi_scom(chip);
		}
	}

	printk(BIOS_EMERG, "ending istep 10.6\n");
}
