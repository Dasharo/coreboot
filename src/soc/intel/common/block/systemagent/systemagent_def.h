/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_INTEL_COMMON_BLOCK_SA_DEF_H
#define SOC_INTEL_COMMON_BLOCK_SA_DEF_H

/* Device 0:0.0 PCI configuration space */

/* GMCH Graphics Control Register */
#define GGC		0x50
#define  G_GMS_OFFSET	0x8
#define  G_GMS_MASK	0xff00
#define  G_GGMS_OFFSET	0x6
#define  G_GGMS_MASK	0xc0
/* DPR register in case CONFIG_SA_ENABLE_DPR is selected by SoC */
#define DPR		0x5c
#define  DPR_EPM	(1 << 2)
#define  DPR_PRS	(1 << 1)
#define  DPR_SIZE_MASK	0xff0
/* CAPID0_A */
#define  CAPID_ECCDIS	(1 << 25)
#define  CAPID_DDPCD	(1 << 14)
#define  CAPID_PDCD	(1 << 12)
#define  CAPID_DDRSZ(x)	(((x) >> 19) & 0x3)

#define  PCIEXBAR_LENGTH_64MB	2
#define  PCIEXBAR_LENGTH_128MB	1
#define  PCIEXBAR_LENGTH_256MB	0
#define  PCIEXBAR_PCIEXBAREN	(1 << 0)

#define PAM0	0x80
#define PAM1	0x81
#define PAM2	0x82
#define PAM3	0x83
#define PAM4	0x84
#define PAM5	0x85
#define PAM6	0x86

/* Device 0:0.0 MMIO space */
#define MCH_PAIR	0x5418

/*
 * IMR register in case CONFIG(SA_ENABLE_IMR) is selected by SoC.
 *
 * IMR registers are found under MCHBAR.
 */
#define MCH_IMR0_BASE	0x6870
#define MCH_IMR0_MASK	0x6874
#define MCH_IMR_PITCH	0x20
#define MCH_NUM_IMRS	20

/*
 * System Memory Map Registers
 * - top_of_ram -> TSEG - DPR: uncacheable
 * - TESG - DPR -> BGSM: cacheable with standard MTRRs and reserved
 * - BGSM -> TOLUD: not cacheable with standard MTRRs and reserved
 * - 4GiB -> TOUUD: cacheable
 */
enum {
	SA_TOUUD_REG,
	SA_TOLUD_REG,
	SA_BGSM_REG,
	SA_TSEG_REG,
	/* Must be last. */
	MAX_MAP_ENTRIES
};

/*
 * Set Fixed MMIO range
 *   REG = Either PCI configuration space registers.
 *   IS_64_BIT = If registers/offset is 64 bit.
 *   DESCRIPTION = Name of the register/offset.
 */
struct sa_mem_map_descriptor {
	unsigned int reg;
	bool is_64_bit;
	const char *description;
};

#endif	/* SOC_INTEL_COMMON_BLOCK_SA_DEF_H */
