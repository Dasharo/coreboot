/* SPDX-License-Identifier: GPL-2.0-only */

#include <drivers/amd/amdmct/wrappers/mcti.h>

void EarlySampleSupport_D(void)
{
}

u32 procOdtWorkaround(struct DCTStatStruc *p_dct_stat, u32 dct, u32 val)
{
	u64 tmp;
	tmp = p_dct_stat->logical_cpuid;
	if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {
		val &= 0x0FFFFFFF;
		if (p_dct_stat->ma_dimms[dct] > 1)
			val |= 0x10000000;
	}

	return val;
}


u32 OtherTiming_A_D(struct DCTStatStruc *p_dct_stat, u32 val)
{
	/* Bug#10695:One MEMCLK Bubble Writes Don't Do X4 X8 Switching Correctly
	 * Solution: BIOS should set DRAM Timing High[Twrwr] > 00b
	 * (F2x[1, 0]8C[1:0] > 00b).  Silicon Status: Fixed in Rev B
	 * FIXME: check if this is still required.
	 */
	u64 tmp;
	tmp = p_dct_stat->logical_cpuid;
	if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {
		if (!(val & (3 << 12)))
			val |= 1 << 12;
	}
	return val;
}


void mct_ForceAutoPrecharge_D(struct DCTStatStruc *p_dct_stat, u32 dct)
{
	u64 tmp;
	u32 reg;
	u32 reg_off;
	u32 dev;
	u32 val;

	tmp = p_dct_stat->logical_cpuid;
	if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {
		if (CheckNBCOFAutoPrechg(p_dct_stat, dct)) {
			dev = p_dct_stat->dev_dct;
			reg_off = 0x100 * dct;
			reg = 0x90 + reg_off;	/* Dram Configuration Lo */
			val = get_nb32(dev, reg);
			val |= 1 << FORCE_AUTO_PCHG;
			if (!p_dct_stat->ganged_mode)
				val |= 1 << BURST_LENGTH_32;
			set_nb32(dev, reg, val);

			reg = 0x88 + reg_off;	/* cx = Dram Timing Lo */
			val = get_nb32(dev, reg);
			val |= 0x000F0000;	/* Trc = 0Fh */
			set_nb32(dev, reg, val);
		}
	}
}


void mct_EndDQSTraining_D(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	/* Bug#13341: Prefetch is getting killed when the limit is reached in
	 * PREF_DRAM_TRAIN_MODE
	 * Solution: Explicitly clear the PREF_DRAM_TRAIN_MODE bit after training
	 * sequence in order to ensure resumption of normal HW prefetch
	 * behavior.
	 * NOTE -- this has been documented with a note at the end of this
	 * section in the  BKDG (although, admittedly, the note does not really
	 * stand out).
	 * Silicon Status: Fixed in Rev B (confirm)
	 * FIXME: check this.
	 */

	u64 tmp;
	u32 dev;
	u32 reg;
	u32 val;
	u32 Node;

	for (Node = 0; Node < MAX_NODES_SUPPORTED; Node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + Node;

		if (!p_dct_stat->node_present) break;

		tmp = p_dct_stat->logical_cpuid;
		if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {
			dev = p_dct_stat->dev_dct;
			reg = 0x11c;
			val = get_nb32(dev, reg);
			val &= ~(1 << PREF_DRAM_TRAIN_MODE);
			set_nb32(dev, reg, val);
		}
	}
}




