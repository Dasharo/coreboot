/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_POWERBUS_H
#define CPU_PPC64_POWERBUS_H

#include <stdint.h>

enum FABRIC_CORE_FLOOR_RATIO
{
	FABRIC_CORE_FLOOR_RATIO_RATIO_8_8 = 0x0,
	FABRIC_CORE_FLOOR_RATIO_RATIO_7_8 = 0x1,
	FABRIC_CORE_FLOOR_RATIO_RATIO_6_8 = 0x2,
	FABRIC_CORE_FLOOR_RATIO_RATIO_5_8 = 0x3,
	FABRIC_CORE_FLOOR_RATIO_RATIO_4_8 = 0x4,
	FABRIC_CORE_FLOOR_RATIO_RATIO_2_8 = 0x5,
};

enum FABRIC_CORE_CEILING_RATIO
{
	FABRIC_CORE_CEILING_RATIO_RATIO_8_8 = 0x0,
	FABRIC_CORE_CEILING_RATIO_RATIO_7_8 = 0x1,
	FABRIC_CORE_CEILING_RATIO_RATIO_6_8 = 0x2,
	FABRIC_CORE_CEILING_RATIO_RATIO_5_8 = 0x3,
	FABRIC_CORE_CEILING_RATIO_RATIO_4_8 = 0x4,
	FABRIC_CORE_CEILING_RATIO_RATIO_2_8 = 0x5,
};

#define NUM_EPSILON_READ_TIERS 3
#define NUM_EPSILON_WRITE_TIERS 2

/* Description of PowerBus configuration */
struct powerbus_cfg
{
	/* Data computed from #V of LRP0 in MVPD, is MHz */
	uint32_t freq_core_floor;
	uint32_t freq_core_ceiling;
	uint32_t fabric_freq;

	/* Derived from data above */
	enum FABRIC_CORE_FLOOR_RATIO core_floor_ratio;
	enum FABRIC_CORE_CEILING_RATIO core_ceiling_ratio;

	/* Derived from all data above */
	/* ATTR_PROC_EPS_READ_CYCLES_T* */
	uint32_t eps_r[NUM_EPSILON_READ_TIERS];
	/* ATTR_PROC_EPS_WRITE_CYCLES_T* */
	uint32_t eps_w[NUM_EPSILON_WRITE_TIERS];
};

const struct powerbus_cfg *powerbus_cfg(uint8_t chip);

#endif // CPU_PPC64_POWERBUS_H
