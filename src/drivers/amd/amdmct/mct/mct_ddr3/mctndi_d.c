/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <console/console.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

void interleave_nodes_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a)
{
	/* Applies node memory interleaving if enabled and if all criteria are met. */
	u8 node;
	u32 base;
	u32 mem_size, mem_size_0 = 0;
	u32 dct_0_mem_size = 0, dct_sel_base, dct_sel_base_offset = 0;
	u8 nodes;
	u8 nodes_wmem;
	u8 do_intlv;
	u8 _nd_int_cap;
	u8 _sw_hole;
	u32 hw_hole_sz;
	u32 dram_hole_addr_reg;
	u32 hole_base;
	u32 dev0;
	u32 reg0;
	u32 val;
	u8 i;
	struct DCTStatStruc *p_dct_stat;

	do_intlv = mct_get_nv_bits(NV_NODE_INTLV);

	_nd_int_cap = 0;
	hw_hole_sz = 0;	/*For HW remapping, NOT node hoisting. */

	p_dct_stat = p_dct_stat_a + 0;
	dev0 = p_dct_stat->dev_host;
	nodes = ((get_nb32(dev0, 0x60) >> 4) & 0x7) + 1;

	dev0 = p_dct_stat->dev_map;
	reg0 = 0x40;

	nodes_wmem = 0;
	node = 0;

	while (do_intlv && (node < nodes)) {
		p_dct_stat = p_dct_stat_a + node;
		if (p_mct_stat->g_status & (1 << GSB_SP_INTLV_REMAP_HOLE)) {
			p_mct_stat->g_status |= 1 << GSB_HW_HOLE;
			_sw_hole = 0;
		} else if (p_dct_stat->status & (1 << SB_SW_NODE_HOLE)) {
			_sw_hole = 1;
		} else {
			_sw_hole = 0;
		}

		if (!_sw_hole) {
			base = get_nb32(dev0, reg0);
			if (base & 1) {
				nodes_wmem++;
				base &= 0xFFFF0000;	/* base[39:8] */

				if (p_dct_stat->status & (1 << SB_HW_HOLE)) {

					/* to get true amount of dram,
					 * subtract out memory hole if HW dram remapping */
					dram_hole_addr_reg = get_nb32(p_dct_stat->dev_map, 0xF0);
					hw_hole_sz = dram_hole_addr_reg >> 16;
					hw_hole_sz = (((~hw_hole_sz) + 1) & 0xFF);
					hw_hole_sz <<= 24-8;
				}
				/* check to see if the amount of memory on each channel
				 * are the same on all nodes */

				dct_sel_base = get_nb32(p_dct_stat->dev_dct, 0x114);
				if (dct_sel_base) {
					dct_sel_base <<= 8;
					if (p_dct_stat->status & (1 << SB_HW_HOLE)) {
						if (dct_sel_base >= 0x1000000) {
							dct_sel_base -= hw_hole_sz;
						}
					}
					dct_sel_base_offset -= base;
					if (node == 0) {
						dct_0_mem_size = dct_sel_base;
					} else if (dct_sel_base != dct_0_mem_size) {
						break;
					}
				}

				mem_size = get_nb32(dev0, reg0 + 4);
				mem_size &= 0xFFFF0000;
				mem_size += 0x00010000;
				mem_size -= base;
				if (p_dct_stat->status & (1 << SB_HW_HOLE)) {
					mem_size -= hw_hole_sz;
				}
				if (node == 0) {
					mem_size_0 = mem_size;
				} else if (mem_size_0 != mem_size) {
					break;
				}
			} else {
				break;
			}
		} else {
			break;
		}
	node++;
	reg0 += 8;
	}

	if (node == nodes) {
		/* if all nodes have memory and no node had SW memhole */
		if (nodes == 2 || nodes == 4 || nodes == 8)
			_nd_int_cap = 1;
	}

	if (!_nd_int_cap)
		do_intlv = 0;

	if (p_mct_stat->g_status & 1 << (GSB_SP_INTLV_REMAP_HOLE)) {
		hw_hole_sz = p_mct_stat->hole_base;
		if (hw_hole_sz == 0) {
			hw_hole_sz = mct_get_nv_bits(NV_BOTTOM_IO) & 0xFF;
			hw_hole_sz <<= 24-8;
		}
		hw_hole_sz = ((~hw_hole_sz) + 1) & 0x00FF0000;
	}

	if (do_intlv) {
		mct_mem_clr_d(p_mct_stat, p_dct_stat_a);
		/* Program Interleaving enabled on node 0 map only.*/
		mem_size_0 <<= bsf(nodes);	/* mem_size = mem_size*2 (or 4, or 8) */
		dct_0_mem_size <<= bsf(nodes);
		mem_size_0 += hw_hole_sz;
		base = ((nodes - 1) << 8) | 3;
		reg0 = 0x40;
		node = 0;
		while (node < nodes) {
			set_nb32(dev0, reg0, base);
			mem_size = mem_size_0;
			mem_size--;
			mem_size &= 0xFFFF0000;
			mem_size |= node << 8;	/* set IntlvSel[2:0] field */
			mem_size |= node;	/* set DstNode[2:0] field */
			set_nb32(dev0, reg0 + 4, mem_size_0);
			reg0 += 8;
			node++;
		}

		/*  set base/limit to F1x120/124 per node */
		node = 0;
		while (node < nodes) {
			p_dct_stat = p_dct_stat_a + node;
			p_dct_stat->node_sys_base = 0;
			mem_size = mem_size_0;
			mem_size -= hw_hole_sz;
			mem_size--;
			p_dct_stat->node_sys_limit = mem_size;
			set_nb32(p_dct_stat->dev_map, 0x120, node << 21);
			mem_size = mem_size_0;
			mem_size--;
			mem_size >>= 19;
			val = base;
			val &= 0x700;
			val <<= 13;
			val |= mem_size;
			set_nb32(p_dct_stat->dev_map, 0x124, val);

			if (p_mct_stat->g_status & (1 << GSB_HW_HOLE)) {
				hole_base = p_mct_stat->hole_base;
				if (dct_0_mem_size >= hole_base) {
					val = hw_hole_sz;
					if (node == 0) {
						val += dct_0_mem_size;
					}
				} else {
					val = hw_hole_sz + dct_0_mem_size;
				}

				val >>= 8;		/* DramHoleOffset */
				hole_base <<= 8;		/* DramHoleBase */
				val |= hole_base;
				val |= 1 << DRAM_MEM_HOIST_VALID;
				val |= 1 << DRAM_HOLE_VALID;
				set_nb32(p_dct_stat->dev_map, 0xF0, val);
			}

			set_nb32(p_dct_stat->dev_dct, 0x114, dct_0_mem_size >> 8);	/* dct_sel_base_offset */
			val = get_nb32(p_dct_stat->dev_dct, 0x110);
			val &= 0x7FF;
			val |= dct_0_mem_size >> 8;
			set_nb32(p_dct_stat->dev_dct, 0x110, val);	/* DctSelBaseAddr */
			printk(BIOS_DEBUG, "InterleaveNodes: DRAM Controller Select Low Register = %x\n", val);
			node++;
		}

		/* Copy node 0 into other nodes' CSRs */
		node = 1;
		while (node < nodes) {
			p_dct_stat = p_dct_stat_a + node;

			for (i = 0x40; i <= 0x80; i++) {
				val = get_nb32(dev0, i);
				set_nb32(p_dct_stat->dev_map, i, val);
			}

			val = get_nb32(dev0, 0xF0);
			set_nb32(p_dct_stat->dev_map, 0xF0, val);
			node++;
		}
		p_mct_stat->g_status = (1 << GSB_NODE_INTLV);
	}
	printk(BIOS_DEBUG, "interleave_nodes_d: status %x\n", p_dct_stat->status);
	printk(BIOS_DEBUG, "interleave_nodes_d: err_status %x\n", p_dct_stat->err_status);
	printk(BIOS_DEBUG, "interleave_nodes_d: err_code %x\n", p_dct_stat->err_code);
	printk(BIOS_DEBUG, "interleave_nodes_d: Done\n\n");
}