void mct_BeforeDQSTrain_Samp_D(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Bug#15115: Uncertainty In The Sync Chain Leads To Setup Violations
	 *  In TX FIFO
	 * Solution: BIOS should program DRAM Control Register[RdPtrInit] = 5h,
	 * (F2x[1, 0]78[3:0] = 5h).
	 *   Silicon Status: Fixed In Rev B0
	 */

	/* Bug#15880: Determine validity of reset settings for DDR PHY timing
	 *   regi..
	 * Solution: At least, set WrDqs fine delay to be 0 for DDR2 training.
	 */

	u32 dev;
	u32 reg_off;
	u32 index_reg;
	u32 index;
	u32 reg;
	u32 val;
	u64 tmp;
	u32 Channel;

	tmp = p_dct_stat->logical_cpuid;
	if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {

		dev = p_dct_stat->dev_dct;
		index = 0;

		for (Channel = 0; Channel < 2; Channel++) {
			index_reg = 0x98 + 0x100 * Channel;
			val = get_nb32_index_wait(dev, index_reg, 0x0d004007);
			val |= 0x3ff;
			set_nb32_index_wait(dev, index_reg, 0x0d0f4f07, val);
		}

		for (Channel = 0; Channel < 2; Channel++) {
			if (p_dct_stat->ganged_mode && Channel)
				break;
			reg_off = 0x100 * Channel;
			reg = 0x78 + reg_off;
			val = get_nb32(dev, reg);
			val &= ~(0x07);
			val |= 5;
			set_nb32(dev, reg, val);
		}

		for (Channel = 0; Channel < 2; Channel++) {
			reg_off = 0x100 * Channel;
			val = 0;
			index_reg = 0x98 + reg_off;
			for (index = 0x30; index < (0x45 + 1); index++) {
				set_nb32_index_wait(dev, index_reg, index, val);
			}
		}

	}
}


u32 Modify_D3CMP(struct DCTStatStruc *p_dct_stat, u32 dct, u32 value)
{
	/* Errata#189: Reads To Phy Driver Calibration Register and Phy
	 *  Predriver Calibration Register Do Not Return Bit 27.
	 * Solution: See #41322 for details.
	 * BIOS can modify bit 27 of the Phy Driver Calibration register
	 * as follows:
	 *  1. Read F2x[1, 0]9C_x09
	 *  2. Read F2x[1, 0]9C_x0D004201
	 *  3. Set F2x[1, 0]9C_x09[27] = F2x[1, 0]9C_x0D004201[10]
	 * BIOS can modify bit 27 of the Phy Predriver Calibration register
	 * as follows:
	 *  1. Read F2x[1, 0]9C_x0A
	 *  2. Read F2x[1, 0]9C_x0D004209
	 *  3. Set F2x[1, 0]9C_x0A[27] = F2x[1, 0]9C_x0D004209[10]
	 * Silicon Status: Fixed planned for DR-B0
	 */

	u32 dev;
	u32 index_reg;
	u32 index;
	u32 val;
	u64 tmp;

	tmp = p_dct_stat->logical_cpuid;
	if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {
		dev = p_dct_stat->dev_dct;
		index_reg = 0x98 + 0x100 * dct;
		index = 0x0D004201;
		val = get_nb32_index_wait(dev, index_reg, index);
		value &= ~(1 << 27);
		value |= ((val >> 10) & 1) << 27;
	}
	return value;
}


void SyncSetting(struct DCTStatStruc *p_dct_stat)
{
	/* Errata#198: AddrCmdSetup, CsOdtSetup, and CkeSetup Require Identical
	 * Programming For Both Channels in Ganged Mode
	 * Solution: The BIOS must program the following DRAM timing parameters
	 * the same for both channels:
	 *  1. F2x[1, 0]9C_x04[21] (AddrCmdSetup)
	 *  2. F2x[1, 0]9C_x04[15] (CsOdtSetup)
	 *  3. F2x[1, 0]9C_x04[5]) (CkeSetup)
	 * That is, if the AddrCmdSetup, CsOdtSetup, or CkeSetup is
	 * set to 1'b1 for one of the controllers, then the corresponding
	 * AddrCmdSetup, CsOdtSetup, or CkeSetup must be set to 1'b1 for the
	 * other controller.
	 * Silicon Status: Fix TBD
	 */

	u64 tmp;
	tmp = p_dct_stat->logical_cpuid;
	if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {
		p_dct_stat->ch_odc_ctl[1] = p_dct_stat->ch_odc_ctl[0];
		p_dct_stat->ch_addr_tmg[1] = p_dct_stat->ch_addr_tmg[0];
	}
}


u32 CheckNBCOFAutoPrechg(struct DCTStatStruc *p_dct_stat, u32 dct)
{
	u32 ret = 0;
	u32 lo, hi;
	u32 msr;
	u32 val;
	u32 valx, valy;
	u32 NbDid;

	/* 3 * (Fn2xD4[NBFid]+4)/(2^NbDid)/(3+Fn2x94[MemClkFreq]) */
	msr = 0xC0010071;
	_RDMSR(msr, &lo, &hi);
	NbDid = (lo >> 22) & 1;

	val = get_nb32(p_dct_stat->dev_dct, 0x94 + 0x100 * dct);
	valx = ((val & 0x07) + 3) << NbDid;
	print_tx("MemClk:", valx >> NbDid);

	val = get_nb32(p_dct_stat->dev_nbmisc, 0xd4);
	valy = ((val & 0x1f) + 4) * 3;
	print_tx("NB COF:", valy >> NbDid);

	val = valy/valx;
	if ((val == 3) && (valy % valx))  /* 3 < NClk/MemClk < 4 */
		ret = 1;

	return ret;
}


