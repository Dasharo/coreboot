/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <console/console.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

void InterleaveNodes_D(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a)
{
	/* Applies Node memory interleaving if enabled and if all criteria are met. */
	u8 Node;
	u32 Base;
	u32 MemSize, MemSize0 = 0;
	u32 Dct0MemSize = 0, DctSelBase, DctSelBaseOffset = 0;
	u8 Nodes;
	u8 NodesWmem;
	u8 DoIntlv;
	u8 _NdIntCap;
	u8 _SWHole;
	u32 HWHoleSz;
	u32 DramHoleAddrReg;
	u32 HoleBase;
	u32 dev0;
	u32 reg0;
	u32 val;
	u8 i;
	struct DCTStatStruc *p_dct_stat;

	DoIntlv = mct_get_nv_bits(NV_NODE_INTLV);

	_NdIntCap = 0;
	HWHoleSz = 0;	/*For HW remapping, NOT Node hoisting. */

	p_dct_stat = p_dct_stat_a + 0;
	dev0 = p_dct_stat->dev_host;
	Nodes = ((get_nb32(dev0, 0x60) >> 4) & 0x7) + 1;

	dev0 = p_dct_stat->dev_map;
	reg0 = 0x40;

	NodesWmem = 0;
	Node = 0;

	while (DoIntlv && (Node < Nodes)) {
		p_dct_stat = p_dct_stat_a + Node;
		if (p_mct_stat->g_status & (1 << GSB_SP_INTLV_REMAP_HOLE)) {
			p_mct_stat->g_status |= 1 << GSB_HW_HOLE;
			_SWHole = 0;
		} else if (p_dct_stat->status & (1 << SB_SW_NODE_HOLE)) {
			_SWHole = 1;
		} else {
			_SWHole = 0;
		}

		if (!_SWHole) {
			Base = get_nb32(dev0, reg0);
			if (Base & 1) {
				NodesWmem++;
				Base &= 0xFFFF0000;	/* Base[39:8] */

				if (p_dct_stat->status & (1 << SB_HW_HOLE)) {

					/* to get true amount of dram,
					 * subtract out memory hole if HW dram remapping */
					DramHoleAddrReg = get_nb32(p_dct_stat->dev_map, 0xF0);
					HWHoleSz = DramHoleAddrReg >> 16;
					HWHoleSz = (((~HWHoleSz) + 1) & 0xFF);
					HWHoleSz <<= 24-8;
				}
				/* check to see if the amount of memory on each channel
				 * are the same on all nodes */

				DctSelBase = get_nb32(p_dct_stat->dev_dct, 0x114);
				if (DctSelBase) {
					DctSelBase <<= 8;
					if (p_dct_stat->status & (1 << SB_HW_HOLE)) {
						if (DctSelBase >= 0x1000000) {
							DctSelBase -= HWHoleSz;
						}
					}
					DctSelBaseOffset -= Base;
					if (Node == 0) {
						Dct0MemSize = DctSelBase;
					} else if (DctSelBase != Dct0MemSize) {
						break;
					}
				}

				MemSize = get_nb32(dev0, reg0 + 4);
				MemSize &= 0xFFFF0000;
				MemSize += 0x00010000;
				MemSize -= Base;
				if (p_dct_stat->status & (1 << SB_HW_HOLE)) {
					MemSize -= HWHoleSz;
				}
				if (Node == 0) {
					MemSize0 = MemSize;
				} else if (MemSize0 != MemSize) {
					break;
				}
			} else {
				break;
			}
		} else {
			break;
		}
	Node++;
	reg0 += 8;
	}

	if (Node == Nodes) {
		/* if all nodes have memory and no Node had SW memhole */
		if (Nodes == 2 || Nodes == 4 || Nodes == 8)
			_NdIntCap = 1;
	}

	if (!_NdIntCap)
		DoIntlv = 0;

	if (p_mct_stat->g_status & 1 << (GSB_SP_INTLV_REMAP_HOLE)) {
		HWHoleSz = p_mct_stat->hole_base;
		if (HWHoleSz == 0) {
			HWHoleSz = mct_get_nv_bits(NV_BOTTOM_IO) & 0xFF;
			HWHoleSz <<= 24-8;
		}
		HWHoleSz = ((~HWHoleSz) + 1) & 0x00FF0000;
	}

	if (DoIntlv) {
		MCTMemClr_D(p_mct_stat, p_dct_stat_a);
		/* Program Interleaving enabled on Node 0 map only.*/
		MemSize0 <<= bsf(Nodes);	/* MemSize = MemSize*2 (or 4, or 8) */
		Dct0MemSize <<= bsf(Nodes);
		MemSize0 += HWHoleSz;
		Base = ((Nodes - 1) << 8) | 3;
		reg0 = 0x40;
		Node = 0;
		while (Node < Nodes) {
			set_nb32(dev0, reg0, Base);
			MemSize = MemSize0;
			MemSize--;
			MemSize &= 0xFFFF0000;
			MemSize |= Node << 8;	/* set IntlvSel[2:0] field */
			MemSize |= Node;	/* set DstNode[2:0] field */
			set_nb32(dev0, reg0 + 4, MemSize0);
			reg0 += 8;
			Node++;
		}

		/*  set base/limit to F1x120/124 per Node */
		Node = 0;
		while (Node < Nodes) {
			p_dct_stat = p_dct_stat_a + Node;
			p_dct_stat->node_sys_base = 0;
			MemSize = MemSize0;
			MemSize -= HWHoleSz;
			MemSize--;
			p_dct_stat->node_sys_limit = MemSize;
			set_nb32(p_dct_stat->dev_map, 0x120, Node << 21);
			MemSize = MemSize0;
			MemSize--;
			MemSize >>= 19;
			val = Base;
			val &= 0x700;
			val <<= 13;
			val |= MemSize;
			set_nb32(p_dct_stat->dev_map, 0x124, val);

			if (p_mct_stat->g_status & (1 << GSB_HW_HOLE)) {
				HoleBase = p_mct_stat->hole_base;
				if (Dct0MemSize >= HoleBase) {
					val = HWHoleSz;
					if (Node == 0) {
						val += Dct0MemSize;
					}
				} else {
					val = HWHoleSz + Dct0MemSize;
				}

				val >>= 8;		/* DramHoleOffset */
				HoleBase <<= 8;		/* DramHoleBase */
				val |= HoleBase;
				val |= 1 << DRAM_MEM_HOIST_VALID;
				val |= 1 << DRAM_HOLE_VALID;
				set_nb32(p_dct_stat->dev_map, 0xF0, val);
			}

			set_nb32(p_dct_stat->dev_dct, 0x114, Dct0MemSize >> 8);	/* DctSelBaseOffset */
			val = get_nb32(p_dct_stat->dev_dct, 0x110);
			val &= 0x7FF;
			val |= Dct0MemSize >> 8;
			set_nb32(p_dct_stat->dev_dct, 0x110, val);	/* DctSelBaseAddr */
			printk(BIOS_DEBUG, "InterleaveNodes: DRAM Controller Select Low Register = %x\n", val);
			Node++;
		}

		/* Copy Node 0 into other Nodes' CSRs */
		Node = 1;
		while (Node < Nodes) {
			p_dct_stat = p_dct_stat_a + Node;

			for (i = 0x40; i <= 0x80; i++) {
				val = get_nb32(dev0, i);
				set_nb32(p_dct_stat->dev_map, i, val);
			}

			val = get_nb32(dev0, 0xF0);
			set_nb32(p_dct_stat->dev_map, 0xF0, val);
			Node++;
		}
		p_mct_stat->g_status = (1 << GSB_Node_INTLV);
	}
	printk(BIOS_DEBUG, "InterleaveNodes_D: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "InterleaveNodes_D: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "InterleaveNodes_D: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "InterleaveNodes_D: Done\n\n");
}
