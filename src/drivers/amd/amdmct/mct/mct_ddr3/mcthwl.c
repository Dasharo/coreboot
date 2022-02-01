/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <stdint.h>
#include <console/console.h>
#include <string.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static void set_ecc_wr_dqs_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat)
{
	u8 byte_lane, dimm_num, odd_byte, addl_index, channel;
	u8 ecc_ref1, ecc_ref2, ecc_dqs_scale;
	u32 val;
	u16 word;

	for (channel = 0; channel < 2; channel ++) {
		for (dimm_num = 0; dimm_num < C_MAX_DIMMS; dimm_num ++) { /* we use dimm_num instead of DimmNumx3 */
			for (byte_lane = 0; byte_lane < 9; byte_lane ++) {
				/* Get RxEn initial value from WrDqs */
				if (byte_lane & 1)
					odd_byte = 1;
				else
					odd_byte = 0;
				if (byte_lane < 2)
					addl_index = 0x30;
				else if (byte_lane < 4)
					addl_index = 0x31;
				else if (byte_lane < 6)
					addl_index = 0x40;
				else if (byte_lane < 8)
					addl_index = 0x41;
				else
					addl_index = 0x32;
				addl_index += dimm_num * 3;

				val = get_nb32_index_wait_dct(p_dct_stat->dev_dct, channel, 0x98, addl_index);
				if (odd_byte)
					val >>= 16;
				/* Save WrDqs to stack for later usage */
				p_dct_stat->persistent_data.ch_d_b_tx_dqs[channel][dimm_num][byte_lane] = val & 0xFF;
				ecc_dqs_scale = p_dct_stat->ch_ecc_dqs_scale[channel];
				word = p_dct_stat->ch_ecc_dqs_like[channel];
				if ((word & 0xFF) == byte_lane) ecc_ref1 = val & 0xFF;
				if (((word >> 8) & 0xFF) == byte_lane) ecc_ref2 = val & 0xFF;
			}
		}
	}
}

static void enable_auto_refresh_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat)
{
	u32 val;

	val = get_nb32_dct(p_dct_stat->dev_dct, 0, 0x8C);
	val &= ~(1 << DIS_AUTO_REFRESH);
	set_nb32_dct(p_dct_stat->dev_dct, 0, 0x8C, val);

	val = get_nb32_dct(p_dct_stat->dev_dct, 1, 0x8C);
	val &= ~(1 << DIS_AUTO_REFRESH);
	set_nb32_dct(p_dct_stat->dev_dct, 1, 0x8C, val);
}

static void disable_auto_refresh_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u32 val;

	val = get_nb32_dct(p_dct_stat->dev_dct, 0, 0x8C);
	val |= 1 << DIS_AUTO_REFRESH;
	set_nb32_dct(p_dct_stat->dev_dct, 0, 0x8C, val);

	val = get_nb32_dct(p_dct_stat->dev_dct, 1, 0x8C);
	val |= 1 << DIS_AUTO_REFRESH;
	set_nb32_dct(p_dct_stat->dev_dct, 1, 0x8C, val);
}


static u8 phy_wl_pass1(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 dimm;
	u16 dimm_valid;
	u8 status = 0;
	void *dct_ptr;

	dct &= 1;

	dct_ptr = (void *)(p_dct_stat->c_dct_ptr[dct]);
	p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[dct];
	p_dct_stat->cs_present = p_dct_stat->cs_present_dct[dct];

	if (p_dct_stat->ganged_mode & 1)
		p_dct_stat->cs_present = p_dct_stat->cs_present_dct[0];

	if (p_dct_stat->dimm_valid) {
		dimm_valid = p_dct_stat->dimm_valid;
		prepare_c_dct(p_mct_stat, p_dct_stat, dct);
		for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm ++) {
			if (dimm_valid & (1 << (dimm << 1))) {
				status |= agesa_hw_wl_phase1(p_mct_stat, p_dct_stat, dct, dimm, FIRST_PASS);
				status |= agesa_hw_wl_phase2(p_mct_stat, p_dct_stat, dct, dimm, FIRST_PASS);
				status |= agesa_hw_wl_phase3(p_mct_stat, p_dct_stat, dct, dimm, FIRST_PASS);
			}
		}
	}

	return status;
}

