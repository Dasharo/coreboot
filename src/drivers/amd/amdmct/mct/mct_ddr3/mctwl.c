/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <stdint.h>
#include <console/console.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static void AgesaDelay(u32 msec)
{
	mct_Wait(msec*10);
}

void PrepareC_MCT(struct MCTStatStruc *pMCTstat,
					struct DCTStatStruc *pDCTstat)
{
	pDCTstat->C_MCTPtr->AgesaDelay = AgesaDelay;
}

void PrepareC_DCT(struct MCTStatStruc *pMCTstat,
					struct DCTStatStruc *pDCTstat, u8 dct)
{
	u8 dimm;
	u16 DimmValid;
	u16 dimm_x8_present;

	dct &= 1;

	pDCTstat->C_DCTPtr[dct]->DctTrain = dct;

	if (dct == 1) {
		dimm_x8_present = pDCTstat->dimm_x8_present >> 1;
	} else
		dimm_x8_present = pDCTstat->dimm_x8_present;
	dimm_x8_present &= 0x55;

	pDCTstat->C_DCTPtr[dct]->MaxDimmsInstalled = pDCTstat->ma_dimms[dct];
	DimmValid = pDCTstat->DIMMValidDCT[dct];

	pDCTstat->C_DCTPtr[dct]->NodeId = pDCTstat->node_id;
	pDCTstat->C_DCTPtr[dct]->LogicalCPUID = pDCTstat->logical_cpuid;

	for (dimm = 0; dimm < MAX_DIMMS; dimm++) {
		if (DimmValid & (1 << (dimm << 1)))
			pDCTstat->C_DCTPtr[dct]->DimmPresent[dimm] = 1;
		if (dimm_x8_present & (1 << (dimm << 1)))
			pDCTstat->C_DCTPtr[dct]->DimmX8Present[dimm] = 1;
	}

	if (pDCTstat->GangedMode & (1 << 0))
		pDCTstat->C_DCTPtr[dct]->CurrDct = 0;
	else
		pDCTstat->C_DCTPtr[dct]->CurrDct = dct;

	pDCTstat->C_DCTPtr[dct]->DctCSPresent = pDCTstat->CSPresent_DCT[dct];
	if (!(pDCTstat->GangedMode & (1 << 0)) && (dct == 1))
		pDCTstat->C_DCTPtr[dct]->DctCSPresent = pDCTstat->CSPresent_DCT[0];

	if (pDCTstat->status & (1 << SB_REGISTERED)) {
		pDCTstat->C_DCTPtr[dct]->status[DCT_STATUS_REGISTERED] = 1;
		pDCTstat->C_DCTPtr[dct]->status[DCT_STATUS_OnDimmMirror] = 0;
	} else {
		if (pDCTstat->MirrPresU_NumRegR > 0)
			pDCTstat->C_DCTPtr[dct]->status[DCT_STATUS_OnDimmMirror] = 1;
		pDCTstat->C_DCTPtr[dct]->status[DCT_STATUS_REGISTERED] = 0;
	}

	if (pDCTstat->status & (1 << SB_LoadReduced)) {
		pDCTstat->C_DCTPtr[dct]->status[DCT_STATUS_LOAD_REDUCED] = 1;
	} else {
		pDCTstat->C_DCTPtr[dct]->status[DCT_STATUS_LOAD_REDUCED] = 0;
	}

	pDCTstat->C_DCTPtr[dct]->RegMan1Present = pDCTstat->RegMan1Present;

	for (dimm = 0; dimm < MAX_TOTAL_DIMMS; dimm++) {
		u8  DimmRanks;
		if (DimmValid & (1 << (dimm << 1))) {
			DimmRanks = 1;
			if (pDCTstat->dimm_dr_present & (1 << ((dimm << 1) + dct)))
				DimmRanks = 2;
			else if (pDCTstat->dimm_qr_present & (1 << ((dimm << 1) + dct)))
				DimmRanks = 4;
		} else
			DimmRanks = 0;
		pDCTstat->C_DCTPtr[dct]->DimmRanks[dimm] = DimmRanks;
	}
}

