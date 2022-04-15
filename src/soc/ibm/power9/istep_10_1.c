/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_10.h>

#include <console/console.h>
#include <cpu/power/powerbus.h>
#include <cpu/power/scom.h>
#include <delay.h>

#include "fsi.h"
#include "xbus.h"

enum build_smp_adu_action {
	SWITCH_AB = 1,
	SWITCH_CD = 2,
	QUIESCE   = 4,
	RESET_SWITCH = 8
};

enum adu_op {
	PB_DIS_OPER,   // pbop.disable_all
	PMISC_OPER,    // pmisc switch
	PRE_SWITCH_CD, // do not issue PB command, pre-set for switch CD operation
	PRE_SWITCH_AB, // do not issue PB command, pre-set for switch AB operation
	POST_SWITCH    // do not issue PB command, clear switch CD/AB flags
};

enum sbe_memory_access_flags {
	SBE_MEM_ACCESS_FLAGS_TARGET_PROC          = 0x00000001,
	SBE_MEM_ACCESS_FLAGS_PB_DIS_MODE          = 0x00000400,
	SBE_MEM_ACCESS_FLAGS_SWITCH_MODE          = 0x00000800,
	SBE_MEM_ACCESS_FLAGS_PRE_SWITCH_CD_MODE   = 0x00002000,
	SBE_MEM_ACCESS_FLAGS_PRE_SWITCH_AB_MODE   = 0x00004000,
	SBE_MEM_ACCESS_FLAGS_POST_SWITCH_MODE     = 0x00008000,
};

enum {
	PU_ALTD_ADDR_REG = 0x00090000,

	PU_SND_MODE_REG = 0x00090021,
	PU_SND_MODE_REG_PB_STOP = 22,
	PU_SND_MODE_REG_ENABLE_PB_SWITCH_AB = 30,
	PU_SND_MODE_REG_ENABLE_PB_SWITCH_CD = 31,

	PU_ALTD_CMD_REG = 0x00090001,
	PU_ALTD_CMD_REG_FBC_START_OP = 2,
	PU_ALTD_CMD_REG_FBC_CLEAR_STATUS = 3,
	PU_ALTD_CMD_REG_FBC_RESET_FSM = 4,
	PU_ALTD_CMD_REG_FBC_AXTYPE = 6,
	PU_ALTD_CMD_REG_FBC_LOCKED = 11,
	PU_ALTD_CMD_REG_FBC_SCOPE = 16,
	PU_ALTD_CMD_REG_FBC_SCOPE_LEN = 3,
	PU_ALTD_CMD_REG_FBC_DROP_PRIORITY = 20,
	PU_ALTD_CMD_REG_FBC_OVERWRITE_PBINIT = 22,
	PU_ALTD_CMD_REG_FBC_WITH_TM_QUIESCE = 24,
	PU_ALTD_CMD_REG_FBC_TTYPE = 25,
	PU_ALTD_CMD_REG_FBC_TTYPE_LEN = 7,
	PU_ALTD_CMD_REG_FBC_TSIZE = 32,
	PU_ALTD_CMD_REG_FBC_TSIZE_LEN = 8,

	ALTD_CMD_TTYPE_PB_OPER = 0x3F,
	ALTD_CMD_TTYPE_PMISC_OPER = 0x31,
	ALTD_CMD_PMISC_TSIZE_1 = 2, // PMISC SWITCH
	ALTD_CMD_SCOPE_SYSTEM = 5,
	ALTD_CMD_PB_DIS_OPERATION_TSIZE = 8,

	PU_ALTD_STATUS_REG = 0x00090003,
	PU_ALTD_STATUS_REG_FBC_ALTD_BUSY = 0,
	PU_ALTD_STATUS_REG_FBC_WAIT_CMD_ARBIT = 1,
	PU_ALTD_STATUS_REG_FBC_ADDR_DONE = 2,
	PU_ALTD_STATUS_REG_FBC_DATA_DONE = 3,
	PU_ALTD_STATUS_REG_FBC_WAIT_RESP = 4,
	PU_ALTD_STATUS_REG_FBC_OVERRUN_ERROR = 5,
	PU_ALTD_STATUS_REG_FBC_AUTOINC_ERROR = 6,
	PU_ALTD_STATUS_REG_FBC_COMMAND_ERROR = 7,
	PU_ALTD_STATUS_REG_FBC_ADDRESS_ERROR = 8,
	PU_ALTD_STATUS_REG_FBC_PBINIT_MISSING = 18,
	PU_ALTD_STATUS_REG_FBC_ECC_CE = 48,
	PU_ALTD_STATUS_REG_FBC_ECC_UE = 49,
	PU_ALTD_STATUS_REG_FBC_ECC_SUE = 50,

