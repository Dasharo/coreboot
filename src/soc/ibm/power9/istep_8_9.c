/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_8.h>

#include <console/console.h>
#include <cpu/power/powerbus.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>
#include <stdbool.h>

#include "xbus.h"

/*
 * This code omits initialization of OBus which isn't present. It also assumes
 * there is only one XBus (X1). Both of these statements are true for Nimbus
 * Sforza.
 *
 * For consistency with Hostboot some read values are unused, written
 * unmodified or ANDed with 0, this simplifies verification that the code
 * operates correctly by comparing against Hostboot logs.
 */

static void p9_fbc_no_hp_scom(bool is_xbus_active, uint8_t chip)
{
	enum {
		/* Power Bus PB West Mode Configuration Register */
		PB_WEST_MODE             = 0x501180A,
		/* Power Bus PB CENT Mode Register */
		PB_CENT_MODE             = 0x5011C0A,
		/* Power Bus PB CENT GP command RATE DP0 Register */
		PB_CENT_GP_CMD_RATE_DP0  = 0x5011C26,
		/* Power Bus PB CENT GP command RATE DP1 Register */
		PB_CENT_GP_CMD_RATE_DP1  = 0x5011C27,
		/* Power Bus PB CENT RGP command RATE DP0 Register */
		PB_CENT_RGP_CMD_RATE_DP0 = 0x5011C28,
		/* Power Bus PB CENT RGP command RATE DP1 Register */
		PB_CENT_RGP_CMD_RATE_DP1 = 0x5011C29,
		/* Power Bus PB CENT SP command RATE DP0 Register */
		PB_CENT_SP_CMD_RATE_DP0  = 0x5011C2A,
		/* Power Bus PB CENT SP command RATE DP1 Register */
		PB_CENT_SP_CMD_RATE_DP1  = 0x5011C2B,
		/* Power Bus PB East Mode Configuration Register */
		PB_EAST_MODE             = 0x501200A,

		PB_CFG_CHIP_IS_SYSTEM = 4,

		PB_CFG_SP_HW_MARK     = 16,
		PB_CFG_SP_HW_MARK_LEN = 7,

		PB_CFG_GP_HW_MARK     = 23,
		PB_CFG_GP_HW_MARK_LEN = 7,

		PB_CFG_LCL_HW_MARK     = 30,
		PB_CFG_LCL_HW_MARK_LEN = 6,
	};

	/*
	 * ATTR_PROC_FABRIC_X_LINKS_CNFG
	 * Number of active XBus links: 1 for two CPUs, 0 for one CPU.
	 */
	const int num_x_links_cfg = (is_xbus_active ? 1 : 0);

	uint64_t pb_west_mode, pb_cent_mode, pb_east_mode;
	uint64_t pb_cent_rgp_cmd_rate_dp0, pb_cent_rgp_cmd_rate_dp1;
	uint64_t pb_cent_sp_cmd_rate_dp0, pb_cent_sp_cmd_rate_dp1;

	pb_west_mode = get_scom(chip, PB_WEST_MODE);
	PPC_INSERT(pb_west_mode, (num_x_links_cfg == 0), PB_CFG_CHIP_IS_SYSTEM, 1);
	PPC_INSERT(pb_west_mode, 0x3F, PB_CFG_SP_HW_MARK, PB_CFG_SP_HW_MARK_LEN);
	PPC_INSERT(pb_west_mode, 0x3F, PB_CFG_GP_HW_MARK, PB_CFG_GP_HW_MARK_LEN);
	PPC_INSERT(pb_west_mode, 0x2A, PB_CFG_LCL_HW_MARK, PB_CFG_LCL_HW_MARK_LEN);
	put_scom(chip, PB_WEST_MODE, pb_west_mode);

	pb_cent_mode = get_scom(chip, PB_CENT_MODE);
	PPC_INSERT(pb_cent_mode, (num_x_links_cfg == 0), PB_CFG_CHIP_IS_SYSTEM, 1);
	PPC_INSERT(pb_cent_mode, 0x3F, PB_CFG_SP_HW_MARK, PB_CFG_SP_HW_MARK_LEN);
	PPC_INSERT(pb_cent_mode, 0x3F, PB_CFG_GP_HW_MARK, PB_CFG_GP_HW_MARK_LEN);
	PPC_INSERT(pb_cent_mode, 0x2A, PB_CFG_LCL_HW_MARK, PB_CFG_LCL_HW_MARK_LEN);
	put_scom(chip, PB_CENT_MODE, pb_cent_mode);

	put_scom(chip, PB_CENT_GP_CMD_RATE_DP0, get_scom(chip, PB_CENT_GP_CMD_RATE_DP0) & 0);
	put_scom(chip, PB_CENT_GP_CMD_RATE_DP1, get_scom(chip, PB_CENT_GP_CMD_RATE_DP1) & 0);

	(void)get_scom(chip, PB_CENT_RGP_CMD_RATE_DP0);
	pb_cent_rgp_cmd_rate_dp0 = (num_x_links_cfg == 0 ? 0 : 0x030406080A0C1218);
	put_scom(chip, PB_CENT_RGP_CMD_RATE_DP0, pb_cent_rgp_cmd_rate_dp0);

	(void)get_scom(chip, PB_CENT_RGP_CMD_RATE_DP1);
	pb_cent_rgp_cmd_rate_dp1 = (num_x_links_cfg == 0 ? 0 : 0x040508080A0C1218);
	put_scom(chip, PB_CENT_RGP_CMD_RATE_DP1, pb_cent_rgp_cmd_rate_dp1);

	pb_cent_sp_cmd_rate_dp0 = get_scom(chip, PB_CENT_SP_CMD_RATE_DP0);
	pb_cent_sp_cmd_rate_dp0 = (num_x_links_cfg == 0 ? 0 : 0x030406080A0C1218);
	put_scom(chip, PB_CENT_SP_CMD_RATE_DP0, pb_cent_sp_cmd_rate_dp0);

	pb_cent_sp_cmd_rate_dp1 = get_scom(chip, PB_CENT_SP_CMD_RATE_DP1);
	pb_cent_sp_cmd_rate_dp1 = (num_x_links_cfg == 0 ? 0 : 0x030406080A0C1218);
	put_scom(chip, PB_CENT_SP_CMD_RATE_DP1, pb_cent_sp_cmd_rate_dp1);

	pb_east_mode = get_scom(chip, PB_EAST_MODE);
	PPC_INSERT(pb_east_mode, (num_x_links_cfg == 0), PB_CFG_CHIP_IS_SYSTEM, 1);
	PPC_INSERT(pb_east_mode, 0x3F, PB_CFG_SP_HW_MARK, PB_CFG_SP_HW_MARK_LEN);
	PPC_INSERT(pb_east_mode, 0x3F, PB_CFG_GP_HW_MARK, PB_CFG_GP_HW_MARK_LEN);
	PPC_INSERT(pb_east_mode, 0x2A, PB_CFG_LCL_HW_MARK, PB_CFG_LCL_HW_MARK_LEN);
	put_scom(chip, PB_EAST_MODE, pb_east_mode);
}

