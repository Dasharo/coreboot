/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_8.h>

#include <console/console.h>
#include <cpu/power/powerbus.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>

#include "scratch.h"
#include "xbus.h"

/*
 * This code omits initialization of OBus which isn't present. It also assumes
 * there is only one XBus (X1). Both of these statements are true for Nimbus
 * Sforza.
 */

static void p9_fbc_no_hp_scom(uint8_t chip)
{
	enum {
		PB_WEST_MODE             = 0x501180A,
		PB_CENT_MODE             = 0x5011C0A,
		PB_CENT_GP_CMD_RATE_DP0  = 0x5011C26,
		PB_CENT_GP_CMD_RATE_DP1  = 0x5011C27,
		PB_CENT_RGP_CMD_RATE_DP0 = 0x5011C28,
		PB_CENT_RGP_CMD_RATE_DP1 = 0x5011C29,
		PB_CENT_SP_CMD_RATE_DP0  = 0x5011C2A,
		PB_CENT_SP_CMD_RATE_DP1  = 0x5011C2B,
		PB_EAST_MODE             = 0x501200A,
	};

	const uint64_t scratch_reg6 = read_scom(0, MBOX_SCRATCH_REG1 + 5);
	/* ATTR_PROC_FABRIC_PUMP_MODE, it's either node or group pump mode */
	const bool node_pump_mode = !(scratch_reg6 & PPC_BIT(MBOX_SCRATCH_REG6_GROUP_PUMP_MODE));

	/* Assuming that ATTR_PROC_EPS_TABLE_TYPE = EPS_TYPE_LE in talos.xml is always correct */
	const bool is_flat_8 = false;

	/*
	 * ATTR_PROC_FABRIC_X_LINKS_CNFG
	 * We have one XBus, so I guess this is 1, otherwise need to dig into
	 * related code in Hostboot where link configuration is generated.
	 */
	const int num_x_links_cfg = 1;

	/* Power Bus PB West Mode Configuration Register */
	uint64_t pb_west_mode = get_scom(chip, PB_WEST_MODE);
	/* Power Bus PB CENT Mode Register */
	uint64_t pb_cent_mode = get_scom(chip, PB_CENT_MODE);
	/* Power Bus PB CENT GP command RATE DP0 Register */
	uint64_t pb_cent_gp_cmd_rate_dp0 = get_scom(chip, PB_CENT_GP_CMD_RATE_DP0);
	/* Power Bus PB CENT GP command RATE DP1 Register */
	uint64_t pb_cent_gp_cmd_rate_dp1 = get_scom(chip, PB_CENT_GP_CMD_RATE_DP1);
	/* Power Bus PB CENT RGP command RATE DP0 Register */
	uint64_t pb_cent_rgp_cmd_rate_dp0 = get_scom(chip, PB_CENT_RGP_CMD_RATE_DP0);
	/* Power Bus PB CENT RGP command RATE DP1 Register */
	uint64_t pb_cent_rgp_cmd_rate_dp1 = get_scom(chip, PB_CENT_RGP_CMD_RATE_DP1);
	/* Power Bus PB CENT SP command RATE DP0 Register */
	uint64_t pb_cent_sp_cmd_rate_dp0 = get_scom(chip, PB_CENT_SP_CMD_RATE_DP0);
	/* Power Bus PB CENT SP command RATE DP1 Register */
	uint64_t pb_cent_sp_cmd_rate_dp1 = get_scom(chip, PB_CENT_SP_CMD_RATE_DP1);
	/* Power Bus PB East Mode Configuration Register */
	uint64_t pb_east_mode = get_scom(chip, PB_EAST_MODE);

	if (!node_pump_mode || num_x_links_cfg == 0) {
		pb_cent_gp_cmd_rate_dp0 = 0;
		pb_cent_gp_cmd_rate_dp1 = 0;
	} else if (node_pump_mode && num_x_links_cfg > 0 && num_x_links_cfg < 3) {
		pb_cent_gp_cmd_rate_dp0 = 0x030406171C243448;
		pb_cent_gp_cmd_rate_dp1 = 0x040508191F283A50;
	} else if (node_pump_mode && num_x_links_cfg > 2) {
		pb_cent_gp_cmd_rate_dp0 = 0x0304062832405C80;
		pb_cent_gp_cmd_rate_dp1 = 0x0405082F3B4C6D98;
	}

	if (!node_pump_mode && num_x_links_cfg == 0) {
		pb_cent_rgp_cmd_rate_dp0 = 0;
		pb_cent_rgp_cmd_rate_dp1 = 0;
		pb_cent_sp_cmd_rate_dp0 = 0;
		pb_cent_sp_cmd_rate_dp1 = 0;
	} else if (!node_pump_mode && num_x_links_cfg > 0 && num_x_links_cfg < 3) {
		pb_cent_rgp_cmd_rate_dp0 = 0x030406080A0C1218;
		pb_cent_rgp_cmd_rate_dp1 = 0x040508080A0C1218;
		pb_cent_sp_cmd_rate_dp0 = 0x030406080A0C1218;
		pb_cent_sp_cmd_rate_dp1 = 0x030406080A0C1218;
	} else if ((!node_pump_mode && num_x_links_cfg == 3) ||
		   (node_pump_mode && num_x_links_cfg == 0)) {
		pb_cent_rgp_cmd_rate_dp0 = 0x0304060D10141D28;
		pb_cent_rgp_cmd_rate_dp1 = 0x0405080D10141D28;
		pb_cent_sp_cmd_rate_dp0 = 0x05070A0D10141D28;
		pb_cent_sp_cmd_rate_dp1 = 0x05070A0D10141D28;
	} else if ((!node_pump_mode && is_flat_8) ||
		   (node_pump_mode && num_x_links_cfg > 0 && num_x_links_cfg < 3)) {
		pb_cent_rgp_cmd_rate_dp0 = 0x030406171C243448;
		pb_cent_rgp_cmd_rate_dp1 = 0x040508191F283A50;
		pb_cent_sp_cmd_rate_dp0 = 0x080C12171C243448;
		pb_cent_sp_cmd_rate_dp1 = 0x0A0D14191F283A50;
	} else if (node_pump_mode && num_x_links_cfg > 2) {
		pb_cent_rgp_cmd_rate_dp0 = 0x0304062832405C80;
		pb_cent_rgp_cmd_rate_dp1 = 0x0405082F3B4C6D98;
		pb_cent_sp_cmd_rate_dp0 = 0x08141F2832405C80;
		pb_cent_sp_cmd_rate_dp1 = 0x0A18252F3B4C6D98;
	}

	if (num_x_links_cfg == 0)
		pb_east_mode |= 0x0300000000000000;
	else
		pb_east_mode &= 0xF1FFFFFFFFFFFFFF;

	if (node_pump_mode && is_flat_8) {
		pb_west_mode &= 0xFFFF0003FFFFFFFF;
		pb_west_mode |= 0x00003E8000000000;

		pb_cent_mode &= 0xFFFF0003FFFFFFFF;
		pb_cent_mode |= 0x00003E8000000000;

		pb_east_mode &= 0xFFFF0003FFFFFFFF;
		pb_east_mode |= 0x00003E8000000000;
	} else {
		pb_west_mode &= 0xFFFF0003FFFFFFFF;
		pb_west_mode |= 0x00007EFC00000000;

		pb_cent_mode &= 0xFFFF0003FFFFFFFF;
		pb_cent_mode |= 0x00007EFC00000000;

		pb_east_mode &= 0xFFFF0003FFFFFFFF;
		pb_east_mode |= 0x00007EFC00000000;
	}

	pb_west_mode &= 0xF7FFFFFFFFFFFFFF;

	pb_west_mode &= 0xFFFFFFFC0FFFFFFF;
	pb_west_mode |= 0x00000002A0000000;

	pb_cent_mode &= 0xFFFFFFFC0FFFFFFF;
	pb_cent_mode |= 0x00000002A0000000;

	pb_east_mode &= 0xFFFFFFFC0FFFFFFF;
	pb_east_mode |= 0x00000002A0000000;

	put_scom(chip, PB_WEST_MODE, pb_west_mode);
	put_scom(chip, PB_CENT_MODE, pb_cent_mode);
	put_scom(chip, PB_CENT_GP_CMD_RATE_DP0, pb_cent_gp_cmd_rate_dp0);
	put_scom(chip, PB_CENT_GP_CMD_RATE_DP1, pb_cent_gp_cmd_rate_dp1);
	put_scom(chip, PB_CENT_RGP_CMD_RATE_DP0, pb_cent_rgp_cmd_rate_dp0);
	put_scom(chip, PB_CENT_RGP_CMD_RATE_DP1, pb_cent_rgp_cmd_rate_dp1);
	put_scom(chip, PB_CENT_SP_CMD_RATE_DP0, pb_cent_sp_cmd_rate_dp0);
	put_scom(chip, PB_CENT_SP_CMD_RATE_DP1, pb_cent_sp_cmd_rate_dp1);
	put_scom(chip, PB_EAST_MODE, pb_east_mode);
}

