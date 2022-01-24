/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <stdint.h>
#include <console/console.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static void AgesaDelay(u32 msec)
{
	mct_wait(msec*10);
}

void PrepareC_MCT(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	p_dct_stat->c_mct_ptr->AgesaDelay = AgesaDelay;
}

void PrepareC_DCT(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 dimm;
	u16 DimmValid;
	u16 dimm_x8_present;

	dct &= 1;

	p_dct_stat->c_dct_ptr[dct]->DctTrain = dct;

	if (dct == 1) {
		dimm_x8_present = p_dct_stat->dimm_x8_present >> 1;
	} else
		dimm_x8_present = p_dct_stat->dimm_x8_present;
	dimm_x8_present &= 0x55;

	p_dct_stat->c_dct_ptr[dct]->MaxDimmsInstalled = p_dct_stat->ma_dimms[dct];
	DimmValid = p_dct_stat->dimm_valid_dct[dct];

	p_dct_stat->c_dct_ptr[dct]->NodeId = p_dct_stat->node_id;
	p_dct_stat->c_dct_ptr[dct]->logical_cpuid = p_dct_stat->logical_cpuid;

	for (dimm = 0; dimm < MAX_DIMMS; dimm++) {
		if (DimmValid & (1 << (dimm << 1)))
			p_dct_stat->c_dct_ptr[dct]->DimmPresent[dimm] = 1;
		if (dimm_x8_present & (1 << (dimm << 1)))
			p_dct_stat->c_dct_ptr[dct]->DimmX8Present[dimm] = 1;
	}

	if (p_dct_stat->ganged_mode & (1 << 0))
		p_dct_stat->c_dct_ptr[dct]->CurrDct = 0;
	else
		p_dct_stat->c_dct_ptr[dct]->CurrDct = dct;

	p_dct_stat->c_dct_ptr[dct]->DctCSPresent = p_dct_stat->cs_present_dct[dct];
	if (!(p_dct_stat->ganged_mode & (1 << 0)) && (dct == 1))
		p_dct_stat->c_dct_ptr[dct]->DctCSPresent = p_dct_stat->cs_present_dct[0];

	if (p_dct_stat->status & (1 << SB_REGISTERED)) {
		p_dct_stat->c_dct_ptr[dct]->status[DCT_STATUS_REGISTERED] = 1;
		p_dct_stat->c_dct_ptr[dct]->status[DCT_STATUS_OnDimmMirror] = 0;
	} else {
		if (p_dct_stat->mirr_pres_u_num_reg_r > 0)
			p_dct_stat->c_dct_ptr[dct]->status[DCT_STATUS_OnDimmMirror] = 1;
		p_dct_stat->c_dct_ptr[dct]->status[DCT_STATUS_REGISTERED] = 0;
	}

	if (p_dct_stat->status & (1 << SB_LoadReduced)) {
		p_dct_stat->c_dct_ptr[dct]->status[DCT_STATUS_LOAD_REDUCED] = 1;
	} else {
		p_dct_stat->c_dct_ptr[dct]->status[DCT_STATUS_LOAD_REDUCED] = 0;
	}

	p_dct_stat->c_dct_ptr[dct]->reg_man_1_present = p_dct_stat->reg_man_1_present;

	for (dimm = 0; dimm < MAX_TOTAL_DIMMS; dimm++) {
		u8  dimm_ranks;
		if (DimmValid & (1 << (dimm << 1))) {
			dimm_ranks = 1;
			if (p_dct_stat->dimm_dr_present & (1 << ((dimm << 1) + dct)))
				dimm_ranks = 2;
			else if (p_dct_stat->dimm_qr_present & (1 << ((dimm << 1) + dct)))
				dimm_ranks = 4;
		} else
			dimm_ranks = 0;
		p_dct_stat->c_dct_ptr[dct]->dimm_ranks[dimm] = dimm_ranks;
	}
}

void EnableZQcalibration(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat)
{
	u32 val;

	val = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94);
	val |= 1 << 11;
	Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94, val);

	val = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94);
	val |= 1 << 11;
	Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94, val);
}

void DisableZQcalibration(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u32 val;

	val = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94);
	val &= ~(1 << 11);
	val &= ~(1 << 10);
	Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94, val);

	val = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94);
	val &= ~(1 << 11);
	val &= ~(1 << 10);
	Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94, val);
}

