/* SPDX-License-Identifier: GPL-2.0-only */

#define __SIMPLE_DEVICE__

#include <device/pci_ops.h>
#include <device/pci.h>
#include <soc/cpu.h>
#include <soc/pci_devs.h>
#include <types.h>

struct sku_table {
  uint16_t  sa_devid;
  uint8_t   num_p_cores;
  uint8_t   num_e_cores;
};

static const struct sku_table adl_sku_table[] = {
  // AlderLake CPU Desktop SA Device IDs
  { 0x4660, 8, 8 }, // AlderLake Desktop (8+8+GT) SA DID
  { 0x4664, 8, 6 }, // AlderLake Desktop (8+6+GT) SA DID
  { 0x4668, 8, 4 }, // AlderLake Desktop (8+4+GT) SA DID
  { 0x466C, 8, 2 }, // AlderLake Desktop (8+2+GT) SA DID
  { 0x4670, 8, 0 }, // AlderLake Desktop (8+0+GT) SA DID
  { 0x4640, 6, 8 }, // AlderLake Desktop (6+8+GT) SA DID
  { 0x4644, 6, 6 }, // AlderLake Desktop (6+6+GT) SA DID
  { 0x4648, 6, 4 }, // AlderLake Desktop (6+4+GT) SA DID
  { 0x464C, 6, 2 }, // AlderLake Desktop (6+2+GT) SA DID
  { 0x4650, 6, 0 }, // AlderLake Desktop (6+0+GT) SA DID
  { 0x4630, 4, 0 }, // AlderLake Desktop (4+0+GT) SA DID
  { 0x4610, 2, 0 }, // AlderLake Desktop (2+0+GT) SA DID
  { 0x4673, 8, 6 }, // AlderLake Desktop (8+6+GT) SA DID
  { 0x4663, 8, 8 }, // AlderLake Desktop BGA  (8+8+GT) SA DID
  { 0x466B, 8, 4 }, // AlderLake Desktop BGA  (8+4+GT) SA DID
  { 0x4653, 6, 0 }, // AlderLake Desktop BGA  (6+0+GT) SA DID
  { 0x4633, 4, 0 }, // AlderLake Desktop BGA  (4+0+GT) SA DID
  { 0x4637, 8, 8 }, // AlderLake Mobile S BGA (8+8+GT) SA DID
  { 0x463B, 6, 8 }, // AlderLake Mobile S BGA (6+8+GT) SA DID
  { 0x4623, 4, 8 }, // AlderLake Mobile S BGA (4+8+GT) SA DID
  { 0x462B, 4, 4 }, // AlderLake Mobile S BGA (4+4+GT) SA DID
  { 0x4647, 6, 4 }, // AlderLake Mobile S BGA (6+4+GT) SA DID
  // AlderLake CPU Mobile SA Device IDs
  { 0x4641, 6, 8 }, // AlderLake P (6+8+GT) SA DID
  { 0x4649, 6, 4 }, // AlderLake P (6+4+GT) SA DID
  { 0x4621, 4, 8 }, // AlderLake P (4+8+GT) SA DID
  { 0x4609, 2, 4 }, // AlderLake P (2+4+GT) SA DID
  { 0x4601, 2, 8 }, // AlderLake P (2+8+GT) SA DID
  { 0x4661, 6, 8 }, // AlderLake P (6+8+2) SA DID
  { 0x4629, 4, 4 }, // AlderLake P (4+4+1) SA DID
  { 0x4619, 1, 4 }, // AlderLake P (1+4+GT) SA DID
  { 0x4659, 1, 8 }, // AlderLake P (1+8+GT) SA DID
  { 0x4645, 6, 6 }, // AlderLake P (6+6+GT) SA DID
  { 0x4602, 2, 8 }, // AlderLake M (2+8+GT) SA DID
  { 0x460A, 2, 4 }, // AlderLake M (2+4+GT) SA DID
  { 0x461A, 1, 4 }, // AlderLake M (1+4+GT) SA DID
  { 0x4622, 1, 8 }, // AlderLake M (1+8+GT) SA DID
  { 0x4617, 0, 8 }, // AlderLake N (0+8+1) SA DID
  { 0x4614, 0, 2 }, // AlderLake N Celeron (0+2+0) SA DID
  { 0x4618, 0, 4 }, // AlderLake N (0+4+0) SA DID
  { 0x461B, 0, 4 }, // AlderLake N Pentium (0+4+0) SA DID
  { 0x461C, 0, 4 }, // AlderLake N Celeron (0+4+0) SA DID
  { 0x4603, 2, 8 }, // AlderLake PS (2+8+GT) SA DID
  { 0x4643, 6, 8 }, // AlderLake PS (6+8+GT) SA DID
  { 0x4627, 4, 8 }, // AlderLake PS (4+8+GT) SA DID
  { 0x460B, 2, 4 }, // AlderLake PS (2+4+GT) SA DID
  { 0x464B, 4, 4 }, // AlderLake PS (4+4+GT) SA DID
  { 0x467B, 1, 4 }, // AlderLake PS (1+4+GT) SA DID
  // RaptorLake CPU Desktop SA Device IDs
  { 0xA700, 8, 16 }, // RaptorLake Desktop      (8+16+GT) SA DID
  { 0xA701, 8, 0  }, // RaptorLake Desktop      (8+0+GT) SA DID
  { 0xA702, 8, 16 }, // RaptorLake Desktop(BGA) (8+16+GT) SA DID
  { 0xA703, 8, 8  }, // RaptorLake Desktop      (8+8+GT) SA DID
  { 0xA704, 6, 8  }, // RaptorLake Desktop      (6+8+GT) SA DID
  { 0xA705, 6, 4  }, // RaptorLake Desktop      (6+4+GT) SA DID
  { 0xA717, 8, 0  }, // RaptorLake Desktop(BGA) (8+0+GT) SA DID
  { 0xA718, 8, 4  }, // RaptorLake Desktop(BGA) (8+4+GT) SA DID
  { 0xA719, 6, 4  }, // RaptorLake Desktop(BGA) (6+4+GT) SA DID
  { 0xA71A, 4, 4  }, // RaptorLake Desktop(BGA) (4+4+GT) SA DID
  { 0xA728, 8, 8  }, // RaptorLake Desktop(BGA) (8+8+GT) SA DID
  { 0xA729, 8, 12 }, // RaptorLake Desktop(BGA) (8+12+GT) SA DID
  { 0xA72A, 6, 8  }, // RaptorLake Desktop(BGA) (6+8+GT) SA DID
  { 0xA72B, 4, 0  }, // RaptorLake Desktop      (4+0+GT) SA DID
  { 0xA740, 8, 12 }, // RaptorLake Desktop      (8+12+GT) SA DID
  // RaptorLake CPU Mobile SA Device IDs
  { 0xA706, 6, 8 }, // RaptorLake P  (6+8+GT) SA DID
  { 0xA707, 4, 8 }, // RaptorLake P  (4+8+GT) SA DID
  { 0xA708, 2, 8 }, // RaptorLake P  (2+8+GT) SA DID
  { 0xA716, 4, 4 }, // RaptorLake P  (4+4+GT) SA DID
  { 0xA709, 6, 8 }, // RaptorLake Px (6+8+GT) SA DID
  { 0xA70A, 4, 8 }, // RaptorLake Px (4+8+GT) SA DID
  { 0xA70B, 2, 8 }, // RaptorLake Px (2+8+GT) SA DID
  { 0xA715, 6, 4 }, // RaptorLake P  (6+4+GT) SA DID
  { 0xA71B, 2, 4 }, // RaptorLake P  (2+4+GT) SA DID
  { 0xA71C, 1, 4 }, // RaptorLake P  (1+4+GT) SA DID
  { 0xA734, 6, 8 }, // RaptorLake PS (6+8+GT) SA DID
  { 0xA735, 4, 8 }, // RaptorLake PS (4+8+GT) SA DID
  { 0xA736, 2, 8 }, // RaptorLake PS (2+8+GT) SA DID
  { 0xA737, 2, 4 }, // RaptorLake PS (2+4+GT) SA DID
  { 0xA738, 6, 4 }, // RaptorLake PS (6+4+GT) SA DID
  { 0xA739, 4, 4 }, // RaptorLake PS (4+4+GT) SA DID
  { 0xA73A, 1, 4 }  // RaptorLake PS (1+4+GT) SA DID
};

void get_num_core_types(uint8_t *num_e_cores, uint8_t *num_p_cores)
{
	uint16_t sa_devid = pci_read_config16(SA_DEV_ROOT, 2);

	*num_p_cores = 0;
	*num_e_cores = 0;

	if (pci_read_config16(SA_DEV_ROOT, 0) != 0x8086)
		return;

	for (size_t i = 0; i < ARRAY_SIZE(adl_sku_table); i++) {
		if (adl_sku_table[i].sa_devid == sa_devid) {
			*num_p_cores = adl_sku_table[i].num_p_cores;
			*num_e_cores = adl_sku_table[i].num_e_cores;
			break;
		}
	}
}