void mct_BeforeDramInit_D(struct DCTStatStruc *p_dct_stat, u32 dct)
{
	u64 tmp;
	u32 Speed;
	u32 ch, ch_start, ch_end;
	u32 index_reg;
	u32 dev;
	u32 val;

	tmp = p_dct_stat->logical_cpuid;
	if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {
		Speed = p_dct_stat->speed;
		/* MemClkFreq = 333MHz or 533MHz */
		if ((Speed == 3) || (Speed == 2)) {
			if (p_dct_stat->ganged_mode) {
				ch_start = 0;
				ch_end = 2;
			} else {
				ch_start = dct;
				ch_end = dct + 1;
			}
			dev = p_dct_stat->dev_dct;

			for (ch = ch_start; ch < ch_end; ch++) {
				index_reg = 0x98 + 0x100 * ch;
				val = get_nb32_index(dev, index_reg, 0x0D00E001);
				val &= ~(0xf0);
				val |= 0x80;
				set_nb32_index(dev, index_reg, 0x0D01E001, val);
			}
		}

	}
}

#ifdef UNUSED_CODE
/* Callback not required */
static u8 mct_AdjustDelay_D(struct DCTStatStruc *p_dct_stat, u8 dly)
{
	u8 skip = 0;
	dly &= 0x1f;
	if ((dly >= MIN_FENCE) && (dly <= MAX_FENCE))
		skip = 1;

	return skip;
}
#endif

u8 mct_checkFenceHoleAdjust_D(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 DQSDelay,
				u8 ChipSel,  u8 *result)
{
	u8 ByteLane;
	u64 tmp;

	tmp = p_dct_stat->logical_cpuid;
	if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {
		if (p_dct_stat->direction == DQS_WRITEDIR) {
			if ((p_dct_stat->speed == 2) || (p_dct_stat->speed == 3)) {
				if (DQSDelay == 13) {
					if (*result == 0xFF) {
						for (ByteLane = 0; ByteLane < 8; ByteLane++) {
							p_dct_stat->dqs_delay = 13;
							p_dct_stat->byte_lane = ByteLane;
							/* store the value into the data structure */
							StoreDQSDatStrucVal_D(p_mct_stat, p_dct_stat, ChipSel);
						}
						return 1;
					}
				}
			}
			if (mct_AdjustDQSPosDelay_D(p_dct_stat, DQSDelay)) {
				*result = 0;
			}
		}
	}
	return 0;
}


u8 mct_AdjustDQSPosDelay_D(struct DCTStatStruc *p_dct_stat, u8 dly)
{
	u8 skip = 0;

	dly &= 0x1f;
	if ((dly >= MIN_DQS_WR_FENCE) && (dly <= MAX_DQS_WR_FENCE))
		skip = 1;

	return skip;

}

#ifdef UNUSED_CODE
static u8 mctDoAxRdPtrInit_D(struct DCTStatStruc *p_dct_stat, u8 *Rdtr)
{
	u32 tmp;

	tmp = p_dct_stat->logical_cpuid;
	if ((tmp == AMD_DR_A0A) || (tmp == AMD_DR_A1B) || (tmp == AMD_DR_A2)) {
		*Rdtr = 5;
		return 1;
	}
	return 0;
}
#endif

void mct_AdjustScrub_D(struct DCTStatStruc *p_dct_stat, u16 *scrub_request) {

	/* Erratum #202: disable DCache scrubber for Ax parts */

	if (p_dct_stat->logical_cpuid & (AMD_DR_Ax)) {
		*scrub_request = 0;
		p_dct_stat->err_status |= 1 << SB_DCBKScrubDis;
	}
}

void beforeInterleaveChannels_D(struct DCTStatStruc *p_dct_stat_a, u8 *enabled) {
	if (p_dct_stat_a->logical_cpuid & (AMD_DR_Ax))
		*enabled = 0;
}
