/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <assert.h>
#include <console/console.h>
#include <northbridge/amd/amdfam10/amdfam10.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"
#include "mwlc_d.h"

u32 swap_addr_bits_wl(struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_value);
u32 swap_bank_bits(struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_value);
void prepare_dimms(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat,
	u8 dct, u8 dimm, bool wl);
void program_odt(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 dct, u8 dimm);
void proc_config(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 dct, u8 dimm, u8 pass, u8 nibble);
void set_wl_byte_delay(struct DCTStatStruc *p_dct_stat, u8 dct, u8 byte_lane, u8 dimm, u8 target_addr, u8 pass, u8 lane_count);
void getWLByteDelay(struct DCTStatStruc *p_dct_stat, u8 dct, u8 byte_lane, u8 dimm, u8 pass, u8 nibble, u8 lane_count);

#define MAX_LANE_COUNT 9

/*-----------------------------------------------------------------------------
 * u8 agesa_hw_wl_phase1(SPDStruct *SPDData,MCTStruct *MCTData, DCTStruct *DCTData,
 *                  u8 Dimm, u8 Pass)
 *
 *  Description:
 *       This function initialized Hardware based write levelization phase 1
 *
 *   Parameters:
 *       IN  OUT   *SPDData - Pointer to buffer with information about each DIMMs
 *                            SPD information
 *                 *MCTData - Pointer to buffer with runtime parameters,
 *                 *DCTData - Pointer to buffer with information about each DCT
 *
 *       IN        DIMM - Logical DIMM number
 *                 Pass - First or Second Pass
 *       OUT
 *-----------------------------------------------------------------------------
 */
u8 agesa_hw_wl_phase1(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat,
		u8 dct, u8 dimm, u8 pass)
{
	u8 byte_lane;
	u32 value, addr;
	u8 nibble = 0;
	u8 train_both_nibbles;
	u16 addl_data_offset, addl_data_port;
	sMCTStruct *p_mct_data = p_dct_stat->c_mct_ptr;
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];
	u8 lane_count;

	lane_count = get_available_lane_count(p_mct_stat, p_dct_stat);

	pdct_data->wl_pass = pass;
	/* 1. Specify the target DIMM that is to be trained by programming
	 * F2x[1, 0]9C_x08[TrDimmSel].
	 */
	set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
			DRAM_ADD_DCT_PHY_CONTROL_REG, TR_DIMM_SEL_START,
			TR_DIMM_SEL_END, (u32)dimm);

	train_both_nibbles = 0;
	if (p_dct_stat->dimm_x4_present)
		if (is_fam15h())
			train_both_nibbles = 1;

	for (nibble = 0; nibble < (train_both_nibbles + 1); nibble++) {
		printk(BIOS_SPEW, "agesa_hw_wl_phase1: training nibble %d\n", nibble);

		if (is_fam15h()) {
			/* Program F2x[1, 0]9C_x08[WRT_LV_TR_EN]=0 */
			set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
					DRAM_ADD_DCT_PHY_CONTROL_REG, WRT_LV_TR_EN, WRT_LV_TR_EN, 0);

			/* Set TR_NIBBLE_SEL */
			set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
					DRAM_ADD_DCT_PHY_CONTROL_REG, 2,
					2, (u32)nibble);
		}

		/* 2. Prepare the DIMMs for write levelization using DDR3-defined
		 * MR commands. */
		prepare_dimms(p_mct_stat, p_dct_stat, dct, dimm, TRUE);

		/* 3. After the DIMMs are configured, BIOS waits 40 MEMCLKs to
		 *    satisfy DDR3-defined internal DRAM timing.
		 */
		if (is_fam15h())
			precise_memclk_delay_fam15(p_mct_stat, p_dct_stat, dct, 40);
		else
			p_mct_data->agesa_delay(40);

		/* 4. Configure the processor's DDR phy for write levelization training: */
		proc_config(p_mct_stat, p_dct_stat, dct, dimm, pass, nibble);

		/* 5. Begin write levelization training:
		 *  Program F2x[1, 0]9C_x08[WRT_LV_TR_EN]=1. */
		if (pdct_data->logical_cpuid & (AMD_DR_Cx | AMD_DR_Dx | AMD_FAM15_ALL))
		{
			set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
					DRAM_ADD_DCT_PHY_CONTROL_REG, WRT_LV_TR_EN, WRT_LV_TR_EN, 1);
		}
		else
		{
			/* Broadcast write to all D3Dbyte chipset register offset 0xc
			 * Set bit 0 (wrTrain)
			 * Program bit 4 to nibble being trained (only matters for x4dimms)
			 * retain value of 3:2 (Trdimmsel)
			 * reset bit 5 (FrzPR)
			 */
			if (dct)
			{
				addl_data_offset = 0x198;
				addl_data_port = 0x19C;
			}
			else
			{
				addl_data_offset = 0x98;
				addl_data_port = 0x9C;
			}
			addr = 0x0D00000C;
			amd_mem_pci_write_bits(MAKE_SBDFO(0,0,24+(pdct_data->node_id),FUN_DCT,addl_data_offset), 31, 0, &addr);
			while ((get_bits(pdct_data,FUN_DCT,pdct_data->node_id, FUN_DCT, addl_data_offset,
					DCT_ACCESS_DONE, DCT_ACCESS_DONE)) == 0);
			amd_mem_pci_read_bits(MAKE_SBDFO(0,0,24+(pdct_data->node_id),FUN_DCT,addl_data_port), 31, 0, &value);
			value = bit_test_set(value, 0);	/* enable WL training */
			value = bit_test_reset(value, 4); /* for x8 only */
			value = bit_test_reset(value, 5); /* for hardware WL training */
			amd_mem_pci_write_bits(MAKE_SBDFO(0,0,24+(pdct_data->node_id),FUN_DCT,addl_data_port), 31, 0, &value);
			addr = 0x4D030F0C;
			amd_mem_pci_write_bits(MAKE_SBDFO(0,0,24+(pdct_data->node_id),FUN_DCT,addl_data_offset), 31, 0, &addr);
			while ((get_bits(pdct_data,FUN_DCT,pdct_data->node_id, FUN_DCT, addl_data_offset,
					DCT_ACCESS_DONE, DCT_ACCESS_DONE)) == 0);
		}

		if (is_fam15h())
			proc_mfence();

		/* Wait 200 MEMCLKs. If executing pass 2, wait 32 MEMCLKs. */
		if (is_fam15h())
			precise_memclk_delay_fam15(p_mct_stat, p_dct_stat, dct, 200);
		else
			p_mct_data->agesa_delay(140);

		/* Program F2x[1, 0]9C_x08[WrtLevelTrEn]=0. */
		set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_ADD_DCT_PHY_CONTROL_REG, WRT_LV_TR_EN, WRT_LV_TR_EN, 0);

		/* Read from registers F2x[1, 0]9C_x[51:50] and F2x[1, 0]9C_x52
		 * to get the gross and fine delay settings
		 * for the target DIMM and save these values. */
		for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
			getWLByteDelay(p_dct_stat, dct, byte_lane, dimm, pass, nibble, lane_count);
		}

		pdct_data->wl_critical_gross_delay_prev_pass = 0x0;

		/* Exit nibble training if current DIMM is not x4 */
		if ((p_dct_stat->dimm_x4_present & (1 << (dimm + dct))) == 0)
			break;
	}

	return 0;
}