	PU_ALTD_OPTION_REG = 0x00090002,
	PU_ALTD_OPTION_REG_FBC_WITH_PRE_QUIESCE = 23,
	PU_ALTD_OPTION_REG_FBC_AFTER_QUIESCE_WAIT_COUNT = 28,
	PU_ALTD_OPTION_REG_FBC_AFTER_QUIESCE_WAIT_COUNT_LEN = 20,
	PU_ALTD_OPTION_REG_FBC_WITH_POST_INIT = 51,
	PU_ALTD_OPTION_REG_FBC_ALTD_HW397129 = 52,
	PU_ALTD_OPTION_REG_FBC_BEFORE_INIT_WAIT_COUNT = 54,
	PU_ALTD_OPTION_REG_FBC_BEFORE_INIT_WAIT_COUNT_LEN = 10,

	PU_ALTD_DATA_REG = 0x00090004,

	PU_PB_CENT_SM0_PB_CENT_MODE = 0x05011C0A,

	P9_BUILD_SMP_NUM_SHADOWS = 3,

	PU_PB_WEST_SM0_PB_WEST_HP_MODE_CURR  = 0x0501180C,
	PU_PB_CENT_SM0_PB_CENT_HP_MODE_CURR  = 0x05011C0C,
	PU_PB_EAST_HP_MODE_CURR              = 0x0501200C,

	PU_PB_WEST_SM0_PB_WEST_HP_MODE_NEXT  = 0x0501180B,
	PU_PB_CENT_SM0_PB_CENT_HP_MODE_NEXT  = 0x05011C0B,
	PU_PB_EAST_HP_MODE_NEXT              = 0x0501200B,

	PU_PB_WEST_SM0_PB_WEST_HPX_MODE_CURR = 0x05011810,
	PU_PB_CENT_SM0_PB_CENT_HPX_MODE_CURR = 0x05011C10,
	PU_PB_EAST_HPX_MODE_CURR             = 0x05012010,

	PU_PB_WEST_SM0_PB_WEST_HPX_MODE_NEXT = 0x0501180F,
	PU_PB_CENT_SM0_PB_CENT_HPX_MODE_NEXT = 0x05011C0F,
	PU_PB_EAST_HPX_MODE_NEXT             = 0x0501200F,

	PU_PB_WEST_SM0_PB_WEST_HPA_MODE_CURR = 0x0501180E,
	PU_PB_CENT_SM0_PB_CENT_HPA_MODE_CURR = 0x05011C0E,
	PU_PB_EAST_HPA_MODE_CURR             = 0x0501200E,

	PU_PB_WEST_SM0_PB_WEST_HPA_MODE_NEXT = 0x0501180D,
	PU_PB_CENT_SM0_PB_CENT_HPA_MODE_NEXT = 0x05011C0D,
	PU_PB_EAST_HPA_MODE_NEXT             = 0x0501200D,
};

/* HP (HotPlug Mode Register) */
static const uint64_t PB_HP_MODE_CURR_SHADOWS[P9_BUILD_SMP_NUM_SHADOWS] = {
	PU_PB_WEST_SM0_PB_WEST_HP_MODE_CURR,
	PU_PB_CENT_SM0_PB_CENT_HP_MODE_CURR,
	PU_PB_EAST_HP_MODE_CURR
};
static const uint64_t PB_HP_MODE_NEXT_SHADOWS[P9_BUILD_SMP_NUM_SHADOWS] = {
	PU_PB_WEST_SM0_PB_WEST_HP_MODE_NEXT,
	PU_PB_CENT_SM0_PB_CENT_HP_MODE_NEXT,
	PU_PB_EAST_HP_MODE_NEXT
};

/* HPX (Hotplug Mode Register Extension) */
static const uint64_t PB_HPX_MODE_CURR_SHADOWS[P9_BUILD_SMP_NUM_SHADOWS] = {
	PU_PB_WEST_SM0_PB_WEST_HPX_MODE_CURR,
	PU_PB_CENT_SM0_PB_CENT_HPX_MODE_CURR,
	PU_PB_EAST_HPX_MODE_CURR
};
static const uint64_t PB_HPX_MODE_NEXT_SHADOWS[P9_BUILD_SMP_NUM_SHADOWS] = {
	PU_PB_WEST_SM0_PB_WEST_HPX_MODE_NEXT,
	PU_PB_CENT_SM0_PB_CENT_HPX_MODE_NEXT,
	PU_PB_EAST_HPX_MODE_NEXT
};

/* HPA */
static const uint64_t PB_HPA_MODE_CURR_SHADOWS[P9_BUILD_SMP_NUM_SHADOWS] = {
	PU_PB_WEST_SM0_PB_WEST_HPA_MODE_CURR,
	PU_PB_CENT_SM0_PB_CENT_HPA_MODE_CURR,
	PU_PB_EAST_HPA_MODE_CURR
};
static const uint64_t PB_HPA_MODE_NEXT_SHADOWS[P9_BUILD_SMP_NUM_SHADOWS] = {
	PU_PB_WEST_SM0_PB_WEST_HPA_MODE_NEXT,
	PU_PB_CENT_SM0_PB_CENT_HPA_MODE_NEXT,
	PU_PB_EAST_HPA_MODE_NEXT
};

/*
 * SCOM registers in this function are not documented. Moreover not all of 1-11
 * bits are zero in them which contradicts documentation.
 */