static void EnterSelfRefresh(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u8 DCT0Present, DCT1Present;
	u32 val;

	DCT0Present = p_dct_stat->dimm_valid_dct[0];
	if (p_dct_stat->ganged_mode)
		DCT1Present = 0;
	else
		DCT1Present = p_dct_stat->dimm_valid_dct[1];

	/* Program F2x[1, 0]90[EnterSelfRefresh]=1. */
	if (DCT0Present) {
		val = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90);
		val |= 1 << ENTER_SELF_REF;
		Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90, val);
	}
	if (DCT1Present) {
		val = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90);
		val |= 1 << ENTER_SELF_REF;
		Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90, val);
	}
	/* Wait until the hardware resets F2x[1, 0]90[EnterSelfRefresh]=0. */
	if (DCT0Present)
		do {
			val = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90);
		} while (val & (1 <<ENTER_SELF_REF));
	if (DCT1Present)
		do {
			val = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90);
		} while (val & (1 <<ENTER_SELF_REF));
}

/*
 * Change memclk for write levelization pass 2
 */
static void ChangeMemClk(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	u8 DCT0Present;
	u8 DCT1Present;
	u32 dword;
	u32 mask;
	u32 offset;

	DCT0Present = p_dct_stat->dimm_valid_dct[0];
	if (p_dct_stat->ganged_mode)
		DCT1Present = 0;
	else
		DCT1Present = p_dct_stat->dimm_valid_dct[1];

	if (is_fam15h()) {
		/* Program D18F2x9C_x0D0F_E006_dct[1:0][PllLockTime] = 0x190 */
		if (DCT0Present) {
			dword = Get_NB32_index_wait_DCT(p_dct_stat->dev_dct, 0, 0x98, 0x0d0fe006);
			dword &= ~(0x0000ffff);
			dword |= 0x00000190;
			Set_NB32_index_wait_DCT(p_dct_stat->dev_dct, 0, 0x98, 0x0d0fe006, dword);
		}
		if (DCT1Present) {
			dword = Get_NB32_index_wait_DCT(p_dct_stat->dev_dct, 1, 0x98, 0x0d0fe006);
			dword &= ~(0x0000ffff);
			dword |= 0x00000190;
			Set_NB32_index_wait_DCT(p_dct_stat->dev_dct, 1, 0x98, 0x0d0fe006, dword);
		}
	} else {
		/* Program F2x[1, 0]9C[DIS_AUTO_COMP]=1. */
		if (DCT0Present) {
			dword = Get_NB32_index_wait_DCT(p_dct_stat->dev_dct, 0, 0x98, 8);
			dword |= 1 << DIS_AUTO_COMP;
			Set_NB32_index_wait_DCT(p_dct_stat->dev_dct, 0, 0x98, 8, dword);
			mct_wait(100);	/* Wait for 5us */
		}
		if (DCT1Present) {
			dword = Get_NB32_index_wait_DCT(p_dct_stat->dev_dct, 1, 0x98, 8);
			dword |= 1 << DIS_AUTO_COMP;
			Set_NB32_index_wait_DCT(p_dct_stat->dev_dct, 1, 0x98, 8, dword);
			mct_wait(100);	/* Wait for 5us */
		}
	}

	/* Program F2x[1, 0]94[MEM_CLK_FREQ_VAL] = 0. */
	if (DCT0Present) {
		dword = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94);
		dword &= ~(1 << MEM_CLK_FREQ_VAL);
		Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94, dword);
	}
	if (DCT1Present) {
		dword = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94);
		dword &= ~(1 << MEM_CLK_FREQ_VAL);
		Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94, dword);
	}

	/* Program F2x[1, 0]94[MemClkFreq] to specify the target MEMCLK frequency. */
	if (is_fam15h()) {
		offset = 0x0;
		mask = 0x1f;
	} else {
		offset = 0x1;
		mask = 0x7;
	}
	if (DCT0Present) {
		dword = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94);
		dword &= ~mask;
		dword |= (p_dct_stat->target_freq - offset) & mask;
		Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94, dword);
	}
	if (DCT1Present) {
		dword = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94);
		dword &= ~mask;
		dword |= (p_dct_stat->target_freq - offset) & mask;
		Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94, dword);
	}

	if (is_fam15h()) {
		if (DCT0Present) {
			mct_get_ps_cfg_d(p_mct_stat, p_dct_stat, 0);
			set_2t_configuration(p_mct_stat, p_dct_stat, 0);
			mct_BeforePlatformSpec(p_mct_stat, p_dct_stat, 0);
			mct_PlatformSpec(p_mct_stat, p_dct_stat, 0);
		}
		if (DCT1Present) {
			mct_get_ps_cfg_d(p_mct_stat, p_dct_stat, 1);
			set_2t_configuration(p_mct_stat, p_dct_stat, 1);
			mct_BeforePlatformSpec(p_mct_stat, p_dct_stat, 1);
			mct_PlatformSpec(p_mct_stat, p_dct_stat, 1);
		}
	}

	/* Program F2x[1, 0]94[MEM_CLK_FREQ_VAL] = 1. */
	if (DCT0Present) {
		dword = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94);
		dword |= 1 << MEM_CLK_FREQ_VAL;
		Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94, dword);
	}
	if (DCT1Present) {
		dword = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94);
		dword |= 1 << MEM_CLK_FREQ_VAL;
		Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94, dword);
	}

	/* Wait until F2x[1, 0]94[FREQ_CHG_IN_PROG]=0. */
	if (DCT0Present)
		do {
			dword = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x94);
		} while (dword & (1 << FREQ_CHG_IN_PROG));
	if (DCT1Present)
		do {
			dword = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x94);
		} while (dword & (1 << FREQ_CHG_IN_PROG));

	if (is_fam15h()) {
		/* Program D18F2x9C_x0D0F_E006_dct[1:0][PllLockTime] = 0xf */
		if (DCT0Present) {
			dword = Get_NB32_index_wait_DCT(p_dct_stat->dev_dct, 0, 0x98, 0x0d0fe006);
			dword &= ~(0x0000ffff);
			dword |= 0x0000000f;
			Set_NB32_index_wait_DCT(p_dct_stat->dev_dct, 0, 0x98, 0x0d0fe006, dword);
		}
		if (DCT1Present) {
			dword = Get_NB32_index_wait_DCT(p_dct_stat->dev_dct, 1, 0x98, 0x0d0fe006);
			dword &= ~(0x0000ffff);
			dword |= 0x0000000f;
			Set_NB32_index_wait_DCT(p_dct_stat->dev_dct, 1, 0x98, 0x0d0fe006, dword);
		}
	} else {
		/* Program F2x[1, 0]9C[DIS_AUTO_COMP] = 0. */
		if (DCT0Present) {
			dword = Get_NB32_index_wait_DCT(p_dct_stat->dev_dct, 0, 0x98, 8);
			dword &= ~(1 << DIS_AUTO_COMP);
			Set_NB32_index_wait_DCT(p_dct_stat->dev_dct, 0, 0x98, 8, dword);
			mct_wait(15000);	/* Wait for 750us */
		}
		if (DCT1Present) {
			dword = Get_NB32_index_wait_DCT(p_dct_stat->dev_dct, 1, 0x98, 8);
			dword &= ~(1 << DIS_AUTO_COMP);
			Set_NB32_index_wait_DCT(p_dct_stat->dev_dct, 1, 0x98, 8, dword);
			mct_wait(15000);	/* Wait for 750us */
		}
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

/*
 * the DRAM controller to bring the DRAMs out of self refresh mode.
 */
static void ExitSelfRefresh(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u8 DCT0Present, DCT1Present;
	u32 val;

	DCT0Present = p_dct_stat->dimm_valid_dct[0];
	if (p_dct_stat->ganged_mode)
		DCT1Present = 0;
	else
		DCT1Present = p_dct_stat->dimm_valid_dct[1];

	/* Program F2x[1, 0]90[EXIT_SELF_REF]=1 for both DCTs. */
	if (DCT0Present) {
		val = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90);
		val |= 1 << EXIT_SELF_REF;
		Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90, val);
	}
	if (DCT1Present) {
		val = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90);
		val |= 1 << EXIT_SELF_REF;
		Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90, val);
	}
	/* Wait until the hardware resets F2x[1, 0]90[EXIT_SELF_REF]=0. */
	if (DCT0Present)
		do {
			val = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90);
		} while (val & (1 << EXIT_SELF_REF));
	if (DCT1Present)
		do {
			val = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90);
		} while (val & (1 << EXIT_SELF_REF));
}

