/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <stdint.h>
#include <console/console.h>
#include <string.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static void SetEccWrDQS_D(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat)
{
	u8 ByteLane, DimmNum, OddByte, Addl_Index, Channel;
	u8 EccRef1, EccRef2, EccDQSScale;
	u32 val;
	u16 word;

	for (Channel = 0; Channel < 2; Channel ++) {
		for (DimmNum = 0; DimmNum < C_MAX_DIMMS; DimmNum ++) { /* we use DimmNum instead of DimmNumx3 */
			for (ByteLane = 0; ByteLane < 9; ByteLane ++) {
				/* Get RxEn initial value from WrDqs */
				if (ByteLane & 1)
					OddByte = 1;
				else
					OddByte = 0;
				if (ByteLane < 2)
					Addl_Index = 0x30;
				else if (ByteLane < 4)
					Addl_Index = 0x31;
				else if (ByteLane < 6)
					Addl_Index = 0x40;
				else if (ByteLane < 8)
					Addl_Index = 0x41;
				else
					Addl_Index = 0x32;
				Addl_Index += DimmNum * 3;

				val = Get_NB32_index_wait_DCT(p_dct_stat->dev_dct, Channel, 0x98, Addl_Index);
				if (OddByte)
					val >>= 16;
				/* Save WrDqs to stack for later usage */
				p_dct_stat->persistentData.CH_D_B_TxDqs[Channel][DimmNum][ByteLane] = val & 0xFF;
				EccDQSScale = p_dct_stat->ch_ecc_dqs_scale[Channel];
				word = p_dct_stat->ch_ecc_dqs_like[Channel];
				if ((word & 0xFF) == ByteLane) EccRef1 = val & 0xFF;
				if (((word >> 8) & 0xFF) == ByteLane) EccRef2 = val & 0xFF;
			}
		}
	}
}

static void EnableAutoRefresh_D(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat)
{
	u32 val;

	val = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x8C);
	val &= ~(1 << DIS_AUTO_REFRESH);
	Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x8C, val);

	val = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x8C);
	val &= ~(1 << DIS_AUTO_REFRESH);
	Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x8C, val);
}

static void DisableAutoRefresh_D(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u32 val;

	val = Get_NB32_DCT(p_dct_stat->dev_dct, 0, 0x8C);
	val |= 1 << DIS_AUTO_REFRESH;
	Set_NB32_DCT(p_dct_stat->dev_dct, 0, 0x8C, val);

	val = Get_NB32_DCT(p_dct_stat->dev_dct, 1, 0x8C);
	val |= 1 << DIS_AUTO_REFRESH;
	Set_NB32_DCT(p_dct_stat->dev_dct, 1, 0x8C, val);
}


static u8 PhyWLPass1(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 dimm;
	u16 dimm_valid;
	u8 status = 0;
	void *DCTPtr;

	dct &= 1;

	DCTPtr = (void *)(p_dct_stat->c_dct_ptr[dct]);
	p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[dct];
	p_dct_stat->cs_present = p_dct_stat->cs_present_dct[dct];

	if (p_dct_stat->ganged_mode & 1)
		p_dct_stat->cs_present = p_dct_stat->cs_present_dct[0];

	if (p_dct_stat->dimm_valid) {
		dimm_valid = p_dct_stat->dimm_valid;
		PrepareC_DCT(p_mct_stat, p_dct_stat, dct);
		for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm ++) {
			if (dimm_valid & (1 << (dimm << 1))) {
				status |= AgesaHwWlPhase1(p_mct_stat, p_dct_stat, dct, dimm, FIRST_PASS);
				status |= AgesaHwWlPhase2(p_mct_stat, p_dct_stat, dct, dimm, FIRST_PASS);
				status |= AgesaHwWlPhase3(p_mct_stat, p_dct_stat, dct, dimm, FIRST_PASS);
			}
		}
	}

	return status;
}