static u8 phy_wl_pass2(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct, u8 final)
{
	u8 dimm;
	u16 dimm_valid;
	u8 status = 0;
	void *dct_ptr;

	dct &= 1;

	dct_ptr = (void *)&(p_dct_stat->c_dct_ptr[dct]); /* todo: */
	p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[dct];
	p_dct_stat->cs_present = p_dct_stat->cs_present_dct[dct];

	if (p_dct_stat->ganged_mode & 1)
		p_dct_stat->cs_present = p_dct_stat->cs_present_dct[0];

	if (p_dct_stat->dimm_valid) {
		dimm_valid = p_dct_stat->dimm_valid;
		prepare_c_dct(p_mct_stat, p_dct_stat, dct);
		p_dct_stat->speed = p_dct_stat->dimm_auto_speed = p_dct_stat->target_freq;
		p_dct_stat->cas_latency = p_dct_stat->dimm_casl = p_dct_stat->target_casl;
		spd_2nd_timing(p_mct_stat, p_dct_stat, dct);
		if (!is_fam15h()) {
			prog_dram_mrs_reg_d(p_mct_stat, p_dct_stat, dct);
			platform_spec_d(p_mct_stat, p_dct_stat, dct);
			fence_dyn_training_d(p_mct_stat, p_dct_stat, dct);
		}
		restore_on_dimm_mirror(p_mct_stat, p_dct_stat);
		startup_dct_d(p_mct_stat, p_dct_stat, dct);
		clear_on_dimm_mirror(p_mct_stat, p_dct_stat);
		set_dll_speed_up_d(p_mct_stat, p_dct_stat, dct);
		disable_auto_refresh_d(p_mct_stat, p_dct_stat);
		for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm ++) {
			if (dimm_valid & (1 << (dimm << 1))) {
				status |= agesa_hw_wl_phase1(p_mct_stat, p_dct_stat, dct, dimm, SECOND_PASS);
				status |= agesa_hw_wl_phase2(p_mct_stat, p_dct_stat, dct, dimm, SECOND_PASS);
				status |= agesa_hw_wl_phase3(p_mct_stat, p_dct_stat, dct, dimm, SECOND_PASS);
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
static void write_levelization_hw(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a, u8 node, u8 pass)
{
	u8 status;
	u8 timeout;
	u16 final_target_freq;

	struct DCTStatStruc *p_dct_stat;
	p_dct_stat = p_dct_stat_a + node;

	p_dct_stat->c_mct_ptr  = &(p_dct_stat->s_c_mct_ptr);
	p_dct_stat->c_dct_ptr[0] = &(p_dct_stat->s_c_dct_ptr[0]);
	p_dct_stat->c_dct_ptr[1] = &(p_dct_stat->s_c_dct_ptr[1]);

	/* Disable auto refresh by configuring F2x[1, 0]8C[DIS_AUTO_REFRESH] = 1 */
	disable_auto_refresh_d(p_mct_stat, p_dct_stat);

	/* Disable ZQ calibration short command by F2x[1,0]94[ZqcsInterval]=00b */
	disable_zq_calibration(p_mct_stat, p_dct_stat);
	prepare_c_mct(p_mct_stat, p_dct_stat);

	if (p_dct_stat->ganged_mode & (1 << 0)) {
		p_dct_stat->dimm_valid_dct[1] = p_dct_stat->dimm_valid_dct[0];
	}

	if (pass == FIRST_PASS) {
		timeout = 0;
		do {
			status = 0;
			timeout++;
			status |= phy_wl_pass1(p_mct_stat, p_dct_stat, 0);
			status |= phy_wl_pass1(p_mct_stat, p_dct_stat, 1);
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

	if (pass == SECOND_PASS) {
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
				set_target_freq(p_mct_stat, p_dct_stat_a, node);
				timeout = 0;
				do {
					status = 0;
					timeout++;
					status |= phy_wl_pass2(p_mct_stat, p_dct_stat, 0, (p_dct_stat->target_freq == final_target_freq));
					status |= phy_wl_pass2(p_mct_stat, p_dct_stat, 1, (p_dct_stat->target_freq == final_target_freq));
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
				memcpy(pDCTData->wl_gross_delay_final_pass, pDCTData->wl_gross_delay_prev_pass, sizeof(pDCTData->wl_gross_delay_prev_pass));
				memcpy(pDCTData->wl_fine_delay_final_pass, pDCTData->wl_fine_delay_prev_pass, sizeof(pDCTData->wl_fine_delay_prev_pass));
				pDCTData->wl__critical_gross_delay_final_pass = pDCTData->wl_critical_gross_delay_prev_pass;
			}
		}
	}

	set_ecc_wr_dqs_d(p_mct_stat, p_dct_stat);
	enable_auto_refresh_d(p_mct_stat, p_dct_stat);
	enable_zq_calibration(p_mct_stat, p_dct_stat);
}

void mct_write_levelization_hw(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a, u8 pass)
{
	u8 node;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;

		if (p_dct_stat->node_present) {
			mct_smb_hub_init(node);
			clear_on_dimm_mirror(p_mct_stat, p_dct_stat);
			write_levelization_hw(p_mct_stat, p_dct_stat_a, node, pass);
			restore_on_dimm_mirror(p_mct_stat, p_dct_stat);
		}
	}
}