static void p9_fbc_ioe_tl_scom(uint8_t chip)
{
	enum {
		PB_FP01_CFG              = 0x501340A,
		PB_FP23_CFG              = 0x501340B,
		PB_FP45_CFG              = 0x501340C,
		PB_ELINK_DATA_23_CFG_REG = 0x5013411,
		PB_ELINK_DATA_45_CFG_REG = 0x5013412,
		PB_MISC_CFG              = 0x5013423,
		PB_TRACE_CFG             = 0x5013424,
	};

	/*
	 * We only support one XBus with
	 *     ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG = { false, true, false }
	 * Meaning that X1 is present and X0 and X2 aren't (from logs).
	 * Should we determine link number dynamically?
	 */

	const uint64_t pb_freq_mhz = powerbus_cfg(chip)->fabric_freq;

	const uint64_t dd2_lo_limit_n = pb_freq_mhz * 82;

	/* Processor bus Electrical Framer/Parser 01 configuration register */
	uint64_t pb_fp01_cfg = get_scom(chip, PB_FP01_CFG);
	/* Power Bus Electrical Framer/Parser 23 Configuration Register */
	uint64_t pb_fp23_cfg = get_scom(chip, PB_FP23_CFG);
	/* Power Bus Electrical Framer/Parser 45 Configuration Register */
	uint64_t pb_fp45_cfg = get_scom(chip, PB_FP45_CFG);
	/* Power Bus Electrical Link Data Buffer 23 Configuration Register */
	uint64_t pb_elink_data_23_cfg_reg = get_scom(chip, PB_ELINK_DATA_23_CFG_REG);
	/* Power Bus Electrical Link Data Buffer 45 Configuration Register */
	uint64_t pb_elink_data_45_cfg_reg = get_scom(chip, PB_ELINK_DATA_45_CFG_REG);
	/* Power Bus Electrical Miscellaneous Configuration Register */
	uint64_t pb_misc_cfg = get_scom(chip, PB_MISC_CFG);
	/* Power Bus Electrical Link Trace Configuration Register */
	uint64_t pb_trace_cfg = get_scom(chip, PB_TRACE_CFG);

	pb_fp01_cfg |= 0x0000084000000840;

	pb_fp45_cfg |= 0x0000084000000840;

	PPC_INSERT(pb_trace_cfg, 0x4, 16, 4);
	PPC_INSERT(pb_trace_cfg, 0x4, 24, 4);
	PPC_INSERT(pb_trace_cfg, 0x1, 20, 4);
	PPC_INSERT(pb_trace_cfg, 0x1, 28, 4);

	PPC_INSERT(pb_fp23_cfg, 0x15 - (dd2_lo_limit_n / 20000), 4, 8);
	PPC_INSERT(pb_fp23_cfg, 0x15 - (dd2_lo_limit_n / 20000), 36, 8);

	PPC_INSERT(pb_fp23_cfg, 0x01, 22, 2);
	PPC_INSERT(pb_fp23_cfg, 0x20, 12, 8);
	PPC_INSERT(pb_fp23_cfg, 0x00, 20, 1);
	PPC_INSERT(pb_fp23_cfg, 0x00, 25, 1);
	PPC_INSERT(pb_fp23_cfg, 0x20, 44, 8);
	PPC_INSERT(pb_fp23_cfg, 0x00, 52, 1);
	PPC_INSERT(pb_fp23_cfg, 0x00, 57, 1);
	PPC_INSERT(pb_elink_data_23_cfg_reg, 0x1F, 24, 5);
	PPC_INSERT(pb_elink_data_23_cfg_reg, 0x40, 1, 7);
	PPC_INSERT(pb_elink_data_23_cfg_reg, 0x40, 33, 7);
	PPC_INSERT(pb_elink_data_23_cfg_reg, 0x3C, 9, 7);
	PPC_INSERT(pb_elink_data_23_cfg_reg, 0x3C, 41, 7);
	PPC_INSERT(pb_elink_data_23_cfg_reg, 0x3C, 17, 7);
	PPC_INSERT(pb_elink_data_23_cfg_reg, 0x3C, 49, 7);

	pb_misc_cfg &= ~PPC_BIT(0);
	pb_misc_cfg |= PPC_BIT(1);
	pb_misc_cfg &= ~PPC_BIT(2);

	put_scom(chip, PB_FP01_CFG, pb_fp01_cfg);
	put_scom(chip, PB_FP23_CFG, pb_fp23_cfg);
	put_scom(chip, PB_FP45_CFG, pb_fp45_cfg);
	put_scom(chip, PB_ELINK_DATA_23_CFG_REG, pb_elink_data_23_cfg_reg);
	put_scom(chip, PB_ELINK_DATA_45_CFG_REG, pb_elink_data_45_cfg_reg);
	put_scom(chip, PB_MISC_CFG, pb_misc_cfg);
	put_scom(chip, PB_TRACE_CFG, pb_trace_cfg);
}