u8 agesa_hw_wl_phase2(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat,
		u8 dct, u8 dimm, u8 pass)
{
	u8 byte_lane;
	u8 status = 0;
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];
	u8 lane_count;

	lane_count = get_available_lane_count(p_mct_stat, p_dct_stat);

	assert(lane_count <= MAX_LANE_COUNT);

	if (is_fam15h()) {
		int32_t gross_diff[MAX_LANE_COUNT];
		int32_t cgd = pdct_data->wl_critical_gross_delay_prev_pass;
		u8 index = (u8)(lane_count * dimm);

		printk(BIOS_SPEW, "\toriginal critical gross delay: %d\n", cgd);

		/* FIXME
		 * For now, disable CGD adjustment as it seems to interfere with registered DIMM training
		 */

		/* Calculate the Critical Gross Delay */
		for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
			/* Calculate the gross delay differential for this lane */
			gross_diff[byte_lane] = pdct_data->wl_seed_gross_delay[index + byte_lane] + pdct_data->wl_gross_delay[index + byte_lane];
			gross_diff[byte_lane] -= pdct_data->wl_seed_pre_gross_delay[index + byte_lane];

			/* wr_dq_dqs_early values greater than 2 are reserved */
			if (gross_diff[byte_lane] < -2)
				gross_diff[byte_lane] = -2;

			/* Update the Critical Gross Delay */
			if (gross_diff[byte_lane] < cgd)
				cgd = gross_diff[byte_lane];
		}

		printk(BIOS_SPEW, "\tnew critical gross delay: %d\n", cgd);

		pdct_data->wl_critical_gross_delay_prev_pass = cgd;

		if (p_dct_stat->speed != p_dct_stat->target_freq) {
			/* FIXME
			 * Using the Pass 1 training values causes major phy training problems on
			 * all Family 15h processors I tested (Pass 1 values are randomly too high,
			 * and Pass 2 cannot lock).
			 * Figure out why this is and fix it, then remove the bypass code below...
			 */
			if (pass == FIRST_PASS) {
				for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
					pdct_data->wl_gross_delay[index + byte_lane] = pdct_data->wl_seed_gross_delay[index + byte_lane];
					pdct_data->wl_fine_delay[index + byte_lane] = pdct_data->wl_seed_fine_delay[index + byte_lane];
				}
				return 0;
			}
		}

		/* Compensate for occasional noise/instability causing sporadic training failure */
		for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
			u8 faulty_value_detected = 0;
			u16 total_delay_seed = ((pdct_data->wl_seed_gross_delay[index + byte_lane] & 0x1f) << 5) | (pdct_data->wl_seed_fine_delay[index + byte_lane] & 0x1f);
			u16 total_delay_phy = ((pdct_data->wl_gross_delay[index + byte_lane] & 0x1f) << 5) | (pdct_data->wl_fine_delay[index + byte_lane] & 0x1f);
			if (pass == FIRST_PASS) {
				/* Allow a somewhat higher step threshold on the first pass
				 * For the most part, as long as the phy isn't stepping
				 * several clocks at once the values are probably valid.
				 */
				if (abs(total_delay_phy - total_delay_seed) > 0x30)
					faulty_value_detected = 1;
			} else {
				/* Stepping memory clocks between adjacent allowed frequencies
				 *  should not yield large phy value differences...
				 */

				if (abs(total_delay_phy - total_delay_seed) > 0x20)
					faulty_value_detected = 1;
			}
			if (faulty_value_detected) {
				printk(BIOS_INFO, "%s: overriding faulty phy value (seed: %04x phy: %04x step: %04x)\n", __func__,
					total_delay_seed, total_delay_phy, abs(total_delay_phy - total_delay_seed));
				pdct_data->wl_gross_delay[index + byte_lane] = pdct_data->wl_seed_gross_delay[index + byte_lane];
				pdct_data->wl_fine_delay[index + byte_lane] = pdct_data->wl_seed_fine_delay[index + byte_lane];
				status = 1;
			}
		}
	}

	return status;
}

u8 agesa_hw_wl_phase3(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat,
		u8 dct, u8 dimm, u8 pass)
{
	u8 byte_lane;
	sMCTStruct *p_mct_data = p_dct_stat->c_mct_ptr;
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];
	u8 lane_count;

	lane_count = get_available_lane_count(p_mct_stat, p_dct_stat);

	assert(lane_count <= MAX_LANE_COUNT);

	if (is_fam15h()) {
		u32 dword;
		int32_t gross_diff[MAX_LANE_COUNT];
		int32_t cgd = pdct_data->wl_critical_gross_delay_prev_pass;
		u8 index = (u8)(lane_count * dimm);

		/* Apply offset(s) if needed */
		if (cgd < 0) {
			dword = Get_NB32_DCT(p_dct_stat->dev_dct, dct, 0xa8);
			dword &= ~(0x3 << 24);			/* wr_dq_dqs_early = abs(cgd) */
			dword |= ((abs(cgd) & 0x3) << 24);
			Set_NB32_DCT(p_dct_stat->dev_dct, dct, 0xa8, dword);

			for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
				/* Calculate the gross delay differential for this lane */
				gross_diff[byte_lane] = pdct_data->wl_seed_gross_delay[index + byte_lane] + pdct_data->wl_gross_delay[index + byte_lane];
				gross_diff[byte_lane] -= pdct_data->wl_seed_pre_gross_delay[index + byte_lane];

				/* Prevent underflow in the presence of noise / instability */
				if (gross_diff[byte_lane] < cgd)
					gross_diff[byte_lane] = cgd;

				pdct_data->wl_gross_delay[index + byte_lane] = (gross_diff[byte_lane] + (abs(cgd) & 0x3));
			}
		} else {
			dword = Get_NB32_DCT(p_dct_stat->dev_dct, dct, 0xa8);
			dword &= ~(0x3 << 24);			/* wr_dq_dqs_early = pdct_data->wr_dqs_gross_dly_base_offset */
			dword |= ((pdct_data->wr_dqs_gross_dly_base_offset & 0x3) << 24);
			Set_NB32_DCT(p_dct_stat->dev_dct, dct, 0xa8, dword);
		}
	}

	/* Write the adjusted gross and fine delay settings
	 * to the target DIMM. */
	for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
		set_wl_byte_delay(p_dct_stat, dct, byte_lane, dimm, 1, pass, lane_count);
	}

	/* 6. Configure DRAM Phy Control Register so that the phy stops driving
	 *    write levelization ODT. */
	set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
			DRAM_ADD_DCT_PHY_CONTROL_REG, WR_LV_ODT_EN, WR_LV_ODT_EN, 0);

	if (is_fam15h())
		proc_mfence();

	/* Wait 10 MEMCLKs to allow for ODT signal settling. */
	if (is_fam15h())
		precise_memclk_delay_fam15(p_mct_stat, p_dct_stat, dct, 10);
	else
		p_mct_data->agesa_delay(10);

	/* 7. Program the target DIMM back to normal operation by configuring
	 * the following (See section 2.8.5.4.1.1
	 * [Phy Assisted Write Levelization] on page 97 pass 1, step #2):
	 * Configure all ranks of the target DIMM for normal operation.
	 * Enable the output drivers of all ranks of the target DIMM.
	 * For a two DIMM system, program the Rtt value for the target DIMM
	 * to the normal operating termination:
	 */
	prepare_dimms(p_mct_stat, p_dct_stat, dct, dimm, FALSE);

	return 0;
}

/*----------------------------------------------------------------------------
 *	LOCAL FUNCTIONS
 *
 *----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
 * u32 swap_addr_bits_wl(struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_value)
 *
 * Description:
 *	This function swaps the bits in MSR register value
 *
 * Parameters:
 *	IN  OUT   *DCTData - Pointer to buffer with information about each DCT
 *	IN	u32: MRS value
 *	OUT	u32: Swapped BANK BITS
 *
 * ----------------------------------------------------------------------------
 */
u32 swap_addr_bits_wl(struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_value)
{
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];
	u32 temp_w, temp_w_1;

	if (is_fam15h())
		temp_w_1 = get_bits(pdct_data, dct, pdct_data->node_id,
				FUN_DCT, DRAM_INIT, MRS_CHIP_SEL_START_FAM_15, MRS_CHIP_SEL_END_FAM_15);
	else
		temp_w_1 = get_bits(pdct_data, dct, pdct_data->node_id,
			FUN_DCT, DRAM_INIT, MRS_CHIP_SEL_START_FAM_10, MRS_CHIP_SEL_END_FAM_10);
	if (temp_w_1 & 1)
	{
		if ((pdct_data->status[DCT_STATUS_OnDimmMirror]))
		{
			/* swap A3/A4,A5/A6,A7/A8 */
			temp_w = mrs_value;
			temp_w_1 = mrs_value;
			temp_w &= 0x0A8;
			temp_w_1 &= 0x0150;
			mrs_value &= 0xFE07;
			mrs_value |= (temp_w << 1);
			mrs_value |= (temp_w_1 >> 1);
		}
	}
	return mrs_value;
}

/*-----------------------------------------------------------------------------
 *  u32 swap_bank_bits(struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_value)
 *
 *  Description:
 *       This function swaps the bits in MSR register value
 *
 *   Parameters:
 *       IN  OUT   *DCTData - Pointer to buffer with information about each DCT
 *       IN	u32: MRS value
 *       OUT       u32: Swapped BANK BITS
 *
 * ----------------------------------------------------------------------------
 */
u32 swap_bank_bits(struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_value)
{
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];
	u32 temp_w, temp_w_1;

	if (is_fam15h())
		temp_w_1 = get_bits(pdct_data, dct, pdct_data->node_id,
				FUN_DCT, DRAM_INIT, MRS_CHIP_SEL_START_FAM_15, MRS_CHIP_SEL_END_FAM_15);
	else
		temp_w_1 = get_bits(pdct_data, dct, pdct_data->node_id,
				FUN_DCT, DRAM_INIT, MRS_CHIP_SEL_START_FAM_10, MRS_CHIP_SEL_END_FAM_10);
	if (temp_w_1 & 1)
	{
		if ((pdct_data->status[DCT_STATUS_OnDimmMirror]))
		{
			/* swap BA0/BA1 */
			temp_w = mrs_value;
			temp_w_1 = mrs_value;
			temp_w &= 0x01;
			temp_w_1 &= 0x02;
			mrs_value = 0;
			mrs_value |= (temp_w << 1);
			mrs_value |= (temp_w_1 >> 1);
		}
	}
	return mrs_value;
}

