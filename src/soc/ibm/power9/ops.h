/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_OPS_H
#define CPU_PPC64_OPS_H

#include <stdint.h>

static const uint32_t ATTN_OP             = 0x00000200;
static const uint32_t BLR_OP              = 0x4E800020;
static const uint32_t BR_OP               = 0x48000000;
static const uint32_t BCCTR_OP            = 0x4C000420;
static const uint32_t ORI_OP              = 0x60000000;
static const uint32_t LIS_OP              = 0x3C000000;
static const uint32_t MTSPR_OP            = 0x7C0003A6;
static const uint32_t SKIP_SPR_REST_INST  = 0x4800001C;
static const uint32_t MR_R0_TO_R10_OP     = 0x7C0A0378;
static const uint32_t MR_R0_TO_R21_OP     = 0x7C150378;
static const uint32_t MR_R0_TO_R9_OP      = 0x7C090378;
static const uint32_t MTLR_R30_OP         = 0x7FC803A6;
static const uint32_t MFLR_R30_OP         = 0x7FC802A6;

static inline uint32_t ppc_lis(uint16_t rt, uint16_t data)
{
	uint32_t inst;
	inst = LIS_OP;
	inst |= rt << (31 - 10);
	inst |= data;
	return inst;
}

static inline uint32_t ppc_ori(uint16_t rs, uint16_t ra, uint16_t data)
{
	uint32_t inst;
	inst = ORI_OP;
	inst |= rs << (31 - 10);
	inst |= ra << (31 - 15);
	inst |= data;
	return inst;
}

static inline uint32_t ppc_mtspr(uint16_t rs, uint16_t spr)
{
	uint32_t temp = ((spr & 0x03FF) << (31 - 20));

	uint32_t inst;
	inst = MTSPR_OP;
	inst |= rs << (31 - 10);
	inst |= (temp & 0x0000F800) << 5;  // Perform swizzle
	inst |= (temp & 0x001F0000) >> 5;  // Perform swizzle
	return inst;
}

static inline uint32_t ppc_bctr(void)
{
	uint32_t inst;
	inst = BCCTR_OP;
	inst |= 20 << (31 - 10); // BO
	return inst;
}

static inline uint32_t ppc_b(uint32_t target_addr)
{
	uint32_t inst;
	inst = BR_OP;
	inst |= (target_addr & 0x03FFFFFF);
	return inst;
}

#endif /* CPU_PPC64_OPS_H */
