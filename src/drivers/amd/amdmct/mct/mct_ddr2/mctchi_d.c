/* SPDX-License-Identifier: GPL-2.0-only */

#include <drivers/amd/amdmct/wrappers/mcti.h>

void InterleaveChannels_D(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a)
{

	u8 Node;
	u32 DramBase, DctSelBase;
	u8 DctSelIntLvAddr, DctSelHi;
	u8 HoleValid = 0;
	u32 HoleSize, HoleBase = 0;
	u32 val, tmp;
	u32 dct0_size, dct1_size;
	u8 enabled;
	struct DCTStatStruc *p_dct_stat;

	/* HoleValid - indicates whether the current Node contains hole.
	 * HoleSize - indicates whether there is IO hole in the whole system
	 * memory.
	 */

	/* call back to wrapper not needed ManualChannelInterleave_D(); */
	/* call back - DctSelIntLvAddr = mct_get_nv_bits(NV_ChannelIntlv);*/	/* override interleave */
	// FIXME: Check for Cx
	DctSelIntLvAddr = mct_get_nv_bits(NV_ChannelIntlv); /* typ = 5: Hash*: exclusive OR of address bits[20:16, 6]. */
	beforeInterleaveChannels_D(p_dct_stat_a, &enabled);

	if (DctSelIntLvAddr & 1) {
		DctSelIntLvAddr >>= 1;
		HoleSize = 0;
		if ((p_mct_stat->g_status & (1 << GSB_SOFT_HOLE)) ||
		     (p_mct_stat->g_status & (1 << GSB_HW_HOLE))) {
			if (p_mct_stat->hole_base) {
				HoleBase = p_mct_stat->hole_base >> 8;
				HoleSize = HoleBase & 0xFFFF0000;
				HoleSize |= ((~HoleBase) + 1) & 0xFFFF;
			}
		}
		Node = 0;
		while (Node < MAX_NODES_SUPPORTED) {
			p_dct_stat = p_dct_stat_a + Node;
			val = get_nb32(p_dct_stat->dev_map, 0xF0);
			if (val & (1 << DRAM_HOLE_VALID))
				HoleValid = 1;
			if (!p_dct_stat->ganged_mode && p_dct_stat->dimm_valid_dct[0] && p_dct_stat->dimm_valid_dct[1]) {
				DramBase = p_dct_stat->node_sys_base >> 8;
				dct1_size = ((p_dct_stat->node_sys_limit) + 2) >> 8;
				dct0_size = get_nb32(p_dct_stat->dev_dct, 0x114);
				if (dct0_size >= 0x10000) {
					dct0_size -= HoleSize;
				}

				dct0_size -= DramBase;
				dct1_size -= dct0_size;
				DctSelHi = 0x05;		/* DctSelHiRngEn = 1, DctSelHi = 0 */
				if (dct1_size == dct0_size) {
					dct1_size = 0;
					DctSelHi = 0x04;	/* DctSelHiRngEn = 0 */
				} else if (dct1_size > dct0_size) {
					dct1_size = dct0_size;
					DctSelHi = 0x07;	/* DctSelHiRngEn = 1, DctSelHi = 1 */
				}
				dct0_size = dct1_size;
				dct0_size += DramBase;
				dct0_size += dct1_size;
				if (dct0_size >= HoleBase)	/* if DctSelBaseAddr > HoleBase */
					dct0_size += HoleSize;
				DctSelBase = dct0_size;

				if (dct1_size == 0)
					dct0_size = 0;
				dct0_size -= dct1_size;		/* DctSelBaseOffset = DctSelBaseAddr - Interleaved region */
				set_nb32(p_dct_stat->dev_dct, 0x114, dct0_size);

				if (dct1_size == 0)
					dct1_size = DctSelBase;
				val = get_nb32(p_dct_stat->dev_dct, 0x110);
				val &= 0x7F8;
				val |= dct1_size;
				val |= DctSelHi;
				val |= (DctSelIntLvAddr << 6) & 0xFF;
				set_nb32(p_dct_stat->dev_dct, 0x110, val);
				print_tx("InterleaveChannels: F2x110 DRAM Controller Select Low Register = ", val);

				if (HoleValid) {
					tmp = DramBase;
					val = DctSelBase;
					if (val < HoleBase) {	/* DctSelBaseAddr < DramHoleBase */
						val -= DramBase;
						val >>= 1;
						tmp += val;
					}
					tmp += HoleSize;
					val = get_nb32(p_dct_stat->dev_map, 0xF0);	/* DramHoleOffset */
					val &= 0xFFFF007F;
					val |= (tmp & ~0xFFFF007F);
					set_nb32(p_dct_stat->dev_map, 0xF0, val);
					print_tx("InterleaveChannels: F1xF0 DRAM Hole Address Register = ", val);

				}
			}
			print_tx("InterleaveChannels_D: Node ", Node);
			print_tx("InterleaveChannels_D: status ", p_dct_stat->status);
			print_tx("InterleaveChannels_D: err_status ", p_dct_stat->err_status);
			print_tx("InterleaveChannels_D: err_code ", p_dct_stat->err_code);
			Node++;
		}
	}
	print_t("InterleaveChannels_D: Done\n");
}