static u16 unbuffered_dimm_nominal_termination_emrs(u8 number_of_dimms, u8 frequency_index, u8 rank_count, u8 rank)
{
	u16 term;

	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	if (number_of_dimms == 1) {
		if (max_dimms_installable < 3) {
			term = 0x04;	/* Rtt_Nom = RZQ/4 = 60 Ohm */
		} else {
			if (rank_count == 1) {
				term = 0x04;	/* Rtt_Nom = RZQ/4 = 60 Ohm */
			} else {
				if (rank == 0)
					term = 0x04;	/* Rtt_Nom = RZQ/4 = 60 Ohm */
				else
					term = 0x00;	/* Rtt_Nom = OFF */
			}
		}
	} else {
		if (frequency_index < 5)
			term = 0x0044;	/* Rtt_Nom = RZQ/6 = 40 Ohm */
		else
			term = 0x0204;	/* Rtt_Nom = RZQ/8 = 30 Ohm */
	}

	return term;
}

static u16 unbuffered_dimm_dynamic_termination_emrs(u8 number_of_dimms, u8 frequency_index, u8 rank_count)
{
	u16 term;

	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	if (number_of_dimms == 1) {
		if (max_dimms_installable < 3) {
			term = 0x00;	/* Rtt_WR = off */
		} else {
			if (rank_count == 1)
				term = 0x00;	/* Rtt_WR = off */
			else
				term = 0x200;	/* Rtt_WR = RZQ/4 = 60 Ohm */
		}
	} else {
		term = 0x400;	/* Rtt_WR = RZQ/2 = 120 Ohm */
	}

	return term;
}

/*-----------------------------------------------------------------------------
 *  void prepare_dimms(sMCTStruct *p_mct_data, sDCTStruct *DCTData, u8 Dimm, bool WL)
 *
 *  Description:
 *       This function prepares DIMMS for training
 *       Fam10h: BKDG Rev. 3.62 section 2.8.9.9.1
 *       Fam15h: BKDG Rev. 3.14 section 2.10.5.8.1
 * ----------------------------------------------------------------------------
 */