static void p9_fbc_cd_hp1_scom(uint8_t chip)
{
	const struct powerbus_cfg *pb_cfg = powerbus_cfg(chip);
	const uint32_t pb_freq_mhz = pb_cfg->fabric_freq;

	/* Frequency of XBus for Nimbus DD2 */
	const uint32_t xbus_freq_mhz = 2000;

	uint64_t val;
	uint64_t tmp;

	val = PPC_PLACE(0x08, 54, 5) | PPC_PLACE(0x03, 59, 5);
	put_scom(chip, 0x90000CB205012011, val);

	tmp = 0;
	if (100 * xbus_freq_mhz >= 120 * pb_freq_mhz)
		tmp = 0x09;
	else if (100 * xbus_freq_mhz >= 100 * pb_freq_mhz)
		tmp = 0x0A;
	else if (105 * xbus_freq_mhz >= 100 * pb_freq_mhz)
		tmp = 0x0B;
	else if (125 * xbus_freq_mhz >= 100 * pb_freq_mhz)
		tmp = 0x0C;
	val = PPC_PLACE(tmp, 54, 5) | PPC_PLACE(3, 59, 5);
	put_scom(chip, 0x90000CB305012011, val);

	val = PPC_PLACE(0x10, 51, 5) | PPC_PLACE(2, 58, 2) | PPC_PLACE(0xF, 60, 4);
	put_scom(chip, 0x90000CDB05011C11, val);

	val = PPC_PLACE(7, 49, 3) | PPC_PLACE(4, 52, 6);
	put_scom(chip, 0x90000CF405011C11, val);

	val = PPC_PLACE(0xC, 45, 4) | PPC_PLACE(1, 57, 2);
	put_scom(chip, 0x90000D3F05011C11, val);

	val = PPC_PLACE(3, 41, 2) | PPC_PLACE(1, 43, 2) | PPC_PLACE(3, 45, 4)
	    | PPC_PLACE(0xC0, 49, 8);
	put_scom(chip, 0x90000D7805011C11, val);

	val = PPC_PLACE(8, 38, 4) | PPC_PLACE(4, 42, 4) | PPC_PLACE(1, 57, 3)
	    | PPC_PLACE(0xF, 60, 4);
	put_scom(chip, 0x90000DAA05011C11, val);

	val = PPC_PLACE(4, 36, 3) | PPC_PLACE(0x20, 41, 8) | PPC_BIT(49) | PPC_BIT(51)
	    | PPC_BIT(52) | PPC_BIT(53) | PPC_BIT(55) | PPC_BIT(56) | PPC_BIT(57)
	    | PPC_PLACE(0xF, 60, 4);
	put_scom(chip, 0x90000DCC05011C11, val);

	val = PPC_PLACE(1, 41, 3) | PPC_PLACE(1, 44, 3) | PPC_PLACE(2, 47, 3)
	    | PPC_PLACE(3, 50, 3) | PPC_PLACE(5, 53, 3) | PPC_PLACE(5, 57, 3);
	put_scom(chip, 0x90000E0605011C11, val);

	val = PPC_PLACE(0x06, 33, 5) | PPC_PLACE(0x0D, 38, 5) | PPC_PLACE(0x1E, 48, 5)
	    | PPC_PLACE(0x19, 53, 5) | PPC_BIT(63);
	put_scom(chip, 0x90000E4305011C11, val);

	val = PPC_PLACE(0x400, 22, 12) | PPC_PLACE(0x400, 34, 12)
	    | PPC_PLACE(2, 46, 3) | PPC_PLACE(2, 49, 3) | PPC_PLACE(2, 52, 3)
	    | PPC_PLACE(2, 55, 3) | PPC_PLACE(2, 58, 3) | PPC_PLACE(2, 61, 3);
	put_scom(chip, 0x90000EA205011C11, val);

	/* 44 - set because ATTR_CHIP_EC_FEATURE_HW409019 == 1 */
	val = PPC_PLACE(0x0C, 20, 8) | PPC_BIT(44);
	put_scom(chip, 0x90000EC705011C11, val);

	val = PPC_PLACE(0x4, 18, 10) | PPC_PLACE(0x141, 28, 12) | PPC_PLACE(0x21B, 40, 12)
	    | PPC_PLACE(0x30D, 52, 12);
	put_scom(chip, 0x90000EE105011C11, val);

	val = PPC_PLACE(1, 25, 3) | PPC_PLACE(1, 28, 3) | PPC_PLACE(2, 31, 3)
	    | PPC_PLACE(3, 34, 3) | PPC_PLACE(5, 37, 3) | PPC_PLACE(1, 49, 3)
	    | PPC_PLACE(1, 52, 3) | PPC_PLACE(2, 55, 3) | PPC_PLACE(3, 58, 3)
	    | PPC_PLACE(5, 61, 3);
	put_scom(chip, 0x90000F0505011C11, val);

	val = PPC_PLACE(0x7, 14, 10) | PPC_PLACE(0x5, 24, 10) | PPC_PLACE(0x5, 34, 10)
	    | PPC_PLACE(0x4, 44, 10) | PPC_PLACE(0x5, 54, 10);
	put_scom(chip, 0x90000F2005011C11, val);

	val = PPC_BIT(20) | PPC_PLACE(3, 32, 2) | PPC_PLACE(7, 34, 3) | PPC_PLACE(3, 37, 2)
	    | PPC_PLACE(1, 41, 1) | PPC_PLACE(1, 42, 1);
	if (pb_cfg->core_ceiling_ratio != FABRIC_CORE_CEILING_RATIO_RATIO_8_8)
		val |= PPC_PLACE(3, 24, 2) | PPC_PLACE(3, 44, 2);
	tmp = (pb_cfg->core_ceiling_ratio == FABRIC_CORE_CEILING_RATIO_RATIO_8_8 ? 3 : 2);
	val |= PPC_PLACE(tmp, 28, 2);
	put_scom(chip, 0x90000F4005011811, val);
	put_scom(chip, 0x90000F4005012011, val);

	val = PPC_BIT(12) | PPC_PLACE(4, 13, 4) | PPC_PLACE(4, 17, 4) | PPC_PLACE(4, 21, 4)
	    | PPC_PLACE(1, 25, 3) | PPC_PLACE(1, 28, 3) | PPC_PLACE(1, 31, 3)
	    | PPC_PLACE(0xFE, 34, 8) | PPC_PLACE(0xFE, 42, 8) | PPC_PLACE(1, 50, 2)
	    | PPC_PLACE(2, 54, 3) | PPC_PLACE(2, 57, 2) | PPC_BIT(60) | PPC_BIT(61)
	    | PPC_BIT(63);
	put_scom(chip, 0x90000F4D05011C11, val);

	val = PPC_BIT(35) | PPC_PLACE(1, 36, 2) | PPC_PLACE(2, 39, 2) | PPC_BIT(49)
	    | PPC_PLACE(1, 51, 2);

	if (pb_cfg->core_floor_ratio == FABRIC_CORE_FLOOR_RATIO_RATIO_2_8)
		tmp = 3;
	else if (pb_cfg->core_floor_ratio == FABRIC_CORE_FLOOR_RATIO_RATIO_4_8)
		tmp = 2;
	else
		tmp = 1;
	val |= PPC_PLACE(tmp, 41, 2);

	if (pb_cfg->core_floor_ratio == FABRIC_CORE_FLOOR_RATIO_RATIO_2_8)
		tmp = 0;
	else if (pb_cfg->core_floor_ratio == FABRIC_CORE_FLOOR_RATIO_RATIO_4_8)
		tmp = 3;
	else
		tmp = 2;
	val |= PPC_PLACE(tmp, 44, 2);

	put_scom(chip, 0x90000E6105011811, val);
	put_scom(chip, 0x90000E6105012011, val);
}

