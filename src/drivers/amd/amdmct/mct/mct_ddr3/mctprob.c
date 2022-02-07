/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

void mct_before_dqs_train_samp(struct DCTStatStruc *p_dct_stat)
{
	u32 val;

	if (p_dct_stat->logical_cpuid & AMD_DR_Bx) {
		set_nb32(p_dct_stat->dev_dct, 0x98, 0x0D004007);
		val = get_nb32(p_dct_stat->dev_dct, 0x9C);
		val |= 0x3FF;
		set_nb32(p_dct_stat->dev_dct, 0x9C, val);
		set_nb32(p_dct_stat->dev_dct, 0x98, 0x4D0F4F07);

		set_nb32(p_dct_stat->dev_dct, 0x198, 0x0D004007);
		val = get_nb32(p_dct_stat->dev_dct, 0x19C);
		val |= 0x3FF;
		set_nb32(p_dct_stat->dev_dct, 0x19C, val);
		set_nb32(p_dct_stat->dev_dct, 0x198, 0x4D0F4F07);
	}
}

void mct_ext_mct_config_bx(struct DCTStatStruc *p_dct_stat)
{
	if (p_dct_stat->logical_cpuid & (AMD_DR_Bx)) {
		set_nb32(p_dct_stat->dev_dct, 0x11C, 0x0FE40FC0 | 1 << 29/* FLUSH_WR_ON_STP_GNT */);
	}
}