void prepare_dimms(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat,
	u8 dct, u8 dimm, bool wl)
{
	u32 temp_w, temp_w_1, temp_w_2, mrs_bank;
	u8 rank, curr_dimm, mem_clk_freq;
	sMCTStruct *p_mct_data = p_dct_stat->c_mct_ptr;
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];
	u8 package_type = mct_get_nv_bits(NV_PACK_TYPE);
	u8 number_of_dimms = pdct_data->max_dimms_installed;

	if (is_fam15h()) {
		mem_clk_freq = get_bits(pdct_data, dct, pdct_data->node_id,
			FUN_DCT, DRAM_CONFIG_HIGH, 0, 4);
	} else {
		mem_clk_freq = get_bits(pdct_data, dct, pdct_data->node_id,
			FUN_DCT, DRAM_CONFIG_HIGH, 0, 2);
	}
	/* Configure the DCT to send initialization MR commands to the target DIMM
	 * by programming the F2x[1,0]7C register using the following steps.
	 */
	rank = 0;
	while ((rank < pdct_data->dimm_ranks[dimm]) && (rank < 2))
	{
		/* Program F2x[1, 0]7C[MrsChipSel[2:0]] for the current rank to be trained. */
		if (is_fam15h())
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_CHIP_SEL_START_FAM_15, MRS_CHIP_SEL_END_FAM_15, dimm * 2 + rank);
		else
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_CHIP_SEL_START_FAM_10, MRS_CHIP_SEL_END_FAM_10, dimm * 2 + rank);

		/* Program F2x[1, 0]7C[mrs_bank[2:0]] for the appropriate internal DRAM
		 * register that defines the required DDR3-defined function for write
		 * levelization.
		 */
		mrs_bank = swap_bank_bits(p_dct_stat, dct, 1);
		if (is_fam15h())
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_BANK_START_FAM_15, MRS_BANK_END_FAM_15, mrs_bank);
		else
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_BANK_START_FAM_10, MRS_BANK_END_FAM_10, mrs_bank);

		/* Program F2x[1, 0]7C[MrsAddress[15:0]] to the required DDR3-defined function
		 * for write levelization.
		 */
		temp_w = 0;/* DLL_DIS = 0, DIC = 0, AL = 0, TDQS = 0 */

		/* Retrieve normal settings of the MRS control word and clear Rtt_Nom */
		if (is_fam15h()) {
			temp_w = mct_mr1(p_mct_stat, p_dct_stat, dct, dimm * 2 + rank) & 0xffff;
			temp_w &= ~(0x0244);
		} else {
			/* Set TDQS = 1b for x8 DIMM, TDQS = 0b for x4 DIMM, when mixed x8 & x4 */
			temp_w_2 = get_bits(pdct_data, dct, pdct_data->node_id,
					FUN_DCT, DRAM_CONFIG_HIGH, RDQS_EN, RDQS_EN);
			if (temp_w_2)
			{
				if (pdct_data->dimm_x8_present[dimm])
					temp_w |= 0x800;
			}
		}

		/* determine Rtt_Nom for WL & Normal mode */
		if (is_fam15h()) {
			if (wl) {
				if (number_of_dimms > 1) {
					if (rank == 0) {
						/* Get Rtt_WR for the current DIMM and rank */
						temp_w_2 = fam15_rttwr(p_dct_stat, dct, dimm, rank, package_type);
					} else {
						temp_w_2 = fam15_rttnom(p_dct_stat, dct, dimm, rank, package_type);
					}
				} else {
					temp_w_2 = fam15_rttnom(p_dct_stat, dct, dimm, rank, package_type);
				}
			} else {
				temp_w_2 = fam15_rttnom(p_dct_stat, dct, dimm, rank, package_type);
			}
			temp_w_1 = 0;
			temp_w_1 |= ((temp_w_2 & 0x4) >> 2) << 9;
			temp_w_1 |= ((temp_w_2 & 0x2) >> 1) << 6;
			temp_w_1 |= ((temp_w_2 & 0x1) >> 0) << 2;
		} else {
			if (pdct_data->status[DCT_STATUS_REGISTERED]) {
				temp_w_1 = rtt_nom_target_reg_dimm(p_mct_data, pdct_data, dimm, wl, mem_clk_freq, rank);
			} else {
				if (wl) {
					if (number_of_dimms > 1) {
						if (rank == 0) {
							/* Get Rtt_WR for the current DIMM and rank */
							u16 dynamic_term = unbuffered_dimm_dynamic_termination_emrs(pdct_data->max_dimms_installed, mem_clk_freq, pdct_data->dimm_ranks[dimm]);

							/* Convert dynamic termination code to corresponding nominal termination code */
							if (dynamic_term == 0x200)
								temp_w_1 = 0x04;
							else if (dynamic_term == 0x400)
								temp_w_1 = 0x40;
							else
								temp_w_1 = 0x0;
						} else {
							temp_w_1 = unbuffered_dimm_nominal_termination_emrs(pdct_data->max_dimms_installed, mem_clk_freq, pdct_data->dimm_ranks[dimm], rank);
						}
					} else {
						temp_w_1 = unbuffered_dimm_nominal_termination_emrs(pdct_data->max_dimms_installed, mem_clk_freq, pdct_data->dimm_ranks[dimm], rank);
					}
				} else {
					temp_w_1 = unbuffered_dimm_nominal_termination_emrs(pdct_data->max_dimms_installed, mem_clk_freq, pdct_data->dimm_ranks[dimm], rank);
				}
			}
		}

		/* Apply Rtt_Nom to the MRS control word */
		temp_w = temp_w | temp_w_1;

		/* All ranks of the target DIMM are set to write levelization mode. */
		if (wl)
		{
			temp_w_1 = bit_test_set(temp_w, MRS_LEVEL);
			if (rank == 0)
			{
				/* Enable the output driver of the first rank of the target DIMM. */
				temp_w = temp_w_1;
			}
			else
			{
				/* Disable the output drivers of all other ranks for
				 * the target DIMM.
				 */
				temp_w = bit_test_set(temp_w_1, QOFF);
			}
		}

		/* Program MrsAddress[5,1]=output driver impedance control (DIC) */
		if (is_fam15h()) {
			temp_w_1 = fam15_dimm_dic(p_dct_stat, dct, dimm, rank, package_type);
		} else {
			/* Read DIC from F2x[1,0]84[DrvImpCtrl] */
			temp_w_1 = get_bits(pdct_data, dct, pdct_data->node_id,
					FUN_DCT, DRAM_MRS_REGISTER, DRV_IMP_CTRL_START, DRV_IMP_CTRL_END);
		}

		/* Apply DIC to the MRS control word */
		if (bit_test(temp_w_1, 1))
			temp_w = bit_test_set(temp_w, 5);
		if (bit_test(temp_w_1, 0))
			temp_w = bit_test_set(temp_w, 1);

		temp_w = swap_addr_bits_wl(p_dct_stat, dct, temp_w);

		if (is_fam15h())
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_ADDRESS_START_FAM_15, MRS_ADDRESS_END_FAM_15, temp_w);
		else
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_ADDRESS_START_FAM_10, MRS_ADDRESS_END_FAM_10, temp_w);

		/* Program F2x[1, 0]7C[SEND_MRS_CMD]=1 to initiate the command to
		 * the specified DIMM.
		 */
		set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
			DRAM_INIT, SEND_MRS_CMD, SEND_MRS_CMD, 1);
		/* Wait for F2x[1, 0]7C[SEND_MRS_CMD] to be cleared by hardware. */
		while ((get_bits(pdct_data, dct, pdct_data->node_id,
				FUN_DCT, DRAM_INIT, SEND_MRS_CMD, SEND_MRS_CMD)) == 0x1)
		{
		}

		/* Program F2x[1, 0]7C[mrs_bank[2:0]] for the appropriate internal DRAM
		 * register that defines the required DDR3-defined function for Rtt_WR.
		 */
		mrs_bank = swap_bank_bits(p_dct_stat, dct, 2);
		if (is_fam15h())
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_BANK_START_FAM_15, MRS_BANK_END_FAM_15, mrs_bank);
		else
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_BANK_START_FAM_10, MRS_BANK_END_FAM_10, mrs_bank);

		/* Program F2x[1, 0]7C[MrsAddress[15:0]] to the required DDR3-defined function
		 * for Rtt_WR (DRAMTermDyn).
		 */
		temp_w = 0;/* PASR = 0,*/

		/* Retrieve normal settings of the MRS control word and clear Rtt_WR */
		if (is_fam15h()) {
			temp_w = mct_mr2(p_mct_stat, p_dct_stat, dct, dimm * 2 + rank) & 0xffff;
			temp_w &= ~(0x0600);
		} else {
			/* program MrsAddress[7,6,5:3]=SRT,ASR,CWL,
			* based on F2x[1,0]84[19,18,22:20]=,SRT,ASR,Tcwl */
			temp_w_1 = get_bits(pdct_data, dct, pdct_data->node_id,
					FUN_DCT, DRAM_MRS_REGISTER, PCI_MIN_LOW, PCI_MAX_HIGH);
			if (bit_test(temp_w_1,19))
			{temp_w = bit_test_set(temp_w, 7);}
			if (bit_test(temp_w_1,18))
			{temp_w = bit_test_set(temp_w, 6);}
			/* temp_w = temp_w|(((temp_w_1 >> 20) & 0x7)<< 3); */
			temp_w = temp_w | ((temp_w_1 & 0x00700000) >> 17);
			/* workaround for DR-B0 */
			if ((pdct_data->logical_cpuid & AMD_DR_Bx) && (pdct_data->status[DCT_STATUS_REGISTERED]))
				temp_w += 0x8;
		}

		/* determine Rtt_WR for WL & Normal mode */
		if (is_fam15h()) {
			temp_w_1 = (fam15_rttwr(p_dct_stat, dct, dimm, rank, package_type) << 9);
		} else {
			if (pdct_data->status[DCT_STATUS_REGISTERED])
				temp_w_1 = rtt_wr_reg_dimm(p_mct_data, pdct_data, dimm, wl, mem_clk_freq, rank);
			else
				temp_w_1 = unbuffered_dimm_dynamic_termination_emrs(pdct_data->max_dimms_installed, mem_clk_freq, pdct_data->dimm_ranks[dimm]);
		}

		/* Apply Rtt_WR to the MRS control word */
		temp_w = temp_w | temp_w_1;
		temp_w = swap_addr_bits_wl(p_dct_stat, dct, temp_w);
		if (is_fam15h())
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_ADDRESS_START_FAM_15, MRS_ADDRESS_END_FAM_15, temp_w);
		else
			set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_INIT, MRS_ADDRESS_START_FAM_10, MRS_ADDRESS_END_FAM_10, temp_w);

		/* Program F2x[1, 0]7C[SEND_MRS_CMD]=1 to initiate the command to
		   the specified DIMM.*/
		set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
			DRAM_INIT, SEND_MRS_CMD, SEND_MRS_CMD, 1);

		/* Wait for F2x[1, 0]7C[SEND_MRS_CMD] to be cleared by hardware. */
		while ((get_bits(pdct_data, dct, pdct_data->node_id,
				FUN_DCT, DRAM_INIT, SEND_MRS_CMD, SEND_MRS_CMD)) == 0x1)
		{
		}

		rank++;
	}

	/* Configure the non-target DIMM normally. */
	curr_dimm = 0;
	while (curr_dimm < MAX_LDIMMS)
	{
		if (pdct_data->dimm_present[curr_dimm])
		{
			if (curr_dimm != dimm)
			{
				rank = 0;
				while ((rank < pdct_data->dimm_ranks[curr_dimm]) && (rank < 2))
				{
					/* Program F2x[1, 0]7C[MrsChipSel[2:0]] for the current rank
					 * to be trained.
					 */
					if (is_fam15h())
						set_bits(pdct_data, dct, pdct_data->node_id,
							FUN_DCT, DRAM_INIT, MRS_CHIP_SEL_START_FAM_15, MRS_CHIP_SEL_END_FAM_15, curr_dimm * 2 + rank);
					else
						set_bits(pdct_data, dct, pdct_data->node_id,
							FUN_DCT, DRAM_INIT, MRS_CHIP_SEL_START_FAM_10, MRS_CHIP_SEL_END_FAM_10, curr_dimm * 2 + rank);

					/* Program F2x[1, 0]7C[mrs_bank[2:0]] for the appropriate internal
					 * DRAM register that defines the required DDR3-defined function
					 * for write levelization.
					 */
					mrs_bank = swap_bank_bits(p_dct_stat, dct, 1);
					if (is_fam15h())
						set_bits(pdct_data, dct, pdct_data->node_id,
							FUN_DCT, DRAM_INIT, MRS_BANK_START_FAM_15, MRS_BANK_END_FAM_15, mrs_bank);
					else
						set_bits(pdct_data, dct, pdct_data->node_id,
							FUN_DCT, DRAM_INIT, MRS_BANK_START_FAM_10, MRS_BANK_END_FAM_10, mrs_bank);

					/* Program F2x[1, 0]7C[MrsAddress[15:0]] to the required
					 * DDR3-defined function for write levelization.
					 */
					temp_w = 0;/* DLL_DIS = 0, DIC = 0, AL = 0, TDQS = 0, Level = 0, QOFF = 0 */

					/* Retrieve normal settings of the MRS control word and clear Rtt_Nom */
					if (is_fam15h()) {
						temp_w = mct_mr1(p_mct_stat, p_dct_stat, dct, curr_dimm * 2 + rank) & 0xffff;
						temp_w &= ~(0x0244);
					} else {
						/* Set TDQS = 1b for x8 DIMM, TDQS = 0b for x4 DIMM, when mixed x8 & x4 */
						temp_w_2 = get_bits(pdct_data, dct, pdct_data->node_id,
								FUN_DCT, DRAM_CONFIG_HIGH, RDQS_EN, RDQS_EN);
						if (temp_w_2)
						{
							if (pdct_data->dimm_x8_present[curr_dimm])
								temp_w |= 0x800;
						}
					}

					/* determine Rtt_Nom for WL & Normal mode */
					if (is_fam15h()) {
						temp_w_2 = fam15_rttnom(p_dct_stat, dct, dimm, rank, package_type);
						temp_w_1 = 0;
						temp_w_1 |= ((temp_w_2 & 0x4) >> 2) << 9;
						temp_w_1 |= ((temp_w_2 & 0x2) >> 1) << 6;
						temp_w_1 |= ((temp_w_2 & 0x1) >> 0) << 2;
					} else {
						if (pdct_data->status[DCT_STATUS_REGISTERED])
							temp_w_1 = rtt_nom_non_target_reg_dimm(p_mct_data, pdct_data, curr_dimm, wl, mem_clk_freq, rank);
						else
							temp_w_1 = unbuffered_dimm_nominal_termination_emrs(pdct_data->max_dimms_installed, mem_clk_freq, pdct_data->dimm_ranks[curr_dimm], rank);
					}

					/* Apply Rtt_Nom to the MRS control word */
					temp_w = temp_w | temp_w_1;

					/* Program MrsAddress[5,1]=output driver impedance control (DIC) */
					if (is_fam15h()) {
						temp_w_1 = fam15_dimm_dic(p_dct_stat, dct, dimm, rank, package_type);
					} else {
						/* Read DIC from F2x[1,0]84[DrvImpCtrl] */
						temp_w_1 = get_bits(pdct_data, dct, pdct_data->node_id,
								FUN_DCT, DRAM_MRS_REGISTER, DRV_IMP_CTRL_START, DRV_IMP_CTRL_END);
					}

					/* Apply DIC to the MRS control word */
					if (bit_test(temp_w_1,1))
					{temp_w = bit_test_set(temp_w, 5);}
					if (bit_test(temp_w_1,0))
					{temp_w = bit_test_set(temp_w, 1);}

					temp_w = swap_addr_bits_wl(p_dct_stat, dct, temp_w);

					if (is_fam15h())
						set_bits(pdct_data, dct, pdct_data->node_id,
							FUN_DCT, DRAM_INIT, MRS_ADDRESS_START_FAM_15, MRS_ADDRESS_END_FAM_15, temp_w);
					else
						set_bits(pdct_data, dct, pdct_data->node_id,
							FUN_DCT, DRAM_INIT, MRS_ADDRESS_START_FAM_10, MRS_ADDRESS_END_FAM_10, temp_w);

					/* Program F2x[1, 0]7C[SEND_MRS_CMD]=1 to initiate the command
					 * to the specified DIMM.
					 */
					set_bits(pdct_data, dct, pdct_data->node_id,
						FUN_DCT, DRAM_INIT, SEND_MRS_CMD, SEND_MRS_CMD, 1);

					/* Wait for F2x[1, 0]7C[SEND_MRS_CMD] to be cleared by hardware. */
					while ((get_bits(pdct_data, dct,
							pdct_data->node_id, FUN_DCT, DRAM_INIT,
							SEND_MRS_CMD, SEND_MRS_CMD)) == 1);

					/* Program F2x[1, 0]7C[mrs_bank[2:0]] for the appropriate internal DRAM
					 * register that defines the required DDR3-defined function for Rtt_WR.
					 */
					mrs_bank = swap_bank_bits(p_dct_stat, dct, 2);
					if (is_fam15h())
						set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
							DRAM_INIT, MRS_BANK_START_FAM_15, MRS_BANK_END_FAM_15, mrs_bank);
					else
						set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
							DRAM_INIT, MRS_BANK_START_FAM_10, MRS_BANK_END_FAM_10, mrs_bank);

					/* Program F2x[1, 0]7C[MrsAddress[15:0]] to the required DDR3-defined function
					 * for Rtt_WR (DRAMTermDyn).
					 */
					temp_w = 0;/* PASR = 0,*/

					/* Retrieve normal settings of the MRS control word and clear Rtt_WR */
					if (is_fam15h()) {
						temp_w = mct_mr2(p_mct_stat, p_dct_stat, dct, curr_dimm * 2 + rank) & 0xffff;
						temp_w &= ~(0x0600);
					} else {
						/* program MrsAddress[7,6,5:3]=SRT,ASR,CWL,
						* based on F2x[1,0]84[19,18,22:20]=,SRT,ASR,Tcwl */
						temp_w_1 = get_bits(pdct_data, dct, pdct_data->node_id,
								FUN_DCT, DRAM_MRS_REGISTER, PCI_MIN_LOW, PCI_MAX_HIGH);
						if (bit_test(temp_w_1,19))
						{temp_w = bit_test_set(temp_w, 7);}
						if (bit_test(temp_w_1,18))
						{temp_w = bit_test_set(temp_w, 6);}
						/* temp_w = temp_w|(((temp_w_1 >> 20) & 0x7) << 3); */
						temp_w = temp_w | ((temp_w_1 & 0x00700000) >> 17);
						/* workaround for DR-B0 */
						if ((pdct_data->logical_cpuid & AMD_DR_Bx) && (pdct_data->status[DCT_STATUS_REGISTERED]))
							temp_w+=0x8;
					}

					/* determine Rtt_WR for WL & Normal mode */
					if (is_fam15h()) {
						temp_w_1 = (fam15_rttwr(p_dct_stat, dct, dimm, rank, package_type) << 9);
					} else {
						if (pdct_data->status[DCT_STATUS_REGISTERED])
							temp_w_1 = rtt_wr_reg_dimm(p_mct_data, pdct_data, curr_dimm, wl, mem_clk_freq, rank);
						else
							temp_w_1 = unbuffered_dimm_dynamic_termination_emrs(pdct_data->max_dimms_installed, mem_clk_freq, pdct_data->dimm_ranks[curr_dimm]);
					}

					/* Apply Rtt_WR to the MRS control word */
					temp_w = temp_w | temp_w_1;
					temp_w = swap_addr_bits_wl(p_dct_stat, dct, temp_w);
					if (is_fam15h())
						set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
							DRAM_INIT, MRS_ADDRESS_START_FAM_15, MRS_ADDRESS_END_FAM_15, temp_w);
					else
						set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
							DRAM_INIT, MRS_ADDRESS_START_FAM_10, MRS_ADDRESS_END_FAM_10, temp_w);

					/* Program F2x[1, 0]7C[SEND_MRS_CMD]=1 to initiate the command to
					   the specified DIMM.*/
					set_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
						DRAM_INIT, SEND_MRS_CMD, SEND_MRS_CMD, 1);

					/* Wait for F2x[1, 0]7C[SEND_MRS_CMD] to be cleared by hardware. */
					while ((get_bits(pdct_data, dct, pdct_data->node_id,
							FUN_DCT, DRAM_INIT, SEND_MRS_CMD, SEND_MRS_CMD)) == 0x1)
					{
					}
					rank++;
				}
			}
		}
		curr_dimm++;
	}
}

