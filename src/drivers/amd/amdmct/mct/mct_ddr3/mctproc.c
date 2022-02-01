/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

/* mct_SetDramConfigMisc2_Cx & mct_SetDramConfigMisc2_Dx */
u32 mct_set_dram_config_misc_2(struct DCTStatStruc *p_dct_stat,
				u8 dct, u32 misc2, u32 dram_control)
{
	u32 val;

	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	if (p_dct_stat->logical_cpuid & AMD_FAM15_ALL) {
		u8 cs_mux_45;
		u8 cs_mux_67;
		u32 f2x80;

		misc2 &= ~(0x1 << 28);			/* FastSelfRefEntryDis = 0x0 */
		if (max_dimms_installable == 3) {
			/* FIXME 3 DIMMS per channel unimplemented */
			cs_mux_45 = 0;
		} else {
			u32 f2x60 = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x60);
			f2x80 = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x80);
			if ((((f2x80 & 0xf) == 0x7) || ((f2x80 & 0xf) == 0x9))
				&& ((f2x60 & 0x3) == 0x3))
				cs_mux_45 = 1;
			else if ((((f2x80 & 0xf) == 0xa) || ((f2x80 & 0xf) == 0xb))
				&& ((f2x60 & 0x3) > 0x1))
				cs_mux_45 = 1;
			else
				cs_mux_45 = 0;
		}

		if (max_dimms_installable == 1) {
			cs_mux_67 = 0;
		} else if (max_dimms_installable == 2) {
			u32 f2x64 = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x64);
			f2x80 = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x80);
			if (((((f2x80 >> 4) & 0xf) == 0x7) || (((f2x80 >> 4) & 0xf) == 0x9))
				&& ((f2x64 & 0x3) == 0x3))
				cs_mux_67 = 1;
			else if (((((f2x80 >> 4) & 0xf) == 0xa) || (((f2x80 >> 4) & 0xf) == 0xb))
				&& ((f2x64 & 0x3) > 0x1))
				cs_mux_67 = 1;
			else
				cs_mux_67 = 0;
		} else {
			/* FIXME 3 DIMMS per channel unimplemented */
			cs_mux_67 = 0;
		}

		misc2 &= ~(0x1 << 27);		/* CsMux67 = cs_mux_67 */
		misc2 |= ((cs_mux_67 & 0x1) << 27);
		misc2 &= ~(0x1 << 26);		/* CsMux45 = cs_mux_45 */
		misc2 |= ((cs_mux_45 & 0x1) << 26);
	} else if (p_dct_stat->logical_cpuid & (AMD_DR_Dx | AMD_DR_Cx)) {
		if (p_dct_stat->Status & (1 << SB_REGISTERED)) {
			misc2 |= 1 << SUB_MEM_CLK_REG_DLY;
			if (mct_get_nv_bits(NV_MAX_DIMMS) == 8)
				misc2 |= 1 << DDR3_FOUR_SOCKET_CH;
			else
				misc2 &= ~(1 << DDR3_FOUR_SOCKET_CH);
		}

		if (p_dct_stat->logical_cpuid & AMD_DR_Cx)
			misc2 |= 1 << ODT_SWIZZLE;

		val = dram_control;
		val &= 7;
		val = ((~val) & 0xff) + 1;
		val += 6;
		val &= 0x7;
		misc2 &= 0xfff8ffff;
		misc2 |= val << 16;	/* DataTxFifoWrDly */
		if (p_dct_stat->logical_cpuid & AMD_DR_Dx)
			misc2 |= 1 << 7; /* ProgOdtEn */
	}
	return misc2;
}

void mct_ext_mct_config_cx(struct DCTStatStruc *p_dct_stat)
{
	u32 val;

	if (p_dct_stat->logical_cpuid & (AMD_DR_Cx)) {
		/* Revision C */
		set_nb32(p_dct_stat->dev_dct, 0x11c, 0x0ce00fc0 | 1 << 29/* FLUSH_WR_ON_STP_GNT */);

		val = get_nb32(p_dct_stat->dev_dct, 0x1b0);
		val &= ~0x73f;
		val |= 0x101;	/* BKDG recommended settings */

		set_nb32(p_dct_stat->dev_dct, 0x1b0, val);
	}
}