static void p9_fbc_ioe_tl_scom(bool is_xbus_active, uint8_t chip)
{
	enum {
		/* Processor bus Electrical Framer/Parser 01 configuration register */
		PB_FP01_CFG              = 0x501340A,
		/* Power Bus Electrical Framer/Parser 23 Configuration Register */
		PB_FP23_CFG              = 0x501340B,
		/* Power Bus Electrical Framer/Parser 45 Configuration Register */
		PB_FP45_CFG              = 0x501340C,
		/* Power Bus Electrical Link Data Buffer 01 Configuration Register */
		PB_ELINK_DATA_01_CFG_REG = 0x5013410,
		/* Power Bus Electrical Link Data Buffer 23 Configuration Register */
		PB_ELINK_DATA_23_CFG_REG = 0x5013411,
		/* Power Bus Electrical Link Data Buffer 45 Configuration Register */
		PB_ELINK_DATA_45_CFG_REG = 0x5013412,
		/* Power Bus Electrical Miscellaneous Configuration Register */
		PB_MISC_CFG              = 0x5013423,
		/* Power Bus Electrical Link Trace Configuration Register */
		PB_TRACE_CFG             = 0x5013424,

		FP0_FMR_DISABLE = 20,
		FP0_PRS_DISABLE = 25,
		FP1_FMR_DISABLE = 52,
		FP1_PRS_DISABLE = 57,

		FP2_FMR_DISABLE = 20,
		FP2_PRS_DISABLE = 25,
		FP3_FMR_DISABLE = 52,
		FP3_PRS_DISABLE = 57,

		FP4_FMR_DISABLE = 20,
		FP4_PRS_DISABLE = 25,
		FP5_FMR_DISABLE = 52,
		FP5_PRS_DISABLE = 57,

		IOE01_IS_LOGICAL_PAIR = 0,
		IOE23_IS_LOGICAL_PAIR = 1,
		IOE45_IS_LOGICAL_PAIR = 2,
	};

	/*
	 * According to schematics we only support one XBus with
	 *     ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG = { false, true, false }
	 * Meaning that X1 is present and X0 and X2 aren't.
	 */

	const uint64_t pb_freq_mhz = powerbus_cfg()->fabric_freq;

	const uint64_t dd2_lo_limit_d = (FREQ_X_MHZ * 10);
	const uint64_t dd2_lo_limit_n = pb_freq_mhz * 82;

	uint64_t pb_fp01_cfg, pb_fp23_cfg, pb_fp45_cfg;
	uint64_t pb_elink_data_23_cfg_reg;
	uint64_t pb_misc_cfg, pb_trace_cfg;

	pb_fp01_cfg = get_scom(chip, PB_FP01_CFG);
	pb_fp01_cfg |= PPC_BIT(FP0_FMR_DISABLE);
	pb_fp01_cfg |= PPC_BIT(FP0_PRS_DISABLE);
	pb_fp01_cfg |= PPC_BIT(FP1_FMR_DISABLE);
	pb_fp01_cfg |= PPC_BIT(FP1_PRS_DISABLE);
	put_scom(chip, PB_FP01_CFG, pb_fp01_cfg);

	pb_fp23_cfg = get_scom(chip, PB_FP23_CFG);

	PPC_INSERT(pb_fp23_cfg, !is_xbus_active, FP2_FMR_DISABLE, 1);
	PPC_INSERT(pb_fp23_cfg, !is_xbus_active, FP2_PRS_DISABLE, 1);
	PPC_INSERT(pb_fp23_cfg, !is_xbus_active, FP3_FMR_DISABLE, 1);
	PPC_INSERT(pb_fp23_cfg, !is_xbus_active, FP3_PRS_DISABLE, 1);

	if (is_xbus_active) {
		PPC_INSERT(pb_fp23_cfg, 0x01, 22, 2);
		PPC_INSERT(pb_fp23_cfg, 0x20, 12, 8);
		PPC_INSERT(pb_fp23_cfg, 0x15 - (dd2_lo_limit_n / dd2_lo_limit_d), 4, 8);
		PPC_INSERT(pb_fp23_cfg, 0x20, 44, 8);
		PPC_INSERT(pb_fp23_cfg, 0x15 - (dd2_lo_limit_n / dd2_lo_limit_d), 36, 8);
	}

	put_scom(chip, PB_FP23_CFG, pb_fp23_cfg);

	pb_fp45_cfg = get_scom(chip, PB_FP45_CFG);
	pb_fp45_cfg |= PPC_BIT(FP4_FMR_DISABLE);
	pb_fp45_cfg |= PPC_BIT(FP4_PRS_DISABLE);
	pb_fp45_cfg |= PPC_BIT(FP5_FMR_DISABLE);
	pb_fp45_cfg |= PPC_BIT(FP5_PRS_DISABLE);
	put_scom(chip, PB_FP45_CFG, pb_fp45_cfg);

	put_scom(chip, PB_ELINK_DATA_01_CFG_REG, get_scom(chip, PB_ELINK_DATA_01_CFG_REG));

	pb_elink_data_23_cfg_reg = get_scom(chip, PB_ELINK_DATA_23_CFG_REG);
	if (is_xbus_active) {
		PPC_INSERT(pb_elink_data_23_cfg_reg, 0x1F, 24, 5);
		PPC_INSERT(pb_elink_data_23_cfg_reg, 0x40, 1, 7);
		PPC_INSERT(pb_elink_data_23_cfg_reg, 0x40, 33, 7);
		PPC_INSERT(pb_elink_data_23_cfg_reg, 0x3C, 9, 7);
		PPC_INSERT(pb_elink_data_23_cfg_reg, 0x3C, 41, 7);
		PPC_INSERT(pb_elink_data_23_cfg_reg, 0x3C, 17, 7);
		PPC_INSERT(pb_elink_data_23_cfg_reg, 0x3C, 49, 7);
	}
	put_scom(chip, PB_ELINK_DATA_23_CFG_REG, pb_elink_data_23_cfg_reg);

	put_scom(chip, PB_ELINK_DATA_45_CFG_REG, get_scom(chip, PB_ELINK_DATA_45_CFG_REG));

	pb_misc_cfg = get_scom(chip, PB_MISC_CFG);
	PPC_INSERT(pb_misc_cfg, 0x00, IOE01_IS_LOGICAL_PAIR, 1);
	PPC_INSERT(pb_misc_cfg, is_xbus_active, IOE23_IS_LOGICAL_PAIR, 1);
	PPC_INSERT(pb_misc_cfg, 0x00, IOE45_IS_LOGICAL_PAIR, 1);
	put_scom(chip, PB_MISC_CFG, pb_misc_cfg);

	pb_trace_cfg = get_scom(chip, PB_TRACE_CFG);
	if (is_xbus_active) {
		PPC_INSERT(pb_trace_cfg, 0x4, 16, 4);
		PPC_INSERT(pb_trace_cfg, 0x4, 24, 4);
		PPC_INSERT(pb_trace_cfg, 0x1, 20, 4);
		PPC_INSERT(pb_trace_cfg, 0x1, 28, 4);
	}
	put_scom(chip, PB_TRACE_CFG, pb_trace_cfg);
}