static u8 PhyWLPass2(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct, u8 final)
{
	u8 dimm;
	u16 dimm_valid;
	u8 status = 0;
	void *DCTPtr;

	dct &= 1;

	DCTPtr = (void *)&(p_dct_stat->c_dct_ptr[dct]); /* todo: */
	p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[dct];
	p_dct_stat->cs_present = p_dct_stat->cs_present_dct[dct];

	if (p_dct_stat->ganged_mode & 1)
		p_dct_stat->cs_present = p_dct_stat->cs_present_dct[0];

	if (p_dct_stat->dimm_valid) {
		dimm_valid = p_dct_stat->dimm_valid;
		PrepareC_DCT(p_mct_stat, p_dct_stat, dct);
		p_dct_stat->speed = p_dct_stat->dimm_auto_speed = p_dct_stat->target_freq;
		p_dct_stat->cas_latency = p_dct_stat->dimm_casl = p_dct_stat->target_casl;
		SPD2ndTiming(p_mct_stat, p_dct_stat, dct);
		if (!is_fam15h()) {
			ProgDramMRSReg_D(p_mct_stat, p_dct_stat, dct);
			PlatformSpec_D(p_mct_stat, p_dct_stat, dct);
			fence_dyn_training_d(p_mct_stat, p_dct_stat, dct);
		}
		Restore_OnDimmMirror(p_mct_stat, p_dct_stat);
		startup_dct_d(p_mct_stat, p_dct_stat, dct);
		Clear_OnDimmMirror(p_mct_stat, p_dct_stat);
		SetDllSpeedUp_D(p_mct_stat, p_dct_stat, dct);
		DisableAutoRefresh_D(p_mct_stat, p_dct_stat);
		for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm ++) {
			if (dimm_valid & (1 << (dimm << 1))) {
				status |= AgesaHwWlPhase1(p_mct_stat, p_dct_stat, dct, dimm, SECOND_PASS);
				status |= AgesaHwWlPhase2(p_mct_stat, p_dct_stat, dct, dimm, SECOND_PASS);
				status |= AgesaHwWlPhase3(p_mct_stat, p_dct_stat, dct, dimm, SECOND_PASS);
			}
		}
	}

	return status;
}

static u16 fam15h_next_highest_memclk_freq(u16 memclk_freq)
{
	u16 fam15h_next_highest_freq_tab[] = {0, 0, 0, 0, 0x6, 0, 0xa, 0, 0, 0, 0xe, 0, 0, 0, 0x12, 0, 0, 0, 0x16, 0, 0, 0, 0x16};
	return fam15h_next_highest_freq_tab[memclk_freq];
}

/* Write Levelization Training
 * Algorithm detailed in the Fam10h BKDG Rev. 3.62 section 2.8.9.9.1
 */