/*-----------------------------------------------------------------------------
 * void program_odt(sMCTStruct *p_mct_data, DCTStruct *DCTData, u8 dimm)
 *
 *  Description:
 *       This function programs the ODT values for the NB
 *
 *   Parameters:
 *       IN  OUT   *DCTData - Pointer to buffer with information about each DCT
 *       IN
 *       OUT
 * ----------------------------------------------------------------------------
 */
void program_odt(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 dct, u8 dimm)
{
	sMCTStruct *p_mct_data = p_dct_stat->c_mct_ptr;
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];

	u8 wr_lv_odt_1 = 0;

	if (is_fam15h()) {
		/* On Family15h processors, the value for the specific CS being targeted
		 * is taken from F2x238 / F2x23C as appropriate, then loaded into F2x9C_x0000_0008
		 */

		/* Convert DIMM number to CS */
		u32 dword;
		u8 cs;
		u8 rank = 0;

		cs = (dimm * 2) + rank;

		/* Fetch preprogammed ODT pattern from configuration registers */
		dword = Get_NB32_DCT(p_dct_stat->dev_dct, dct, ((cs > 3) ? 0x23c : 0x238));
		if ((cs == 7) || (cs == 3))
			wr_lv_odt_1 = ((dword >> 24) & 0xf);
		else if ((cs == 6) || (cs == 2))
			wr_lv_odt_1 = ((dword >> 16) & 0xf);
		else if ((cs == 5) || (cs == 1))
			wr_lv_odt_1 = ((dword >> 8) & 0xf);
		else if ((cs == 4) || (cs == 0))
			wr_lv_odt_1 = (dword & 0xf);
	} else {
		if (pdct_data->status[DCT_STATUS_REGISTERED]) {
			wr_lv_odt_1 = wr_lv_odt_reg_dimm(p_mct_data, pdct_data, dimm);
		} else {
			if ((pdct_data->dct_cs_present & 0x05) == 0x05) {
				wr_lv_odt_1 = 0x03;
			} else if (bit_test((u32)pdct_data->dct_cs_present,(u8)(dimm * 2 + 1))) {
				wr_lv_odt_1 = (u8)bit_test_set(wr_lv_odt_1, dimm + 2);
			} else {
				wr_lv_odt_1 = (u8)bit_test_set(wr_lv_odt_1, dimm);
			}
		}
	}

	set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
			DRAM_ADD_DCT_PHY_CONTROL_REG, 8, 11, (u32)wr_lv_odt_1);

	printk(BIOS_SPEW, "Programmed DCT %d write levelling ODT pattern %08x from DIMM %d data\n", dct, wr_lv_odt_1, dimm);

}

#ifdef UNUSED_CODE
static u16 fam15h_next_lowest_memclk_freq(u16 memclk_freq)
{
	u16 fam15h_next_lowest_freq_tab[] = {0, 0, 0, 0, 0x4, 0, 0x4, 0, 0, 0, 0x6, 0, 0, 0, 0xa, 0, 0, 0, 0xe, 0, 0, 0, 0x12};
	return fam15h_next_lowest_freq_tab[memclk_freq];
}
#endif

/*-----------------------------------------------------------------------------
 * void proc_config(MCTStruct *MCTData,DCTStruct *DCTData, u8 Dimm, u8 Pass, u8 Nibble)
 *
 *  Description:
 *       This function programs the ODT values for the NB
 *
 *   Parameters:
 *       IN  OUT   *DCTData - Pointer to buffer with information about each DCT
 *		 *MCTData - Pointer to buffer with runtime parameters,
 *       IN	Dimm - Logical DIMM
 *		 Pass - First of Second Pass
 *       OUT
 * ----------------------------------------------------------------------------
 */