void EnableZQcalibration(struct MCTStatStruc *pMCTstat, struct DCTStatStruc *pDCTstat)
{
	u32 val;

	val = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x94);
	val |= 1 << 11;
	Set_NB32_DCT(pDCTstat->dev_dct, 0, 0x94, val);

	val = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x94);
	val |= 1 << 11;
	Set_NB32_DCT(pDCTstat->dev_dct, 1, 0x94, val);
}

void DisableZQcalibration(struct MCTStatStruc *pMCTstat,
					struct DCTStatStruc *pDCTstat)
{
	u32 val;

	val = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x94);
	val &= ~(1 << 11);
	val &= ~(1 << 10);
	Set_NB32_DCT(pDCTstat->dev_dct, 0, 0x94, val);

	val = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x94);
	val &= ~(1 << 11);
	val &= ~(1 << 10);
	Set_NB32_DCT(pDCTstat->dev_dct, 1, 0x94, val);
}

static void EnterSelfRefresh(struct MCTStatStruc *pMCTstat,
					struct DCTStatStruc *pDCTstat)
{
	u8 DCT0Present, DCT1Present;
	u32 val;

	DCT0Present = pDCTstat->DIMMValidDCT[0];
	if (pDCTstat->GangedMode)
		DCT1Present = 0;
	else
		DCT1Present = pDCTstat->DIMMValidDCT[1];

	/* Program F2x[1, 0]90[EnterSelfRefresh]=1. */
	if (DCT0Present) {
		val = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x90);
		val |= 1 << EnterSelfRef;
		Set_NB32_DCT(pDCTstat->dev_dct, 0, 0x90, val);
	}
	if (DCT1Present) {
		val = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x90);
		val |= 1 << EnterSelfRef;
		Set_NB32_DCT(pDCTstat->dev_dct, 1, 0x90, val);
	}
	/* Wait until the hardware resets F2x[1, 0]90[EnterSelfRefresh]=0. */
	if (DCT0Present)
		do {
			val = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x90);
		} while (val & (1 <<EnterSelfRef));
	if (DCT1Present)
		do {
			val = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x90);
		} while (val & (1 <<EnterSelfRef));
}

/*
 * Change memclk for write levelization pass 2
 */
static void ChangeMemClk(struct MCTStatStruc *pMCTstat,
					struct DCTStatStruc *pDCTstat)
{
	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	u8 DCT0Present;
	u8 DCT1Present;
	u32 dword;
	u32 mask;
	u32 offset;

	DCT0Present = pDCTstat->DIMMValidDCT[0];
	if (pDCTstat->GangedMode)
		DCT1Present = 0;
	else
		DCT1Present = pDCTstat->DIMMValidDCT[1];

	if (is_fam15h()) {
		/* Program D18F2x9C_x0D0F_E006_dct[1:0][PllLockTime] = 0x190 */
		if (DCT0Present) {
			dword = Get_NB32_index_wait_DCT(pDCTstat->dev_dct, 0, 0x98, 0x0d0fe006);
			dword &= ~(0x0000ffff);
			dword |= 0x00000190;
			Set_NB32_index_wait_DCT(pDCTstat->dev_dct, 0, 0x98, 0x0d0fe006, dword);
		}
		if (DCT1Present) {
			dword = Get_NB32_index_wait_DCT(pDCTstat->dev_dct, 1, 0x98, 0x0d0fe006);
			dword &= ~(0x0000ffff);
			dword |= 0x00000190;
			Set_NB32_index_wait_DCT(pDCTstat->dev_dct, 1, 0x98, 0x0d0fe006, dword);
		}
	} else {
		/* Program F2x[1, 0]9C[DisAutoComp]=1. */
		if (DCT0Present) {
			dword = Get_NB32_index_wait_DCT(pDCTstat->dev_dct, 0, 0x98, 8);
			dword |= 1 << DisAutoComp;
			Set_NB32_index_wait_DCT(pDCTstat->dev_dct, 0, 0x98, 8, dword);
			mct_Wait(100);	/* Wait for 5us */
		}
		if (DCT1Present) {
			dword = Get_NB32_index_wait_DCT(pDCTstat->dev_dct, 1, 0x98, 8);
			dword |= 1 << DisAutoComp;
			Set_NB32_index_wait_DCT(pDCTstat->dev_dct, 1, 0x98, 8, dword);
			mct_Wait(100);	/* Wait for 5us */
		}
	}

