/* SPDX-License-Identifier: GPL-2.0-only */

static const u8 tab_gr_clk_dis[] = {	8,0,8,8,0,0,8,0 };


u32 mct_adjust_mem_clk_dis_gr(struct DCTStatStruc *p_dct_stat, u32 dct,
				u32 dram_timing_lo)
{
	/* Greayhound format -> Griffin format */
	u32 new_dram_timing_lo;
	u32 dev = p_dct_stat->dev_dct;
	u32 reg;
	u32 reg_off = 0x100 * dct;
	u32 val;
	int i;

	dram_timing_lo = val;
	/* Dram Timing Low (owns Clock Enable bits) */
	new_dram_timing_lo = get_nb32(dev, 0x88 + reg_off);
	if (mct_get_nv_bits(NV_ALL_MEM_CLKS) == 0) {
		/*Special Jedec SPD diagnostic bit - "enable all clocks"*/
		if (!(p_dct_stat->Status & (1 << SB_DIAG_CLKS))) {
			for (i = 0; i < MAX_DIMMS_SUPPORTED; i++) {
				val = tab_gr_clk_dis[i];
				if (val < 8) {
					if (!(p_dct_stat->dimm_valid_dct[dct] & (1 << val))) {
						/* disable memclk */
						new_dram_timing_lo |= (1 << (i + 1));
					}
				}
			}
		}
	}
	dram_timing_lo &= ~(0xff << 24);
	dram_timing_lo |= new_dram_timing_lo & (0xff << 24);
	dram_timing_lo &= (0x4d << 24); /* FIXME - enable all MemClks for now */

	return dram_timing_lo;
}


u32 mct_adjust_dram_config_lo_gr(struct DCTStatStruc *p_dct_stat, u32 dct, u32 val)
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


void mct_adjust_mem_hoist_gr(struct DCTStatStruc *p_dct_stat, u32 base, u32 hole_size)
{
	u32 val;
	if (base >= p_dct_stat->dct_hole_base) {
		u32 dev = p_dct_stat->dev_dct;
		base += hole_size;
		base >>= 27 - 8;
		val = get_nb32(dev, 0x110);
		val &= ~(0xfff << 11);
		val |= (base & 0xfff) << 11;
		set_nb32(dev, 0x110, val);
	}
}