/*
 * SCOM registers in this function are not documented. Moreover not all of 1-11
 * bits are zero in them which contradicts documentation.
 */
static void p9_fbc_cd_hp23_scom(uint8_t chip, int seq)
{
	const uint64_t tmp = (seq == 2);

	uint64_t val;

	val = PPC_PLACE(8, 38, 4) | PPC_PLACE(4, 42, 4) | PPC_PLACE(tmp, 50, 1)
	    | PPC_PLACE(1, 57, 3) | PPC_PLACE(0xF, 60, 4);
	put_scom(chip, 0x90000DAA05011C11, val);

	val = PPC_BIT(12) | PPC_PLACE(4, 13, 4) | PPC_PLACE(4, 17, 4) | PPC_PLACE(4, 21, 4)
	    | PPC_PLACE(1, 25, 3) | PPC_PLACE(1, 28, 3) | PPC_PLACE(1, 31, 3)
	    | PPC_PLACE(0xFE, 34, 8) | PPC_PLACE(0xFE, 42, 8) | PPC_PLACE(1, 50, 2)
	    | PPC_PLACE(2, 54, 3) | PPC_PLACE(2, 57, 2) | PPC_PLACE(tmp, 59, 1)
	    | PPC_PLACE(tmp, 60, 1) | PPC_BIT(61) | PPC_BIT(63);
	put_scom(chip, 0x90000F4D05011C11, val);
}

/* Set action which will occur on fabric pmisc switch command */
static void p9_adu_coherent_utils_set_switch_action(uint8_t chip, enum adu_op adu_op)
{
	uint64_t mask = PPC_BIT(PU_SND_MODE_REG_ENABLE_PB_SWITCH_AB)
		      | PPC_BIT(PU_SND_MODE_REG_ENABLE_PB_SWITCH_CD);

	uint64_t data = 0;
	if (adu_op == PRE_SWITCH_AB)
		data |= PPC_BIT(PU_SND_MODE_REG_ENABLE_PB_SWITCH_AB);
	if (adu_op == PRE_SWITCH_CD)
		data |= PPC_BIT(PU_SND_MODE_REG_ENABLE_PB_SWITCH_CD);

	and_or_scom(chip, PU_SND_MODE_REG, ~mask, data);
}

