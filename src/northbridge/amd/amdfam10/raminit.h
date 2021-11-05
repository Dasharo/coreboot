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

int mctRead_SPD(u32 smaddr, u32 reg);
int spd_read_byte(unsigned int device, unsigned int address);
void mctSMBhub_Init(u32 node);
void raminit_amdmct(struct sys_info *sysinfo);
void amdmct_cbmem_store_info(struct sys_info *sysinfo);

uint16_t mct_MaxLoadFreq(uint8_t count, uint8_t highest_rank_count, uint8_t registered, uint8_t voltage, uint16_t freq);
void Set_NB32_DCT(uint32_t dev, uint8_t dct, uint32_t reg, uint32_t val);
uint32_t Get_NB32_DCT(uint32_t dev, uint8_t dct, uint32_t reg);
uint32_t Get_NB32_index_wait_DCT(uint32_t dev, uint8_t dct, uint32_t index_reg, uint32_t index);
void Set_NB32_index_wait_DCT(uint32_t dev, uint8_t dct, uint32_t index_reg, uint32_t index, uint32_t data);
void fam15h_switch_dct(uint32_t dev, uint8_t dct);
uint32_t Get_NB32_DCT_NBPstate(uint32_t dev, uint8_t dct, uint8_t nb_pstate, uint32_t reg);
void Set_NB32_DCT_NBPstate(uint32_t dev, uint8_t dct, uint8_t nb_pstate, uint32_t reg, uint32_t val);

#endif