static void p9_fbc_ioe_dl_scom(uint8_t chip)
{
	enum {
		/* ELL Configuration Register */
		IOEL_CONFIG           = 0x601180A,
		/* ELL Replay Threshold Register */
		IOEL_REPLAY_THRESHOLD = 0x6011818,
		/* ELL SL ECC Threshold Register */
		IOEL_SL_ECC_THRESHOLD = 0x6011819,

		LL1_CONFIG_LINK_PAIR     = 0,
		LL1_CONFIG_CRC_LANE_ID   = 2,
		LL1_CONFIG_SL_UE_CRC_ERR = 4,
	};

	/* ATTR_LINK_TRAIN == fapi2::ENUM_ATTR_LINK_TRAIN_BOTH (from logs) */

	uint64_t ioel_config, ioel_replay_threshold, ioel_sl_ecc_threshold;

	ioel_config = get_scom(chip, IOEL_CONFIG);
	ioel_config |= PPC_BIT(LL1_CONFIG_LINK_PAIR);
	ioel_config |= PPC_BIT(LL1_CONFIG_CRC_LANE_ID);
	ioel_config |= PPC_BIT(LL1_CONFIG_SL_UE_CRC_ERR);
	PPC_INSERT(ioel_config, 0xF, 11, 5);
	PPC_INSERT(ioel_config, 0xF, 28, 4);
	put_scom(chip, IOEL_CONFIG, ioel_config);

	ioel_replay_threshold = get_scom(chip, IOEL_REPLAY_THRESHOLD);
	PPC_INSERT(ioel_replay_threshold, 0x7, 8, 3);
	PPC_INSERT(ioel_replay_threshold, 0xF, 4, 4);
	PPC_INSERT(ioel_replay_threshold, 0x6, 0, 4);
	put_scom(chip, IOEL_REPLAY_THRESHOLD, ioel_replay_threshold);

	ioel_sl_ecc_threshold = get_scom(chip, IOEL_SL_ECC_THRESHOLD);
	PPC_INSERT(ioel_sl_ecc_threshold, 0x7, 8, 3);
	PPC_INSERT(ioel_sl_ecc_threshold, 0xF, 4, 4);
	PPC_INSERT(ioel_sl_ecc_threshold, 0x7, 0, 4);
	put_scom(chip, IOEL_SL_ECC_THRESHOLD, ioel_sl_ecc_threshold);
}