static void p9_adu_coherent_utils_check_fbc_state(uint8_t chip)
{
	/* PU_PB_CENT_SM0_PB_CENT_MODE_PB_CENT_PBIXXX_INIT */
	if (!(get_scom(chip, PU_PB_CENT_SM0_PB_CENT_MODE) & PPC_BIT(0)))
		die("FBC isn't initialized!\n");

	if (get_scom(chip, PU_SND_MODE_REG) & PPC_BIT(PU_SND_MODE_REG_PB_STOP))
		die("FBC isn't running!\n");
}

static void lock_adu(uint8_t chip)
{
	uint64_t data = 0;

	/* Configuring lock manipulation control data buffer to perform lock acquisition */
	data |= PPC_BIT(PU_ALTD_CMD_REG_FBC_LOCKED);
	data |= PPC_BIT(PU_ALTD_CMD_REG_FBC_RESET_FSM);
	data |= PPC_BIT(PU_ALTD_CMD_REG_FBC_CLEAR_STATUS);

	/* Write ADU command register to attempt lock manipulation */
	put_scom(chip, PU_ALTD_CMD_REG, data);
}

/* Setup the value for ADU option register to enable quiesce & init around a
 * switch operation */
static void set_quiesce_init(uint8_t chip)
{
	enum {
		QUIESCE_SWITCH_WAIT_COUNT = 128,
		INIT_SWITCH_WAIT_COUNT = 128,
	};

	uint64_t data = 0;

	/* Setup quiesce */
	data |= PPC_BIT(PU_ALTD_OPTION_REG_FBC_WITH_PRE_QUIESCE);
	PPC_INSERT(data, QUIESCE_SWITCH_WAIT_COUNT,
		   PU_ALTD_OPTION_REG_FBC_AFTER_QUIESCE_WAIT_COUNT,
		   PU_ALTD_OPTION_REG_FBC_AFTER_QUIESCE_WAIT_COUNT_LEN);

	/* Setup post-command init */
	data |= PPC_BIT(PU_ALTD_OPTION_REG_FBC_WITH_POST_INIT);
	PPC_INSERT(data, INIT_SWITCH_WAIT_COUNT,
		   PU_ALTD_OPTION_REG_FBC_BEFORE_INIT_WAIT_COUNT,
		   PU_ALTD_OPTION_REG_FBC_BEFORE_INIT_WAIT_COUNT_LEN);

	/* Setup workaround for HW397129 to re-enable fastpath for DD2 */
	data |= PPC_BIT(PU_ALTD_OPTION_REG_FBC_ALTD_HW397129);

	put_scom(chip, PU_ALTD_OPTION_REG, data);
}

static void p9_adu_coherent_setup_adu(uint8_t chip, enum adu_op adu_op)
{
	uint64_t cmd = 0x0;
	uint32_t ttype = 0;
	uint32_t tsize = 0;

	/* Write the address. Not sure if operations we support actually need
	 * this. */
	put_scom(chip, PU_ALTD_ADDR_REG, 0);

	/* This routine assumes the lock is held by the caller, preserve this
	 * locked state */
	cmd |= PPC_BIT(PU_ALTD_CMD_REG_FBC_LOCKED);

	if (adu_op == PB_DIS_OPER || adu_op == PMISC_OPER) {
		cmd |= PPC_BIT(PU_ALTD_CMD_REG_FBC_START_OP);

		PPC_INSERT(cmd, ALTD_CMD_SCOPE_SYSTEM,
			   PU_ALTD_CMD_REG_FBC_SCOPE, PU_ALTD_CMD_REG_FBC_SCOPE_LEN);

		/* DROP_PRIORITY = HIGH */
		cmd |= PPC_BIT(PU_ALTD_CMD_REG_FBC_DROP_PRIORITY);
		/* AXTYPE = Address only */
		cmd |= PPC_BIT(PU_ALTD_CMD_REG_FBC_AXTYPE);
		cmd |= PPC_BIT(PU_ALTD_CMD_REG_FBC_WITH_TM_QUIESCE);

		if (adu_op == PB_DIS_OPER) {
			ttype = ALTD_CMD_TTYPE_PB_OPER;
			tsize = ALTD_CMD_PB_DIS_OPERATION_TSIZE;
		} else {
			ttype = ALTD_CMD_TTYPE_PMISC_OPER;
			tsize = ALTD_CMD_PMISC_TSIZE_1;

			/* Set quiesce and init around a switch operation in option reg */
			set_quiesce_init(chip);
		}
	}

	PPC_INSERT(cmd, ttype, PU_ALTD_CMD_REG_FBC_TTYPE, PU_ALTD_CMD_REG_FBC_TTYPE_LEN);
	PPC_INSERT(cmd, tsize, PU_ALTD_CMD_REG_FBC_TSIZE, PU_ALTD_CMD_REG_FBC_TSIZE_LEN);

	put_scom(chip, PU_ALTD_CMD_REG, cmd);
}