void proc_config(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 dct, u8 dimm, u8 pass, u8 nibble)
{
	u8 byte_lane, mem_clk_freq;
	int32_t seed_gross;
	int32_t seed_fine;
	u8 seed_pre_gross;
	u32 value, addr;
	u32 dword;
	u16 addl_data_offset, addl_data_port;
	sMCTStruct *p_mct_data = p_dct_stat->c_mct_ptr;
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];
	u16 fam10h_freq_tab[] = {0, 0, 0, 400, 533, 667, 800};
	u16 fam15h_freq_tab[] = {0, 0, 0, 0, 333, 0, 400, 0, 0, 0, 533, 0, 0, 0, 667, 0, 0, 0, 800, 0, 0, 0, 933};
	u8 lane_count;

	lane_count = get_available_lane_count(p_mct_stat, p_dct_stat);

	assert(lane_count <= MAX_LANE_COUNT);

	if (is_fam15h()) {
		/* mem_clk_freq: 0x4: 333MHz; 0x6: 400MHz; 0xa: 533MHz; 0xe: 667MHz; 0x12: 800MHz; 0x16: 933MHz */
		mem_clk_freq = get_bits(pdct_data, dct, pdct_data->node_id,
					FUN_DCT, DRAM_CONFIG_HIGH, 0, 4);
	} else {
		/* mem_clk_freq: 3: 400MHz; 4: 533MHz; 5: 667MHz; 6: 800MHz */
		mem_clk_freq = get_bits(pdct_data, dct, pdct_data->node_id,
					FUN_DCT, DRAM_CONFIG_HIGH, 0, 2);
	}

	/* Program F2x[1, 0]9C_x08[WrLvOdt[3:0]] to the proper ODT settings for the
	 * current memory subsystem configuration.
	 */
	program_odt(p_mct_stat, p_dct_stat, dct, dimm);

	/* Program F2x[1,0]9C_x08[WR_LV_ODT_EN]=1 */
	if (pdct_data->logical_cpuid & (AMD_DR_Cx | AMD_DR_Dx | AMD_FAM15_ALL)) {
		set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_ADD_DCT_PHY_CONTROL_REG, WR_LV_ODT_EN, WR_LV_ODT_EN, (u32)1);
	}
	else
	{
		/* Program WR_LV_ODT_EN = 1 through set bit 12 of D3CSODT reg offset 0 for Rev.B */
		if (dct)
		{
			addl_data_offset = 0x198;
			addl_data_port = 0x19C;
		}
		else
		{
			addl_data_offset = 0x98;
			addl_data_port = 0x9C;
		}
		addr = 0x0D008000;
		amd_mem_pci_write_bits(MAKE_SBDFO(0,0,24+(pdct_data->node_id),FUN_DCT,addl_data_offset), 31, 0, &addr);
		while ((get_bits(pdct_data,FUN_DCT,pdct_data->node_id, FUN_DCT, addl_data_offset,
				DCT_ACCESS_DONE, DCT_ACCESS_DONE)) == 0);
		amd_mem_pci_read_bits(MAKE_SBDFO(0,0,24+(pdct_data->node_id),FUN_DCT,addl_data_port), 31, 0, &value);
		value = bit_test_set(value, 12);
		amd_mem_pci_write_bits(MAKE_SBDFO(0,0,24+(pdct_data->node_id),FUN_DCT,addl_data_port), 31, 0, &value);
		addr = 0x4D088F00;
		amd_mem_pci_write_bits(MAKE_SBDFO(0,0,24+(pdct_data->node_id),FUN_DCT,addl_data_offset), 31, 0, &addr);
		while ((get_bits(pdct_data,FUN_DCT,pdct_data->node_id, FUN_DCT, addl_data_offset,
				DCT_ACCESS_DONE, DCT_ACCESS_DONE)) == 0);
	}

	if (is_fam15h())
		proc_mfence();

	/* Wait 10 MEMCLKs to allow for ODT signal settling. */
	if (is_fam15h())
		precise_memclk_delay_fam15(p_mct_stat, p_dct_stat, dct, 10);
	else
		p_mct_data->agesa_delay(10);

	/* Program write levelling seed values */
	if (pass == 1)
	{
		/* Pass 1 */
		if (is_fam15h()) {
			u8 addr_cmd_prelaunch = 0;		/* TODO: Fetch the correct value from RC2[0] */
			u8 package_type = mct_get_nv_bits(NV_PACK_TYPE);
			u16 Seed_Total = 0;
			pdct_data->wr_dqs_gross_dly_base_offset = 0x0;
			if (package_type == PT_GR) {
				/* Socket G34: Fam15h BKDG v3.14 Table 96 */
				if (pdct_data->status[DCT_STATUS_REGISTERED]) {
					/* TODO
					 * Implement mainboard-specific seed and
					 * WrDqsGrossDly base overrides.
					 * 0x41 and 0x0 are the "stock" values
					 */
					Seed_Total = 0x41;
					pdct_data->wr_dqs_gross_dly_base_offset = 0x2;
				} else if (pdct_data->status[DCT_STATUS_LOAD_REDUCED]) {
					Seed_Total = 0x0;
				} else {
					Seed_Total = 0xf;
				}
			} else if (package_type == PT_C3) {
				/* Socket C32: Fam15h BKDG v3.14 Table 97 */
				if (pdct_data->status[DCT_STATUS_REGISTERED]) {
					Seed_Total = 0x3e;
				} else if (pdct_data->status[DCT_STATUS_LOAD_REDUCED]) {
					Seed_Total = 0x0;
				} else {
					Seed_Total = 0x12;
				}
			} else if (package_type == PT_M2) {
				/* Socket AM3: Fam15h BKDG v3.14 Table 98 */
				Seed_Total = 0xf;
			} else if (package_type == PT_FM2) {
				/* Socket FM2: Fam15h M10 BKDG 3.12 Table 42 */
				Seed_Total = 0x15;
			}
			if (pdct_data->status[DCT_STATUS_REGISTERED])
				Seed_Total += ((addr_cmd_prelaunch) ? 0x10 : 0x0);

			/* Adjust seed for the minimum platform supported frequency */
			Seed_Total = (int32_t) (((((int64_t) Seed_Total) *
				fam15h_freq_tab[mem_clk_freq] * 100) / (mct_get_nv_bits(NV_MIN_MEMCLK) * 100)));

			seed_gross = (Seed_Total >> 5) & 0x1f;
			seed_fine = Seed_Total & 0x1f;

			/* Save seed values for later use */
			for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
				pdct_data->wl_seed_gross_delay[lane_count * dimm + byte_lane] = seed_gross;
				pdct_data->wl_seed_fine_delay[lane_count * dimm + byte_lane] = seed_fine;

				if (seed_gross == 0)
					seed_pre_gross = 0;
				else if (seed_gross & 0x1)
					seed_pre_gross = 1;
				else
					seed_pre_gross = 2;

				pdct_data->wl_seed_pre_gross_delay[lane_count * dimm + byte_lane] = seed_pre_gross;
			}
		} else {
			if (pdct_data->status[DCT_STATUS_REGISTERED]) {
				u8 addr_cmd_prelaunch = 0;		/* TODO: Fetch the correct value from RC2[0] */

				/* The seed values below assume Pass 1 utilizes a 400MHz clock frequency (DDR3-800) */
				if (addr_cmd_prelaunch == 0) {
					seed_gross = 0x02;
					seed_fine = 0x01;
				} else {
					seed_gross = 0x02;
					seed_fine = 0x11;
				}
			} else {
				if (mem_clk_freq == 6) {
					/* DDR-800 */
					seed_gross = 0x00;
					seed_fine = 0x1a;
				} else {
					/* Use settings for DDR-400 (interpolated from BKDG) */
					seed_gross = 0x00;
					seed_fine = 0x0d;
				}
			}
		}
		for (byte_lane = 0; byte_lane < lane_count; byte_lane++)
		{
			/* Program an initialization value to registers F2x[1, 0]9C_x[51:50] and
			 * F2x[1, 0]9C_x52 to set the gross and fine delay for all the byte lane fields
			 * If the target frequency is different than 400MHz, BIOS must
			 * execute two training passes for each DIMM.
			 * For pass 1 at a 400MHz MEMCLK frequency, use an initial total delay value
			 * of 01Fh. This represents a 1UI (UI=.5MEMCLK) delay and is determined
			 * by design.
			 */
			pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] = seed_gross;
			pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] = seed_fine;
			printk(BIOS_SPEW, "\tLane %02x initial seed: %04x\n", byte_lane, ((seed_gross & 0x1f) << 5) | (seed_fine & 0x1f));
		}
	} else {
		if (nibble == 0) {
			/* Pass 2 */
			/* From BKDG, Write Leveling Seed value. */
			if (is_fam15h()) {
				u32 register_delay;
				int32_t seed_total[MAX_LANE_COUNT];
				int32_t seed_total_pre_scaling[MAX_LANE_COUNT];
				u32 wr_dq_dqs_early;
				u8 addr_cmd_prelaunch = 0;		/* TODO: Fetch the correct value from RC2[0] */

				if (pdct_data->status[DCT_STATUS_REGISTERED]) {
					if (addr_cmd_prelaunch)
						register_delay = 0x30;
					else
						register_delay = 0x20;
				} else {
					register_delay = 0;
				}

				/* Retrieve wr_dq_dqs_early */
				dword = Get_NB32_DCT(p_dct_stat->dev_dct, dct, 0xa8);
				wr_dq_dqs_early = (dword >> 24) & 0x3;

				/* FIXME
				 * Ignore wr_dq_dqs_early for now to work around training issues
				 */
				wr_dq_dqs_early = 0;

				/* Generate new seed values */
				for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
					/* Calculate adjusted seed values */
					seed_total[byte_lane] = (pdct_data->wl_fine_delay_prev_pass[lane_count * dimm + byte_lane] & 0x1f) |
						((pdct_data->wl_gross_delay_prev_pass[lane_count * dimm + byte_lane] & 0x1f) << 5);
					seed_total_pre_scaling[byte_lane] = (seed_total[byte_lane] - register_delay - (0x20 * wr_dq_dqs_early));
					seed_total[byte_lane] = (int32_t) (register_delay + ((((int64_t) seed_total_pre_scaling[byte_lane]) *
						fam15h_freq_tab[mem_clk_freq] * 100) / (fam15h_freq_tab[pdct_data->wl_prev_memclk_freq[dimm]] * 100)));
				}

				/* Generate register values from seeds */
				for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
					printk(BIOS_SPEW, "\tLane %02x scaled delay: %04x\n", byte_lane, seed_total[byte_lane]);

					if (seed_total[byte_lane] >= 0) {
						seed_gross = seed_total[byte_lane] / 32;
						seed_fine = seed_total[byte_lane] % 32;
					} else {
						seed_gross = (seed_total[byte_lane] / 32) - 1;
						seed_fine = (seed_total[byte_lane] % 32) + 32;
					}

					/* The BKDG-recommended algorithm causes problems with registered DIMMs on some systems
					 * due to the long register delays causing premature total delay wrap-around.
					 * Attempt to work around this...
					 */
					seed_pre_gross = seed_gross;

					/* Save seed values for later use */
					pdct_data->wl_seed_gross_delay[lane_count * dimm + byte_lane] = seed_gross;
					pdct_data->wl_seed_fine_delay[lane_count * dimm + byte_lane] = seed_fine;
					pdct_data->wl_seed_pre_gross_delay[lane_count * dimm + byte_lane] = seed_pre_gross;

					pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] = seed_pre_gross;
					pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] = seed_fine;

					printk(BIOS_SPEW, "\tLane %02x new seed: %04x\n", byte_lane, ((pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] & 0x1f) << 5) | (pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] & 0x1f));
				}
			} else {
				u32 register_delay;
				u32 seed_total_pre_scaling;
				u32 seed_total;
				u8 addr_cmd_prelaunch = 0;		/* TODO: Fetch the correct value from RC2[0] */
				for (byte_lane = 0; byte_lane < lane_count; byte_lane++)
				{
					if (pdct_data->status[DCT_STATUS_REGISTERED]) {
						if (addr_cmd_prelaunch == 0)
							register_delay = 0x20;
						else
							register_delay = 0x30;
					} else {
						register_delay = 0;
					}
					seed_total_pre_scaling = ((pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] & 0x1f) |
						(pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] << 5)) - register_delay;
					/* seed_total_pre_scaling = (the total delay value in F2x[1, 0]9C_x[4A:30] from pass 1 of write levelization
					training) - register_delay. */
					seed_total = (u16) ((((u64) seed_total_pre_scaling) *
										fam10h_freq_tab[mem_clk_freq] * 100) / (fam10h_freq_tab[3] * 100));
					seed_gross = seed_total / 32;
					seed_fine = seed_total & 0x1f;
					if (seed_gross == 0)
						seed_gross = 0;
					else if (seed_gross & 0x1)
						seed_gross = 1;
					else
						seed_gross = 2;

					/* The BKDG-recommended algorithm causes problems with registered DIMMs on some systems
					* due to the long register delays causing premature total delay wrap-around.
					* Attempt to work around this...
					*/
					seed_total = ((seed_gross & 0x1f) << 5) | (seed_fine & 0x1f);
					seed_total += register_delay;
					seed_gross = seed_total / 32;
					seed_fine = seed_total & 0x1f;

					pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] = seed_gross;
					pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] = seed_fine;

					printk(BIOS_SPEW, "\tLane %02x new seed: %04x\n", byte_lane, ((pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] & 0x1f) << 5) | (pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] & 0x1f));
				}
			}

			/* Save initial seeds for upper nibble pass */
			for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
				pdct_data->wl_seed_pre_gross_prev_nibble[lane_count * dimm + byte_lane] = pdct_data->wl_seed_pre_gross_delay[lane_count * dimm + byte_lane];
				pdct_data->wl_seed_gross_prev_nibble[lane_count * dimm + byte_lane] = pdct_data->wl_gross_delay[lane_count * dimm + byte_lane];
				pdct_data->wl_seed_fine_prev_nibble[lane_count * dimm + byte_lane] = pdct_data->wl_fine_delay[lane_count * dimm + byte_lane];
			}
		} else {
			/* Restore seed values from lower nibble pass */
			if (is_fam15h()) {
				for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
					pdct_data->wl_seed_gross_delay[lane_count * dimm + byte_lane] = pdct_data->wl_seed_gross_prev_nibble[lane_count * dimm + byte_lane];
					pdct_data->wl_seed_fine_delay[lane_count * dimm + byte_lane] = pdct_data->wl_seed_fine_prev_nibble[lane_count * dimm + byte_lane];
					pdct_data->wl_seed_pre_gross_delay[lane_count * dimm + byte_lane] = pdct_data->wl_seed_pre_gross_prev_nibble[lane_count * dimm + byte_lane];

					pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] = pdct_data->wl_seed_pre_gross_prev_nibble[lane_count * dimm + byte_lane];
					pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] = pdct_data->wl_seed_fine_prev_nibble[lane_count * dimm + byte_lane];

					printk(BIOS_SPEW, "\tLane %02x new seed: %04x\n", byte_lane, ((pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] & 0x1f) << 5) | (pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] & 0x1f));
				}
			} else {
				for (byte_lane = 0; byte_lane < lane_count; byte_lane++) {
					pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] = pdct_data->wl_seed_gross_prev_nibble[lane_count * dimm + byte_lane];
					pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] = pdct_data->wl_seed_fine_prev_nibble[lane_count * dimm + byte_lane];

					printk(BIOS_SPEW, "\tLane %02x new seed: %04x\n", byte_lane, ((pdct_data->wl_gross_delay[lane_count * dimm + byte_lane] & 0x1f) << 5) | (pdct_data->wl_fine_delay[lane_count * dimm + byte_lane] & 0x1f));
				}
			}
		}
	}

	pdct_data->wl_prev_memclk_freq[dimm] = mem_clk_freq;
	set_wl_byte_delay(p_dct_stat, dct, byte_lane, dimm, 0, pass, lane_count);
}

