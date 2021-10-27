/*/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef AMD_FAM10H_RAMINIT_H
#define AMD_FAM10H_RAMINIT_H

//DDR2 REG and unbuffered : Socket F 1027 and AM3
/* every channel have 4 DDR2 DIMM for socket F
 *		       2 for socket M2/M3
 *		       1 for socket s1g1
 */
#define DIMM_SOCKETS 4

struct mem_controller {
	u32 node_id;
	pci_devfn_t f0, f1, f2, f3, f4, f5;
	/* channel0 is DCT0 --- channelA
	 * channel1 is DCT1 --- channelB
	 * can be ganged, a single dual-channel DCT ---> 128 bit
	 *	 or unganged a two single-channel DCTs ---> 64bit
	 * When the DCTs are ganged, the writes to DCT1 set of registers
	 * (F2x1XX) are ignored and reads return all 0's
	 * The exception is the DCT phy registers, F2x[1,0]98, F2x[1,0]9C,
	 * and all the associated indexed registers, are still
	 * independently accessiable
	 */
	/* FIXME: I will only support ganged mode for easy support */
	u8 spd_switch_addr;
	u8 spd_addr[DIMM_SOCKETS*2];
};

void fill_mem_ctrl(u32 controllers, struct mem_controller *ctrl_a, const u8 *spd_addr);
void set_sysinfo_in_ram(u32 val);

#endif