static void p9_adu_setup(uint8_t chip, enum adu_op adu_op)
{
	/* Don't generate fabric command, just pre-condition ADU for upcoming switch */
	if (adu_op == PRE_SWITCH_AB || adu_op == PRE_SWITCH_CD || adu_op == POST_SWITCH) {
		p9_adu_coherent_utils_set_switch_action(chip, adu_op);
		return;
	}

	/* Ensure fabric is running */
	p9_adu_coherent_utils_check_fbc_state(chip);

	/*
	 * Acquire ADU lock to guarantee exclusive use of the ADU resources.
	 * ADU state machine will be reset/cleared by this routine.
	 */
	lock_adu(chip);

	/* Setup the ADU registers for operation */
	p9_adu_coherent_setup_adu(chip, adu_op);
}

static void p9_adu_coherent_status_check(uint8_t chip, bool is_addr_only)
{
	int i;
	uint64_t status;

	//Check for a successful status 10 times
	for (i = 0; i < 10; i++) {
		status = get_scom(chip, PU_ALTD_STATUS_REG);

		if (!(status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_ALTD_BUSY)))
			break;

		/* Delay to allow the write/read/other command to finish */
		udelay(1); // actually need only 100ns, so delaying at the bottom
	}

	if (!(status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_ADDR_DONE)))
		die("The address portion of ADU operation is not complete!\n");
	if (!is_addr_only && !(status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_DATA_DONE)))
		die("The data portion of ADU operation is not complete!\n");

	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_WAIT_CMD_ARBIT))
		die("ADU is still waiting for command arbitrage!\n");
	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_WAIT_RESP))
		die("ADU is still waiting for a clean combined response!\n");
	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_OVERRUN_ERROR))
		die("ADU data overrun!\n");
	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_AUTOINC_ERROR))
		die("Internal ADU address counter rolled over the 0.5M boundary!\n");
	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_COMMAND_ERROR))
		die("New ADU command was issued before previous one finished!\n");
	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_ADDRESS_ERROR))
		die("Invalid ADU Address!\n");
	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_PBINIT_MISSING))
		die("Attempt to start an ADU command without pb_init active!\n");
	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_ECC_CE))
		die("ECC Correctable error from ADU!\n");
	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_ECC_UE))
		die("ECC Uncorrectable error from ADU!\n");
	if (status & PPC_BIT(PU_ALTD_STATUS_REG_FBC_ECC_SUE))
		die("ECC Special Uncorrectable error!\n");

	if (i == 10)
		die("ADU is busy for too long with status: 0x%016llx!\n", status);
}

static void p9_adu_access(uint8_t chip, enum adu_op adu_op)
{
	const bool is_addr_only = (adu_op == PB_DIS_OPER || adu_op == PMISC_OPER);

	/* Don't generate fabric command */
	if (adu_op == PRE_SWITCH_AB || adu_op == PRE_SWITCH_CD || adu_op == POST_SWITCH)
		return;

	if (is_addr_only) {
		udelay(10);
	} else {
		put_scom(chip, PU_ALTD_DATA_REG, 0);
		or_scom(chip, PU_ALTD_CMD_REG, PPC_BIT(PU_ALTD_CMD_REG_FBC_START_OP));

		/* If it's not a cache inhibit operation, we just want to delay
		 * for a while and then it's done */
		udelay(10);
	}

	/* We expect the busy bit to be cleared */
	p9_adu_coherent_status_check(chip, is_addr_only);

	/* If it's the last read/write cleanup the ADU */
	put_scom(chip, PU_ALTD_CMD_REG, 0);
}

/* We don't write any specific data to ADU, just execute an action on it */
static void p9_putmemproc(uint8_t chip, uint32_t mem_flags)
{
	enum adu_op adu_op;

	if (mem_flags & SBE_MEM_ACCESS_FLAGS_PB_DIS_MODE)
		adu_op = PB_DIS_OPER;
	else if (mem_flags & SBE_MEM_ACCESS_FLAGS_SWITCH_MODE)
		adu_op = PMISC_OPER;
	else if (mem_flags & SBE_MEM_ACCESS_FLAGS_PRE_SWITCH_CD_MODE)
		adu_op = PRE_SWITCH_CD;
	else if (mem_flags & SBE_MEM_ACCESS_FLAGS_PRE_SWITCH_AB_MODE)
		adu_op = PRE_SWITCH_AB;
	else if (mem_flags & SBE_MEM_ACCESS_FLAGS_POST_SWITCH_MODE)
		adu_op = POST_SWITCH;
	else
		die("Invalid ADU putmem flags.");

	p9_adu_setup(chip, adu_op);
	p9_adu_access(chip, adu_op);
}

static void p9_build_smp_adu_set_switch_action(uint8_t chip, enum build_smp_adu_action action)
{
	uint32_t flags = SBE_MEM_ACCESS_FLAGS_TARGET_PROC;

	if (action == SWITCH_AB)
		flags |= SBE_MEM_ACCESS_FLAGS_PRE_SWITCH_AB_MODE;
	else if (action == SWITCH_CD)
		flags |= SBE_MEM_ACCESS_FLAGS_PRE_SWITCH_CD_MODE;
	else
		flags |= SBE_MEM_ACCESS_FLAGS_POST_SWITCH_MODE;

	return p9_putmemproc(chip, flags);
}

