/* SPDX-License-Identifier: GPL-2.0-only */

#include <drivers/amd/amdmct/wrappers/mcti.h>

void interleave_channels_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a)
{

	u8 node;
	u32 dram_base, dct_sel_base;
	u8 dct_sel_int_lv_addr, dct_sel_hi;
	u8 hole_valid = 0;
	u32 hole_size, hole_base = 0;
	u32 val, tmp;
	u32 dct0_size, dct1_size;
	u8 enabled;
	struct DCTStatStruc *p_dct_stat;

	/* hole_valid - indicates whether the current node contains hole.
	 * hole_size - indicates whether there is IO hole in the whole system
	 * memory.
	 */

	/* call back to wrapper not needed ManualChannelInterleave_D(); */
	/* override interleave */
	/* call back - dct_sel_int_lv_addr = mct_get_nv_bits(NV_CHANNEL_INTLV);*/
	// FIXME: Check for Cx
	/* typ = 5: Hash*: exclusive OR of address bits[20:16, 6]. */
	dct_sel_int_lv_addr = mct_get_nv_bits(NV_CHANNEL_INTLV);
	before_interleave_channels_d(p_dct_stat_a, &enabled);

	if (dct_sel_int_lv_addr & 1) {
		dct_sel_int_lv_addr >>= 1;
		hole_size = 0;
		if ((p_mct_stat->g_status & (1 << GSB_SOFT_HOLE)) ||
		     (p_mct_stat->g_status & (1 << GSB_HW_HOLE))) {
			if (p_mct_stat->hole_base) {
				hole_base = p_mct_stat->hole_base >> 8;
				hole_size = hole_base & 0xFFFF0000;
				hole_size |= ((~hole_base) + 1) & 0xFFFF;
			}
		}
		node = 0;
		while (node < MAX_NODES_SUPPORTED) {
			p_dct_stat = p_dct_stat_a + node;
			val = get_nb32(p_dct_stat->dev_map, 0xF0);
			if (val & (1 << DRAM_HOLE_VALID))
				hole_valid = 1;
			if (!p_dct_stat->ganged_mode && p_dct_stat->dimm_valid_dct[0]
			&& p_dct_stat->dimm_valid_dct[1]) {
				dram_base = p_dct_stat->node_sys_base >> 8;
				dct1_size = ((p_dct_stat->node_sys_limit) + 2) >> 8;
				dct0_size = get_nb32(p_dct_stat->dev_dct, 0x114);
				if (dct0_size >= 0x10000) {
					dct0_size -= hole_size;
				}

				dct0_size -= dram_base;
				dct1_size -= dct0_size;
				/* DctSelHiRngEn = 1, dct_sel_hi = 0 */
				dct_sel_hi = 0x05;
				if (dct1_size == dct0_size) {
					dct1_size = 0;
					/* DctSelHiRngEn = 0 */
					dct_sel_hi = 0x04;
				} else if (dct1_size > dct0_size) {
					dct1_size = dct0_size;
					/* DctSelHiRngEn = 1, dct_sel_hi = 1 */
					dct_sel_hi = 0x07;
				}
				dct0_size = dct1_size;
				dct0_size += dram_base;
				dct0_size += dct1_size;
				/* if DctSelBaseAddr > hole_base */
				if (dct0_size >= hole_base)
					dct0_size += hole_size;
				dct_sel_base = dct0_size;

				if (dct1_size == 0)
					dct0_size = 0;
				/* DctSelBaseOffset = DctSelBaseAddr - Interleaved region */
				dct0_size -= dct1_size;
				set_nb32(p_dct_stat->dev_dct, 0x114, dct0_size);

				if (dct1_size == 0)
					dct1_size = dct_sel_base;
				val = get_nb32(p_dct_stat->dev_dct, 0x110);
				val &= 0x7F8;
				val |= dct1_size;
				val |= dct_sel_hi;
				val |= (dct_sel_int_lv_addr << 6) & 0xFF;
				set_nb32(p_dct_stat->dev_dct, 0x110, val);
				print_tx("InterleaveChannels: F2x110 DRAM Controller Select Low Register = ",
					val);

				if (hole_valid) {
					tmp = dram_base;
					val = dct_sel_base;
					/* DctSelBaseAddr < DramHoleBase */
					if (val < hole_base) {
						val -= dram_base;
						val >>= 1;
						tmp += val;
					}
					tmp += hole_size;
					/* DramHoleOffset */
					val = get_nb32(p_dct_stat->dev_map, 0xF0);
					val &= 0xFFFF007F;
					val |= (tmp & ~0xFFFF007F);
					set_nb32(p_dct_stat->dev_map, 0xF0, val);
					print_tx("InterleaveChannels: F1xF0 DRAM Hole Address Register = ",
						val);

				}
			}
			print_tx("interleave_channels_d: node ", node);
			print_tx("interleave_channels_d: status ", p_dct_stat->status);
			print_tx("interleave_channels_d: err_status ", p_dct_stat->err_status);
			print_tx("interleave_channels_d: err_code ", p_dct_stat->err_code);
			node++;
		}
	}
	print_t("interleave_channels_d: Done\n");
}