static void p9_fbc_ioe_dl_scom(uint8_t chip)
{
	enum {
		IOEL_CONFIG = 0x601180A,
		IOEL_REPLAY_THRESHOLD = 0x6011818,
		IOEL_SL_ECC_THRESHOLD = 0x6011819,
	};

	/* ELL Configuration Register */
	uint64_t ioel_config = get_scom(chip, IOEL_CONFIG);
	/* ELL Replay Threshold Register */
	uint64_t ioel_replay_threshold = get_scom(chip, IOEL_REPLAY_THRESHOLD);
	/* ELL SL ECC Threshold Register */
	uint64_t ioel_sl_ecc_threshold = get_scom(chip, IOEL_SL_ECC_THRESHOLD);

	/* ATTR_LINK_TRAIN == fapi2::ENUM_ATTR_LINK_TRAIN_BOTH (from logs) */
	ioel_config |= 0x8000000000000000;
	ioel_config &= 0xFFEFFFFFFFFFFFFF;
	ioel_config |= 0x280F000F00000000;

	ioel_sl_ecc_threshold |= 0x00E0000000000000;

	ioel_replay_threshold &= 0x0FFFFFFFFFFFFFFF;
	ioel_replay_threshold |= 0x6FE0000000000000;

	ioel_sl_ecc_threshold &= 0x7FFFFFFFFFFFFFFF;
	ioel_sl_ecc_threshold |= 0x7F00000000000000;

	put_scom(chip, IOEL_CONFIG, ioel_config);
	put_scom(chip, IOEL_REPLAY_THRESHOLD, ioel_replay_threshold);
	put_scom(chip, IOEL_SL_ECC_THRESHOLD, ioel_sl_ecc_threshold);
}

static void chiplet_fabric_scominit(uint8_t chip)
{
	enum {
		PU_PB_CENT_SM0_IR_REG = 0x5011C00,
		PU_PB_CENT_SM0_IR_MASK_REG_SPARE_13 = PPC_BIT(13),

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
	p9_fbc_no_hp_scom(chip);

	/* Setup IOE (XBUS FBC IO) TL SCOMs */
	p9_fbc_ioe_tl_scom(chip);

	/* TL/DL FIRs are configured by us only if not already setup by SBE */
	fbc_cent_fir = get_scom(chip, PU_PB_CENT_SM0_IR_REG);
	init_firs = !(fbc_cent_fir & PU_PB_CENT_SM0_IR_MASK_REG_SPARE_13);

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

	if (chips != 0x01) {
		/* Not skipping master chip */
		for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
			if (chips & (1 << chip))
				chiplet_fabric_scominit(chip);
		}
	}

	printk(BIOS_EMERG, "ending istep 8.9\n");
}