	/* Program F2x[1, 0]94[MEM_CLK_FREQ_VAL] = 0. */
	if (DCT0Present) {
		dword = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x94);
		dword &= ~(1 << MEM_CLK_FREQ_VAL);
		Set_NB32_DCT(pDCTstat->dev_dct, 0, 0x94, dword);
	}
	if (DCT1Present) {
		dword = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x94);
		dword &= ~(1 << MEM_CLK_FREQ_VAL);
		Set_NB32_DCT(pDCTstat->dev_dct, 1, 0x94, dword);
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
		dword = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x94);
		dword &= ~mask;
		dword |= (pDCTstat->TargetFreq - offset) & mask;
		Set_NB32_DCT(pDCTstat->dev_dct, 0, 0x94, dword);
	}
	if (DCT1Present) {
		dword = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x94);
		dword &= ~mask;
		dword |= (pDCTstat->TargetFreq - offset) & mask;
		Set_NB32_DCT(pDCTstat->dev_dct, 1, 0x94, dword);
	}

	if (is_fam15h()) {
		if (DCT0Present) {
			mctGet_PS_Cfg_D(pMCTstat, pDCTstat, 0);
			set_2t_configuration(pMCTstat, pDCTstat, 0);
			mct_BeforePlatformSpec(pMCTstat, pDCTstat, 0);
			mct_PlatformSpec(pMCTstat, pDCTstat, 0);
		}
		if (DCT1Present) {
			mctGet_PS_Cfg_D(pMCTstat, pDCTstat, 1);
			set_2t_configuration(pMCTstat, pDCTstat, 1);
			mct_BeforePlatformSpec(pMCTstat, pDCTstat, 1);
			mct_PlatformSpec(pMCTstat, pDCTstat, 1);
		}
	}

	/* Program F2x[1, 0]94[MEM_CLK_FREQ_VAL] = 1. */
	if (DCT0Present) {
		dword = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x94);
		dword |= 1 << MEM_CLK_FREQ_VAL;
		Set_NB32_DCT(pDCTstat->dev_dct, 0, 0x94, dword);
	}
	if (DCT1Present) {
		dword = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x94);
		dword |= 1 << MEM_CLK_FREQ_VAL;
		Set_NB32_DCT(pDCTstat->dev_dct, 1, 0x94, dword);
	}

	/* Wait until F2x[1, 0]94[FreqChgInProg]=0. */
	if (DCT0Present)
		do {
			dword = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x94);
		} while (dword & (1 << FreqChgInProg));
	if (DCT1Present)
		do {
			dword = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x94);
		} while (dword & (1 << FreqChgInProg));

	if (is_fam15h()) {
		/* Program D18F2x9C_x0D0F_E006_dct[1:0][PllLockTime] = 0xf */
		if (DCT0Present) {
			dword = Get_NB32_index_wait_DCT(pDCTstat->dev_dct, 0, 0x98, 0x0d0fe006);
			dword &= ~(0x0000ffff);
			dword |= 0x0000000f;
			Set_NB32_index_wait_DCT(pDCTstat->dev_dct, 0, 0x98, 0x0d0fe006, dword);
		}
		if (DCT1Present) {
			dword = Get_NB32_index_wait_DCT(pDCTstat->dev_dct, 1, 0x98, 0x0d0fe006);
			dword &= ~(0x0000ffff);
			dword |= 0x0000000f;
			Set_NB32_index_wait_DCT(pDCTstat->dev_dct, 1, 0x98, 0x0d0fe006, dword);
		}
	} else {
		/* Program F2x[1, 0]9C[DisAutoComp] = 0. */
		if (DCT0Present) {
			dword = Get_NB32_index_wait_DCT(pDCTstat->dev_dct, 0, 0x98, 8);
			dword &= ~(1 << DisAutoComp);
			Set_NB32_index_wait_DCT(pDCTstat->dev_dct, 0, 0x98, 8, dword);
			mct_Wait(15000);	/* Wait for 750us */
		}
		if (DCT1Present) {
			dword = Get_NB32_index_wait_DCT(pDCTstat->dev_dct, 1, 0x98, 8);
			dword &= ~(1 << DisAutoComp);
			Set_NB32_index_wait_DCT(pDCTstat->dev_dct, 1, 0x98, 8, dword);
			mct_Wait(15000);	/* Wait for 750us */
		}
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

/*
 * the DRAM controller to bring the DRAMs out of self refresh mode.
 */
static void ExitSelfRefresh(struct MCTStatStruc *pMCTstat,
					struct DCTStatStruc *pDCTstat)
{
	u8 DCT0Present, DCT1Present;
	u32 val;

	DCT0Present = pDCTstat->DIMMValidDCT[0];
	if (pDCTstat->GangedMode)
		DCT1Present = 0;
	else
		DCT1Present = pDCTstat->DIMMValidDCT[1];

	/* Program F2x[1, 0]90[ExitSelfRef]=1 for both DCTs. */
	if (DCT0Present) {
		val = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x90);
		val |= 1 << ExitSelfRef;
		Set_NB32_DCT(pDCTstat->dev_dct, 0, 0x90, val);
	}
	if (DCT1Present) {
		val = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x90);
		val |= 1 << ExitSelfRef;
		Set_NB32_DCT(pDCTstat->dev_dct, 1, 0x90, val);
	}
	/* Wait until the hardware resets F2x[1, 0]90[ExitSelfRef]=0. */
	if (DCT0Present)
		do {
			val = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x90);
		} while (val & (1 << ExitSelfRef));
	if (DCT1Present)
		do {
			val = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x90);
		} while (val & (1 << ExitSelfRef));
}