static void p9_build_smp_sequence_adu(enum build_smp_adu_action action)
{
	uint32_t flags = SBE_MEM_ACCESS_FLAGS_TARGET_PROC;

	switch (action) {
	case SWITCH_AB:
	case SWITCH_CD:
		flags |= SBE_MEM_ACCESS_FLAGS_SWITCH_MODE;
		break;
	case QUIESCE:
		flags |= SBE_MEM_ACCESS_FLAGS_PB_DIS_MODE;
		break;
	case RESET_SWITCH:
		die("RESET_SWITCH is not a valid ADU action to request\n");
	}

	/*
	 * Condition for hotplug switch operation. All chips which were not
	 * quiesced prior to switch AB will need to observe the switch.
	 */
	if (action != QUIESCE) {
		p9_build_smp_adu_set_switch_action(/*chip=*/0, action);
		p9_build_smp_adu_set_switch_action(/*chip=*/1, action);
	}

	if (action == SWITCH_CD || action == SWITCH_AB)
		p9_putmemproc(/*chip=*/0, flags);
	if (action == SWITCH_CD || action == QUIESCE)
		p9_putmemproc(/*chip=*/1, flags);

	if (action != QUIESCE) {
		/* Operation complete, reset switch controls */
		p9_build_smp_adu_set_switch_action(/*chip=*/0, RESET_SWITCH);
		p9_build_smp_adu_set_switch_action(/*chip=*/1, RESET_SWITCH);
	}
}

static void p9_fbc_ab_hp_scom(uint8_t chip)
{
	const struct powerbus_cfg *pb_cfg = powerbus_cfg(chip);
	const uint32_t pb_freq_mhz = pb_cfg->fabric_freq;

	/* Frequency of XBus for Nimbus DD2 */
	const uint32_t xbus_freq_mhz = 2000;

	const bool hw407123 = (get_dd() <= 0x20);

	const bool is_fabric_master = (chip == 0);
	const uint8_t attached_chip = (chip == 0 ? 1 : 0);

	const uint64_t cmd_rate_4b_r = ((6 * pb_freq_mhz) % xbus_freq_mhz);

	const uint64_t cmd_rate_d = xbus_freq_mhz;
	const uint64_t cmd_rate_4b_n = (6 * pb_freq_mhz);

	for (uint8_t i = 0; i < P9_BUILD_SMP_NUM_SHADOWS; i++) {
		uint64_t val;
		uint64_t tmp;

		/* *_HP_MODE_NEXT */

		val = get_scom(chip, PB_HP_MODE_NEXT_SHADOWS[i]);

		if (!is_fabric_master) {
			val &= ~PPC_BIT(0); // PB_COM_PB_CFG_MASTER_CHIP_NEXT_OFF
			val &= ~PPC_BIT(1); // PB_COM_PB_CFG_TM_MASTER_NEXT_OFF
		}

		val &= ~PPC_BIT(2); // PB_COM_PB_CFG_CHG_RATE_GP_MASTER_NEXT_OFF

		if (is_fabric_master)
			val |= PPC_BIT(3); // PB_COM_PB_CFG_CHG_RATE_SP_MASTER_NEXT_ON
		else
			val &= ~PPC_BIT(3); // PB_COM_PB_CFG_CHG_RATE_SP_MASTER_NEXT_OFF

		val &= ~PPC_BIT(29); // PB_COM_PB_CFG_HOP_MODE_NEXT_OFF

		put_scom(chip, PB_HP_MODE_NEXT_SHADOWS[i], val);

		/* *_HPX_MODE_NEXT */

		val = get_scom(chip, PB_HPX_MODE_NEXT_SHADOWS[i]);

		val |= PPC_BIT(1); // PB_COM_PB_CFG_LINK_X1_EN_NEXT_ON

		PPC_INSERT(val, attached_chip, 19, 3); // PB_COM_PB_CFG_LINK_X1_CHIPID_NEXT_ID

		val |= PPC_BIT(49); // PB_COM_PB_CFG_X_INDIRECT_EN_NEXT_ON
		val |= PPC_BIT(50); // PB_COM_PB_CFG_X_GATHER_ENABLE_NEXT_ON

		if (cmd_rate_4b_r != 0 && hw407123)
			tmp = (cmd_rate_4b_n / cmd_rate_d) + 3;
		else if (cmd_rate_4b_r == 0 && hw407123)
			tmp = (cmd_rate_4b_n / cmd_rate_d) + 2;
		else if (cmd_rate_4b_r != 0)
			tmp = (cmd_rate_4b_n / cmd_rate_d);
		else
			tmp = (cmd_rate_4b_n / cmd_rate_d) - 1;
		PPC_INSERT(val, tmp, 56, 8);

		put_scom(chip, PB_HPX_MODE_NEXT_SHADOWS[i], val);
	}
}