static void WriteLevelization_HW(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a, u8 Node, u8 Pass)
{
	u8 status;
	u8 timeout;
	u16 final_target_freq;

	struct DCTStatStruc *p_dct_stat;
	p_dct_stat = p_dct_stat_a + Node;

	p_dct_stat->c_mct_ptr  = &(p_dct_stat->s_c_mct_ptr);
	p_dct_stat->c_dct_ptr[0] = &(p_dct_stat->s_c_dct_ptr[0]);
	p_dct_stat->c_dct_ptr[1] = &(p_dct_stat->s_c_dct_ptr[1]);

	/* Disable auto refresh by configuring F2x[1, 0]8C[DIS_AUTO_REFRESH] = 1 */
	DisableAutoRefresh_D(p_mct_stat, p_dct_stat);

	/* Disable ZQ calibration short command by F2x[1,0]94[ZqcsInterval]=00b */
	DisableZQcalibration(p_mct_stat, p_dct_stat);
	PrepareC_MCT(p_mct_stat, p_dct_stat);

	if (p_dct_stat->ganged_mode & (1 << 0)) {
		p_dct_stat->dimm_valid_dct[1] = p_dct_stat->dimm_valid_dct[0];
	}

	if (Pass == FIRST_PASS) {
		timeout = 0;
		do {
			status = 0;
			timeout++;
			status |= PhyWLPass1(p_mct_stat, p_dct_stat, 0);
			status |= PhyWLPass1(p_mct_stat, p_dct_stat, 1);
			if (status)
				printk(BIOS_INFO,
					"%s: Retrying write levelling due to invalid value(s) detected in first phase\n",
					__func__);
		} while (status && (timeout < 8));
		if (status)
			printk(BIOS_INFO,
				"%s: Uncorrectable invalid value(s) detected in first phase of write levelling\n",
				__func__);
	}

	if (Pass == SECOND_PASS) {
		if (p_dct_stat->target_freq > mhz_to_memclk_config(mct_get_nv_bits(NV_MIN_MEMCLK))) {
			/* 8.Prepare the memory subsystem for the target MEMCLK frequency.
			 * NOTE: BIOS must program both DCTs to the same frequency.
			 * NOTE: Fam15h steps the frequency, Fam10h slams the frequency.
			 */
			u8 global_phy_training_status = 0;
			final_target_freq = p_dct_stat->target_freq;

			while (p_dct_stat->speed != final_target_freq) {
				if (is_fam15h())
					p_dct_stat->target_freq = fam15h_next_highest_memclk_freq(p_dct_stat->speed);
				else
					p_dct_stat->target_freq = final_target_freq;
				SetTargetFreq(p_mct_stat, p_dct_stat_a, Node);
				timeout = 0;
				do {
					status = 0;
					timeout++;
					status |= PhyWLPass2(p_mct_stat, p_dct_stat, 0, (p_dct_stat->target_freq == final_target_freq));
					status |= PhyWLPass2(p_mct_stat, p_dct_stat, 1, (p_dct_stat->target_freq == final_target_freq));
					if (status)
						printk(BIOS_INFO,
							"%s: Retrying write levelling due to invalid value(s) detected in last phase\n",
							__func__);
				} while (status && (timeout < 8));
				global_phy_training_status |= status;
			}

			p_dct_stat->target_freq = final_target_freq;

			if (global_phy_training_status)
				printk(BIOS_WARNING,
					"%s: Uncorrectable invalid value(s) detected in second phase of write levelling; continuing but system may be unstable!\n",
					__func__);

			u8 dct;
			for (dct = 0; dct < 2; dct++) {
				sDCTStruct *pDCTData = p_dct_stat->c_dct_ptr[dct];
				memcpy(pDCTData->WLGrossDelayFinalPass, pDCTData->WLGrossDelayPrevPass, sizeof(pDCTData->WLGrossDelayPrevPass));
				memcpy(pDCTData->WLFineDelayFinalPass, pDCTData->WLFineDelayPrevPass, sizeof(pDCTData->WLFineDelayPrevPass));
				pDCTData->WLCriticalGrossDelayFinalPass = pDCTData->WLCriticalGrossDelayPrevPass;
			}
		}
	}

	SetEccWrDQS_D(p_mct_stat, p_dct_stat);
	EnableAutoRefresh_D(p_mct_stat, p_dct_stat);
	EnableZQcalibration(p_mct_stat, p_dct_stat);
}

void mct_WriteLevelization_HW(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a, u8 Pass)
{
	u8 Node;

	for (Node = 0; Node < MAX_NODES_SUPPORTED; Node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + Node;

		if (p_dct_stat->node_present) {
			mctSMBhub_Init(Node);
			Clear_OnDimmMirror(p_mct_stat, p_dct_stat);
			WriteLevelization_HW(p_mct_stat, p_dct_stat_a, Node, Pass);
			Restore_OnDimmMirror(p_mct_stat, p_dct_stat);
		}
	}
}