void SetTargetFreq(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a, u8 Node)
{
	u32 dword;
	u8 package_type = mct_get_nv_bits(NV_PACK_TYPE);

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	struct DCTStatStruc *p_dct_stat;
	p_dct_stat = p_dct_stat_a + Node;

	printk(BIOS_DEBUG, "%s: Node %d: New frequency code: %04x\n", __func__, Node, p_dct_stat->target_freq);

	if (is_fam15h()) {
		/* Program F2x[1, 0]90[DisDllShutDownSR]=1. */
		if (p_dct_stat->dimm_valid_dct[0]) {
			dword = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90);
			dword |= (0x1 << 27);
			Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90, dword);
		}
		if (p_dct_stat->dimm_valid_dct[1]) {
			dword = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90);
			dword |= (0x1 << 27);
			Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90, dword);
		}
	}

	/* Program F2x[1,0]90[EnterSelfRefresh]=1.
	 * Wait until the hardware resets F2x[1,0]90[EnterSelfRefresh]=0.
	 */
	EnterSelfRefresh(p_mct_stat, p_dct_stat);

	/*
	 * Program F2x[1,0]9C_x08[DIS_AUTO_COMP]=1
	 * Program F2x[1,0]94[MEM_CLK_FREQ_VAL] = 0.
	 * Program F2x[1,0]94[MemClkFreq] to specify the target MEMCLK frequency.
	 * Program F2x[1,0]94[MEM_CLK_FREQ_VAL] = 1.
	 * Wait until F2x[1,0]94[FREQ_CHG_IN_PROG]=0.
	 * Program F2x[1,0]9C_x08[DIS_AUTO_COMP]=0
	 */
	ChangeMemClk(p_mct_stat, p_dct_stat);

	if (is_fam15h()) {
		u8 dct;
		for (dct = 0; dct < 2; dct++) {
			if (p_dct_stat->dimm_valid_dct[dct]) {
				phy_assisted_mem_fence_training(p_mct_stat, p_dct_stat_a, Node);
				InitPhyCompensation(p_mct_stat, p_dct_stat, dct);
			}
		}
	}

	/* Program F2x[1,0]90[EXIT_SELF_REF]=1 for both DCTs.
	 * Wait until the hardware resets F2x[1, 0]90[EXIT_SELF_REF]=0.
	 */
	ExitSelfRefresh(p_mct_stat, p_dct_stat);

	if (is_fam15h()) {
		if ((package_type == PT_C3) || (package_type == PT_GR)) {
			/* Socket C32 or G34 */
			/* Program F2x[1, 0]90[DisDllShutDownSR]=0. */
			if (p_dct_stat->dimm_valid_dct[0]) {
				dword = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90);
				dword &= ~(0x1 << 27);
				Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x90, dword);
			}
			if (p_dct_stat->dimm_valid_dct[1]) {
				dword = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90);
				dword &= ~(0x1 << 27);
				Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x90, dword);
			}
		}
	}

	/* wait for 500 MCLKs after EXIT_SELF_REF, 500*2.5ns = 1250ns */
	mct_wait(250);

	if (p_dct_stat->status & (1 << SB_REGISTERED)) {
		u8 DCT0Present, DCT1Present;

		DCT0Present = p_dct_stat->dimm_valid_dct[0];
		if (p_dct_stat->ganged_mode)
			DCT1Present = 0;
		else
			DCT1Present = p_dct_stat->dimm_valid_dct[1];

		if (!DCT1Present)
			p_dct_stat->cs_present = p_dct_stat->cs_present_dct[0];
		else if (p_dct_stat->ganged_mode)
			p_dct_stat->cs_present = 0;
		else
			p_dct_stat->cs_present = p_dct_stat->cs_present_dct[1];

		if (p_dct_stat->dimm_valid_dct[0]) {
			FreqChgCtrlWrd(p_mct_stat, p_dct_stat, 0);
		}
		if (p_dct_stat->dimm_valid_dct[1]) {
			FreqChgCtrlWrd(p_mct_stat, p_dct_stat, 1);
		}
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

static void Modify_OnDimmMirror(struct DCTStatStruc *p_dct_stat, u8 dct, u8 set)
{
	u32 val;
	u32 reg = 0x44;
	while (reg < 0x60) {
		val = Get_NB32_DCT(p_dct_stat->dev_dct, dct, reg);
		if (val & (1 << CS_ENABLE))
			set ? (val |= 1 << ON_DIMM_MIRROR) : (val &= ~(1 << ON_DIMM_MIRROR));
		Set_NB32_DCT(p_dct_stat->dev_dct, dct, reg, val);
		reg += 8;
	}
}

void Restore_OnDimmMirror(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	if (p_dct_stat->logical_cpuid & (AMD_DR_Bx /* | AMD_RB_C0 */)) { /* We dont support RB-C0 now */
		if (p_dct_stat->mirr_pres_u_num_reg_r & 0x55)
			Modify_OnDimmMirror(p_dct_stat, 0, 1); /* dct = 0, set */
		if (p_dct_stat->mirr_pres_u_num_reg_r & 0xAA)
			Modify_OnDimmMirror(p_dct_stat, 1, 1); /* dct = 1, set */
	}
}
void Clear_OnDimmMirror(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	if (p_dct_stat->logical_cpuid & (AMD_DR_Bx /* | AMD_RB_C0 */)) { /* We dont support RB-C0 now */
		if (p_dct_stat->mirr_pres_u_num_reg_r & 0x55)
			Modify_OnDimmMirror(p_dct_stat, 0, 0); /* dct = 0, clear */
		if (p_dct_stat->mirr_pres_u_num_reg_r & 0xAA)
			Modify_OnDimmMirror(p_dct_stat, 1, 0); /* dct = 1, clear */
	}
}