static void chiplet_fabric_scominit(bool is_xbus_active, uint8_t chip)
{
	enum {
		PU_PB_CENT_SM0_FIR_REG = 0x05011C00,
		PU_PB_CENT_SM0_FIR_MASK_REG_SPARE_13 = 13,

		PU_PB_IOE_FIR_ACTION0_REG = 0x05013406,
		FBC_IOE_TL_FIR_ACTION0 = 0x0000000000000000,

		PU_PB_IOE_FIR_ACTION1_REG = 0x05013407,
		FBC_IOE_TL_FIR_ACTION1 = 0x0049000000000000,

		PU_PB_IOE_FIR_MASK_REG = 0x05013403,
		FBC_IOE_TL_FIR_MASK = 0xFF24F0303FFFF11,
		FBC_IOE_TL_FIR_MASK_X0_NF = 0x00C00C0C00000880,
		FBC_IOE_TL_FIR_MASK_X2_NF = 0x000300C0C0000220,

		XBUS_LL0_IOEL_FIR_ACTION0_REG = 0x06011806,
		FBC_IOE_DL_FIR_ACTION0 = 0x0000000000000000,

		XBUS_LL0_IOEL_FIR_ACTION1_REG = 0x06011807,
		FBC_IOE_DL_FIR_ACTION1 = 0x0303C00000001FFC,

		XBUS_LL0_IOEL_FIR_MASK_REG = 0x06011803,
		FBC_IOE_DL_FIR_MASK = 0xFCFC3FFFFFFFE003,
	};

	bool init_firs;
	uint64_t fbc_cent_fir;

	/* Apply FBC non-hotplug initfile */
	p9_fbc_no_hp_scom(is_xbus_active, chip);

	/* Setup IOE (XBUS FBC IO) TL SCOMs */
	p9_fbc_ioe_tl_scom(is_xbus_active, chip);

	/* TL/DL FIRs are configured by us only if not already setup by SBE */
	fbc_cent_fir = get_scom(chip, PU_PB_CENT_SM0_FIR_REG);
	init_firs = !(fbc_cent_fir & PPC_BIT(PU_PB_CENT_SM0_FIR_MASK_REG_SPARE_13));

	if (init_firs) {
		uint64_t fir_mask;

		put_scom(chip, PU_PB_IOE_FIR_ACTION0_REG, FBC_IOE_TL_FIR_ACTION0);
		put_scom(chip, PU_PB_IOE_FIR_ACTION1_REG, FBC_IOE_TL_FIR_ACTION1);

		fir_mask = FBC_IOE_TL_FIR_MASK
			 | FBC_IOE_TL_FIR_MASK_X0_NF
			 | FBC_IOE_TL_FIR_MASK_X2_NF;
		put_scom(chip, PU_PB_IOE_FIR_MASK_REG, fir_mask);
	}

	/* Setup IOE (XBUS FBC IO) DL SCOMs */
	p9_fbc_ioe_dl_scom(chip);

	if (init_firs) {
		put_scom(chip, XBUS_LL0_IOEL_FIR_ACTION0_REG, FBC_IOE_DL_FIR_ACTION0);
		put_scom(chip, XBUS_LL0_IOEL_FIR_ACTION1_REG, FBC_IOE_DL_FIR_ACTION1);
		put_scom(chip, XBUS_LL0_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK);
	}
}

void istep_8_9(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 8.9\n");
	report_istep(8,9);

	/* Not skipping master chip and initializing it even if we don't have a second chip */
	for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			chiplet_fabric_scominit(/*is_xbus_active=*/chips == 0x03, chip);
	}

	printk(BIOS_EMERG, "ending istep 8.9\n");
}
