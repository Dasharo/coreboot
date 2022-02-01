/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef AMD_FAM10H_RAMINIT_H
#define AMD_FAM10H_RAMINIT_H

//DDR2 REG and unbuffered : Socket F 1027 and AM3
/* every channel have 4 DDR2 DIMM for socket F
 *		       2 for socket M2/M3
 *		       1 for socket s1g1
 */
#define DIMM_SOCKETS 4

#include <device/pci_type.h>
#include <types.h>

#include "amdfam10.h"

struct sys_info;
struct mem_controller;
struct DCTStatStruc;
struct MCTStatStruc;

void fill_mem_ctrl(u32 controllers, struct mem_controller *ctrl_a, const u8 *spd_addr);
void set_sysinfo_in_ram(u32 val);

void activate_spd_rom(const struct mem_controller *ctrl);

int mct_read_spd(u32 smaddr, u32 reg);
int spd_read_byte(unsigned int device, unsigned int address);
void mct_smb_hub_init(u32 node);
void raminit_amdmct(struct sys_info *sysinfo);
void amdmct_cbmem_store_info(struct sys_info *sysinfo);

u16 mct_MaxLoadFreq(u8 count, u8 highest_rank_count, u8 registered, u8 voltage, u16 freq);
void set_nb32_dct(u32 dev, u8 dct, u32 reg, u32 val);
u32 get_nb32_dct(u32 dev, u8 dct, u32 reg);
u32 get_nb32_index_wait_dct(u32 dev, u8 dct, u32 index_reg, u32 index);
void set_nb32_index_wait_dct(u32 dev, u8 dct, u32 index_reg, u32 index, u32 data);
void fam15h_switch_dct(u32 dev, u8 dct);
u32 get_nb32_dct_nb_pstate(u32 dev, u8 dct, u8 nb_pstate, u32 reg);
void set_nb32_dct_nb_pstate(u32 dev, u8 dct, u8 nb_pstate, u32 reg, u32 val);

#endif