/*-----------------------------------------------------------------------------
 *  void set_wl_byte_delay(struct DCTStatStruc *p_dct_stat, u8 dct, u8 byte_lane, u8 Dimm, u8 lane_count){
 *
 *  Description:
 *       This function writes the write levelization byte delay for the Phase
 *       Recovery control registers
 *
 *   Parameters:
 *       IN  OUT   *DCTData - Pointer to buffer with information about each DCT
 *       IN	Dimm - Dimm Number
 *		 DCTData->wl_gross_delay[index+byte_lane] - gross write delay for each
 *						     logical DIMM
 *		 DCTData->wl_fine_delay[index+byte_lane] - fine write delay for each
 *						    logical DIMM
 *		 byte_lane - target byte lane to write
 *	  target_addr -    0: write to DRAM phase recovery control register
 *			  1: write to DQS write register
 *       OUT
 *
 *-----------------------------------------------------------------------------
 */
void set_wl_byte_delay(struct DCTStatStruc *p_dct_stat, u8 dct, u8 byte_lane, u8 dimm, u8 target_addr, u8 pass, u8 lane_count)
{
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];
	u8 fine_start_loc, fine_end_loc, gross_start_loc, gross_end_loc, temp_b, index, offset_addr;
	u32 addr, fine_delay_value, gross_delay_value, value_low, value_high, ecc_value, temp_w;

	if (target_addr == 0)
	{
		index = (u8)(lane_count * dimm);
		value_low = 0;
		value_high = 0;
		byte_lane = 0;
		ecc_value = 0;
		while (byte_lane < lane_count)
		{
			/* This subtract 0xC workaround might be temporary. */
			if ((pdct_data->wl_pass == 2) && (pdct_data->reg_man_1_present & (1 << (dimm * 2 + dct)))) {
				temp_w = (pdct_data->wl_gross_delay[index + byte_lane] << 5) | pdct_data->wl_fine_delay[index + byte_lane];
				temp_w -= 0xC;
				pdct_data->wl_gross_delay[index + byte_lane] = (u8)(temp_w >> 5);
				pdct_data->wl_fine_delay[index + byte_lane] = (u8)(temp_w & 0x1F);
			}
			gross_delay_value = pdct_data->wl_gross_delay[index + byte_lane];
			/* Adjust seed gross delay overflow (greater than 3):
			 *      - Program seed gross delay as 2 (gross is 4 or 6) or 1 (gross is 5).
			 *      - Keep original seed gross delay for later reference.
			 */
			if (gross_delay_value >= 3)
				gross_delay_value = (gross_delay_value & 1) ? 1 : 2;
			fine_delay_value = pdct_data->wl_fine_delay[index + byte_lane];
			if (byte_lane < 4)
				value_low |= ((gross_delay_value << 5) | fine_delay_value) << 8 * byte_lane;
			else if (byte_lane < 8)
				value_high |= ((gross_delay_value << 5) | fine_delay_value) << 8 * (byte_lane - 4);
			else
				ecc_value = ((gross_delay_value << 5) | fine_delay_value);
			byte_lane++;
		}
		set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_CONT_ADD_PHASE_REC_CTRL_LOW, 0, 31, (u32)value_low);
		set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_CONT_ADD_PHASE_REC_CTRL_HIGH, 0, 31, (u32)value_high);
		set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				DRAM_CONT_ADD_ECC_PHASE_REC_CTRL, 0, 31, (u32)ecc_value);
	}
	else
	{
		/* Fam10h BKDG: Rev. 3.62 2.8.9.9.1 (6)
		 * Fam15h BKDG: Rev. 3.14 2.10.5.8.1
		 */
		index = (u8)(lane_count * dimm);
		gross_delay_value = pdct_data->wl_gross_delay[index + byte_lane];
		fine_delay_value = pdct_data->wl_fine_delay[index + byte_lane];

		temp_b = 0;
		offset_addr = (u8)(3 * dimm);
		if (byte_lane < 2) {
			temp_b = (u8)(16 * byte_lane);
			addr = DRAM_CONT_ADD_DQS_TIMING_CTRL_BL_01;
		} else if (byte_lane < 4) {
			temp_b = (u8)(16 * byte_lane);
			addr = DRAM_CONT_ADD_DQS_TIMING_CTRL_BL_01 + 1;
		} else if (byte_lane < 6) {
			temp_b = (u8)(16 * byte_lane);
			addr = DRAM_CONT_ADD_DQS_TIMING_CTRL_BL_45;
		} else if (byte_lane < 8) {
			temp_b = (u8)(16 * byte_lane);
			addr = DRAM_CONT_ADD_DQS_TIMING_CTRL_BL_45 + 1;
		} else {
			temp_b = 0;
			addr = DRAM_CONT_ADD_DQS_TIMING_CTRL_BL_01 + 2;
		}
		addr += offset_addr;

		fine_start_loc = (u8)(temp_b % 32);
		fine_end_loc = (u8)(fine_start_loc + 4);
		gross_start_loc = (u8)(fine_end_loc + 1);
		gross_end_loc = (u8)(gross_start_loc + 2);

		set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				(u16)addr, fine_start_loc, fine_end_loc,(u32)fine_delay_value);
		set_dct_addr_bits(pdct_data, dct, pdct_data->node_id, FUN_DCT,
				(u16)addr, gross_start_loc, gross_end_loc, (u32)gross_delay_value);

		pdct_data->wl_fine_delay_prev_pass[index + byte_lane] = fine_delay_value;
		pdct_data->wl_gross_delay_prev_pass[index + byte_lane] = gross_delay_value;
		if (pass == FIRST_PASS) {
			pdct_data->wl_fine_delay_first_pass[index + byte_lane] = fine_delay_value;
			pdct_data->wl_gross_delay_first_pass[index + byte_lane] = gross_delay_value;
			pdct_data->wl_critical_gross_delay_first_pass = pdct_data->wl_critical_gross_delay_prev_pass;
		}
	}

}