static uint64_t p9_build_smp_get_hp_ab_shadow(uint8_t chip, const uint64_t shadow_regs[])
{
	uint64_t last_data = 0;

	for (uint8_t i = 0; i < P9_BUILD_SMP_NUM_SHADOWS; i++) {
		const uint64_t data = get_scom(chip, shadow_regs[i]);

		/* Check consistency of west/center/east register copies while
		 * reading them */
		if (i != 0 && data != last_data)
			die("Values in shadow registers differ!\n");

		last_data = data;
	}

	return last_data;
}

static void p9_build_smp_set_hp_ab_shadow(uint8_t chip, const uint64_t shadow_regs[],
					  uint64_t data)
{
	for (uint8_t i = 0; i < P9_BUILD_SMP_NUM_SHADOWS; i++)
		put_scom(chip, shadow_regs[i], data);
}

static void p9_build_smp_copy_hp_ab_next_curr(uint8_t chip)
{
	/* Read NEXT */
	uint64_t hp_mode_data = p9_build_smp_get_hp_ab_shadow(chip, PB_HP_MODE_NEXT_SHADOWS);
	uint64_t hpx_mode_data = p9_build_smp_get_hp_ab_shadow(chip, PB_HPX_MODE_NEXT_SHADOWS);
	uint64_t hpa_mode_data = p9_build_smp_get_hp_ab_shadow(chip, PB_HPA_MODE_NEXT_SHADOWS);

	/* Write CURR */
	p9_build_smp_set_hp_ab_shadow(chip, PB_HP_MODE_CURR_SHADOWS, hp_mode_data);
	p9_build_smp_set_hp_ab_shadow(chip, PB_HPX_MODE_CURR_SHADOWS, hpx_mode_data);
	p9_build_smp_set_hp_ab_shadow(chip, PB_HPA_MODE_CURR_SHADOWS, hpa_mode_data);
}

static void p9_build_smp_copy_hp_ab_curr_next(uint8_t chip)
{
	/* Read CURR */
	uint64_t hp_mode_data  = p9_build_smp_get_hp_ab_shadow(chip, PB_HP_MODE_CURR_SHADOWS);
	uint64_t hpx_mode_data = p9_build_smp_get_hp_ab_shadow(chip, PB_HPX_MODE_CURR_SHADOWS);
	uint64_t hpa_mode_data = p9_build_smp_get_hp_ab_shadow(chip, PB_HPA_MODE_CURR_SHADOWS);

	/* Write NEXT */
	p9_build_smp_set_hp_ab_shadow(chip, PB_HP_MODE_NEXT_SHADOWS, hp_mode_data);
	p9_build_smp_set_hp_ab_shadow(chip, PB_HPX_MODE_NEXT_SHADOWS, hpx_mode_data);
	p9_build_smp_set_hp_ab_shadow(chip, PB_HPA_MODE_NEXT_SHADOWS, hpa_mode_data);
}

static void p9_build_smp_set_fbc_ab(void)
{
	/*
	 * quiesce 'slave' fabrics in preparation for joining
	 *   PHASE1 -> quiesce all chips except the chip which is the new fabric master
	 *   PHASE2 -> quiesce all drawers except the drawer containing the new fabric master
	 */
	p9_build_smp_sequence_adu(QUIESCE);

	/* Program NEXT register set for all chips via initfile */
	p9_fbc_ab_hp_scom(/*chip=*/0);
	p9_fbc_ab_hp_scom(/*chip=*/1);

	/* Program CURR register set only for chips which were just quiesced */
	p9_build_smp_copy_hp_ab_next_curr(/*chip=*/1);

	/*
	 * Issue switch AB reconfiguration from chip designated as new master
	 * (which is guaranteed to be a master now)
	 */
	p9_build_smp_sequence_adu(SWITCH_AB);

	/* Reset NEXT register set (copy CURR->NEXT) for all chips */
	p9_build_smp_copy_hp_ab_curr_next(/*chip=*/0);
	p9_build_smp_copy_hp_ab_curr_next(/*chip=*/1);
}

static void p9_build_smp(void)
{
	/* Apply three CD hotplug sequences to each chip to initialize SCOM
	 * chains */
	for (int seq = 1; seq <= 3; seq++) {
		for (int chip = 0; chip < 2; chip++) {
			if (seq == 1)
				p9_fbc_cd_hp1_scom(chip);
			else
				p9_fbc_cd_hp23_scom(chip, seq);
		}

		/* Issue switch CD on all chips to force updates to occur */
		p9_build_smp_sequence_adu(SWITCH_CD);
	}

	p9_build_smp_set_fbc_ab();
}

void istep_10_1(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 10.1\n");
	report_istep(10,1);

	if (chips != 0x01) {
		p9_build_smp();

		switch_secondary_scom_to_xscom();

		/* Sanity check that XSCOM works for the second CPU */
		if (read_scom(1, 0xF000F) == 0xFFFFFFFFFFFFFFFF)
			die("XSCOM doesn't work for the second CPU\n");

		fsi_reset_pib2opb(/*chip=*/1);
	}

	printk(BIOS_EMERG, "ending istep 10.1\n");
}