void SetTargetFreq(struct MCTStatStruc *pMCTstat,
					struct DCTStatStruc *pDCTstatA, u8 Node)
{
	u32 dword;
	u8 package_type = mctGet_NVbits(NV_PACK_TYPE);

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	struct DCTStatStruc *pDCTstat;
	pDCTstat = pDCTstatA + Node;

	printk(BIOS_DEBUG, "%s: Node %d: New frequency code: %04x\n", __func__, Node, pDCTstat->TargetFreq);

	if (is_fam15h()) {
		/* Program F2x[1, 0]90[DisDllShutDownSR]=1. */
		if (pDCTstat->DIMMValidDCT[0]) {
			dword = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x90);
			dword |= (0x1 << 27);
			Set_NB32_DCT(pDCTstat->dev_dct, 0, 0x90, dword);
		}
		if (pDCTstat->DIMMValidDCT[1]) {
			dword = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x90);
			dword |= (0x1 << 27);
			Set_NB32_DCT(pDCTstat->dev_dct, 1, 0x90, dword);
		}
	}

	/* Program F2x[1,0]90[EnterSelfRefresh]=1.
	 * Wait until the hardware resets F2x[1,0]90[EnterSelfRefresh]=0.
	 */
	EnterSelfRefresh(pMCTstat, pDCTstat);

	/*
	 * Program F2x[1,0]9C_x08[DisAutoComp]=1
	 * Program F2x[1,0]94[MEM_CLK_FREQ_VAL] = 0.
	 * Program F2x[1,0]94[MemClkFreq] to specify the target MEMCLK frequency.
	 * Program F2x[1,0]94[MEM_CLK_FREQ_VAL] = 1.
	 * Wait until F2x[1,0]94[FreqChgInProg]=0.
	 * Program F2x[1,0]9C_x08[DisAutoComp]=0
	 */
	ChangeMemClk(pMCTstat, pDCTstat);

	if (is_fam15h()) {
		u8 dct;
		for (dct = 0; dct < 2; dct++) {
			if (pDCTstat->DIMMValidDCT[dct]) {
				phyAssistedMemFnceTraining(pMCTstat, pDCTstatA, Node);
				InitPhyCompensation(pMCTstat, pDCTstat, dct);
			}
		}
	}

	/* Program F2x[1,0]90[ExitSelfRef]=1 for both DCTs.
	 * Wait until the hardware resets F2x[1, 0]90[ExitSelfRef]=0.
	 */
	ExitSelfRefresh(pMCTstat, pDCTstat);

	if (is_fam15h()) {
		if ((package_type == PT_C3) || (package_type == PT_GR)) {
			/* Socket C32 or G34 */
			/* Program F2x[1, 0]90[DisDllShutDownSR]=0. */
			if (pDCTstat->DIMMValidDCT[0]) {
				dword = Get_NB32_DCT(pDCTstat->dev_dct, 0, 0x90);
				dword &= ~(0x1 << 27);
				Set_NB32_DCT(pDCTstat->dev_dct, 0, 0x90, dword);
			}
			if (pDCTstat->DIMMValidDCT[1]) {
				dword = Get_NB32_DCT(pDCTstat->dev_dct, 1, 0x90);
				dword &= ~(0x1 << 27);
				Set_NB32_DCT(pDCTstat->dev_dct, 1, 0x90, dword);
			}
		}
	}

	/* wait for 500 MCLKs after ExitSelfRef, 500*2.5ns = 1250ns */
	mct_Wait(250);

	if (pDCTstat->status & (1 << SB_REGISTERED)) {
		u8 DCT0Present, DCT1Present;

		DCT0Present = pDCTstat->DIMMValidDCT[0];
		if (pDCTstat->GangedMode)
			DCT1Present = 0;
		else
			DCT1Present = pDCTstat->DIMMValidDCT[1];

		if (!DCT1Present)
			pDCTstat->cs_present = pDCTstat->CSPresent_DCT[0];
		else if (pDCTstat->GangedMode)
			pDCTstat->cs_present = 0;
		else
			pDCTstat->cs_present = pDCTstat->CSPresent_DCT[1];

		if (pDCTstat->DIMMValidDCT[0]) {
			FreqChgCtrlWrd(pMCTstat, pDCTstat, 0);
		}
		if (pDCTstat->DIMMValidDCT[1]) {
			FreqChgCtrlWrd(pMCTstat, pDCTstat, 1);
		}
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

static void Modify_OnDimmMirror(struct DCTStatStruc *pDCTstat, u8 dct, u8 set)
{
	u32 val;
	u32 reg = 0x44;
	while (reg < 0x60) {
		val = Get_NB32_DCT(pDCTstat->dev_dct, dct, reg);
		if (val & (1 << CS_ENABLE))
			set ? (val |= 1 << onDimmMirror) : (val &= ~(1 << onDimmMirror));
		Set_NB32_DCT(pDCTstat->dev_dct, dct, reg, val);
		reg += 8;
	}
}

void Restore_OnDimmMirror(struct MCTStatStruc *pMCTstat,
				struct DCTStatStruc *pDCTstat)
{
	if (pDCTstat->logical_cpuid & (AMD_DR_Bx /* | AMD_RB_C0 */)) { /* We dont support RB-C0 now */
		if (pDCTstat->MirrPresU_NumRegR & 0x55)
			Modify_OnDimmMirror(pDCTstat, 0, 1); /* dct = 0, set */
		if (pDCTstat->MirrPresU_NumRegR & 0xAA)
			Modify_OnDimmMirror(pDCTstat, 1, 1); /* dct = 1, set */
	}
}
void Clear_OnDimmMirror(struct MCTStatStruc *pMCTstat,
				struct DCTStatStruc *pDCTstat)
{
	if (pDCTstat->logical_cpuid & (AMD_DR_Bx /* | AMD_RB_C0 */)) { /* We dont support RB-C0 now */
		if (pDCTstat->MirrPresU_NumRegR & 0x55)
			Modify_OnDimmMirror(pDCTstat, 0, 0); /* dct = 0, clear */
		if (pDCTstat->MirrPresU_NumRegR & 0xAA)
			Modify_OnDimmMirror(pDCTstat, 1, 0); /* dct = 1, clear */
	}
}