/*-----------------------------------------------------------------------------
 *  void getWLByteDelay(struct DCTStatStruc *p_dct_stat, u8 dct, u8 byte_lane, u8 Dimm, u8 Nibble, u8 lane_count)
 *
 *  Description:
 *       This function reads the write levelization byte delay from the Phase
 *       Recovery control registers
 *
 *   Parameters:
 *       IN  OUT   *DCTData - Pointer to buffer with information about each DCT
 *       IN	Dimm - Dimm Number
 *		 byte_lane - target byte lane to read
 *       OUT
 *		 DCTData->wl_gross_delay[index+byte_lane] - gross write delay for current
 *						     byte for logical DIMM
 *		 DCTData->wl_fine_delay[index+byte_lane] - fine write delay for current
 *						    byte for logical DIMM
 *
 *-----------------------------------------------------------------------------
 */
void getWLByteDelay(struct DCTStatStruc *p_dct_stat, u8 dct, u8 byte_lane, u8 dimm, u8 pass, u8 nibble, u8 lane_count)
{
	sDCTStruct *pdct_data = p_dct_stat->c_dct_ptr[dct];
	u8 fine_start_loc, fine_end_loc, gross_start_loc, gross_end_loc, temp_b, temp_b_1, index;
	u32 addr, fine, gross;
	temp_b = 0;
	index = (u8)(lane_count*dimm);
	if (byte_lane < 4) {
		temp_b = (u8)(8 * byte_lane);
		addr = DRAM_CONT_ADD_PHASE_REC_CTRL_LOW;
	} else if (byte_lane < 8) {
		temp_b_1 = (u8)(byte_lane - 4);
		temp_b = (u8)(8 * temp_b_1);
		addr = DRAM_CONT_ADD_PHASE_REC_CTRL_HIGH;
	} else {
		temp_b = 0;
		addr = DRAM_CONT_ADD_ECC_PHASE_REC_CTRL;
	}
	fine_start_loc = temp_b;
	fine_end_loc = (u8)(fine_start_loc + 4);
	gross_start_loc = (u8)(fine_end_loc + 1);
	gross_end_loc = (u8)(gross_start_loc + 1);

	fine = get_add_dct_bits(pdct_data, dct, pdct_data->node_id,
				FUN_DCT, (u16)addr, fine_start_loc, fine_end_loc);
	gross = get_add_dct_bits(pdct_data, dct, pdct_data->node_id,
				FUN_DCT, (u16)addr, gross_start_loc, gross_end_loc);

	printk(BIOS_SPEW, "\tLane %02x nibble %01x raw readback: %04x\n", byte_lane, nibble, ((gross & 0x1f) << 5) | (fine & 0x1f));

	/* Adjust seed gross delay overflow (greater than 3):
	 * - Adjust the trained gross delay to the original seed gross delay.
	 */
	if (pdct_data->wl_gross_delay[index + byte_lane] >= 3)
	{
		gross += pdct_data->wl_gross_delay[index + byte_lane];
		if (pdct_data->wl_gross_delay[index + byte_lane] & 1)
			gross -= 1;
		else
			gross -= 2;
	}
	else if ((pdct_data->wl_gross_delay[index + byte_lane] == 0) && (gross == 3))
	{
		/* If seed gross delay is 0 but PRE result gross delay is 3, it is negative.
		 * We will then round the negative number to 0.
		 */
		gross = 0;
		fine = 0;
	}
	printk(BIOS_SPEW, "\tLane %02x nibble %01x adjusted value (pre nibble): %04x\n", byte_lane, nibble, ((gross & 0x1f) << 5) | (fine & 0x1f));

	/* Nibble adjustments */
	if (nibble == 0) {
		pdct_data->wl_fine_delay[index + byte_lane] = (u8)fine;
		pdct_data->wl_gross_delay[index + byte_lane] = (u8)gross;
	} else {
		u32 wl_total_delay = ((pdct_data->wl_gross_delay[index + byte_lane] & 0x1f) << 5) | (pdct_data->wl_fine_delay[index + byte_lane] & 0x1f);
		wl_total_delay += ((gross & 0x1f) << 5) | (fine & 0x1f);
		wl_total_delay /= 2;
		pdct_data->wl_fine_delay[index + byte_lane] = (u8)(wl_total_delay & 0x1f);
		pdct_data->wl_gross_delay[index + byte_lane] = (u8)((wl_total_delay >> 5) & 0x1f);
	}
	printk(BIOS_SPEW, "\tLane %02x nibble %01x adjusted value (post nibble): %04x\n", byte_lane, nibble, ((pdct_data->wl_gross_delay[index + byte_lane] & 0x1f) << 5) | (pdct_data->wl_fine_delay[index + byte_lane] & 0x1f));
}
