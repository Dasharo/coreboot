/* SPDX-License-Identifier: GPL-2.0-only */

static const u8 Tab_GRCLKDis[] = {	8,0,8,8,0,0,8,0 };


u32 mct_AdjustMemClkDis_GR(struct DCTStatStruc *p_dct_stat, u32 dct,
				u32 DramTimingLo)
{
	/* Greayhound format -> Griffin format */
	u32 NewDramTimingLo;
	u32 dev = p_dct_stat->dev_dct;
	u32 reg;
	u32 reg_off = 0x100 * dct;
	u32 val;
	int i;

	DramTimingLo = val;
	/* Dram Timing Low (owns Clock Enable bits) */
	NewDramTimingLo = get_nb32(dev, 0x88 + reg_off);
	if (mct_get_nv_bits(NV_ALL_MEM_CLKS) == 0) {
		/*Special Jedec SPD diagnostic bit - "enable all clocks"*/
		if (!(p_dct_stat->Status & (1 << SB_DIAG_CLKS))) {
			for (i = 0; i < MAX_DIMMS_SUPPORTED; i++) {
				val = Tab_GRCLKDis[i];
				if (val < 8) {
					if (!(p_dct_stat->dimm_valid_dct[dct] & (1 << val))) {
						/* disable memclk */
						NewDramTimingLo |= (1 << (i + 1));
					}
				}
			}
		}
	}
	DramTimingLo &= ~(0xff << 24);
	DramTimingLo |= NewDramTimingLo & (0xff << 24);
	DramTimingLo &= (0x4d << 24); /* FIXME - enable all MemClks for now */

	return DramTimingLo;
}


u32 mct_AdjustDramConfigLo_GR(struct DCTStatStruc *p_dct_stat, u32 dct, u32 val)
{
	/* Greayhound format -> Griffin format */
	/*FIXME - BURST_LENGTH_32 must be 0 when F3x44[DramEccEn]=1. */
/*
			; mov	cx,PA_NBMISC+44h	;MCA NB Configuration
			; call Get_NB32n_D
			; bt eax,22				;EccEn
			; .if (CARRY?)
				; btr eax,BURST_LENGTH_32
			; .endif
*/
	return val;
}


void mct_AdjustMemHoist_GR(struct DCTStatStruc *p_dct_stat, u32 base, u32 HoleSize)
{
	u32 val;
	if (base >= p_dct_stat->dct_hole_base) {
		u32 dev = p_dct_stat->dev_dct;
		base += HoleSize;
		base >>= 27 - 8;
		val = get_nb32(dev, 0x110);
		val &= ~(0xfff << 11);
		val |= (base & 0xfff) << 11;
		set_nb32(dev, 0x110, val);
	}
}
