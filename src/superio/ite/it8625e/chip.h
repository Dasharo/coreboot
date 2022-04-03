/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_ITE_IT8625E_CHIP_H
#define SUPERIO_ITE_IT8625E_CHIP_H

#include <superio/ite/common/env_ctrl_chip.h>

/* IT8625E has 3 "physical" TMPINs and 6 "logical" TMPINs */
/* TSS registers map physical and external sources to logical TMPINs */

struct it8625e_tmpin_config {
	/* TMPIN Source Select 1 register */
	uint8_t tss1[6];

	/* TMPIN Source Select 2 register */
	uint8_t tss2[6];

	/* Enable PECI */
	bool enable_peci;
};

struct superio_ite_it8625e_config {
	struct ite_ec_config ec;
	struct it8625e_tmpin_config extra;
};

enum {
	TSS1_TMPIN1 = 0x0,
	TSS1_TMPIN2 = 0x1,
	TSS1_TMPIN3 = 0x2,
	TSS1_SB_TSI = 0x3,
	TSS1_PECI1  = 0x4,
	TSS1_PECI2  = 0x5,
	TSS1_PECI3  = 0x6,
	TSS1_PECI4  = 0x7,
	TSS1_PECI5  = 0x8,        /* TSS2=1: AMD Solar */
	TSS1_SM_LINK_PCH0  = 0x9, /* TSS2=1: AMD Evergreen */
	TSS1_SM_LINK_PCH1  = 0xa,
	TSS1_SM_LINK_MCH   = 0xb,
	TSS1_SM_LINK_DIMM0 = 0xc, /* TSS2=1: AMD CPU DTS */
	TSS1_SM_LINK_DIMM1 = 0xd, /* TSS2=1: MxM GPU */
	TSS1_SM_LINK_DIMM2 = 0xe,
	TSS1_SM_LINK_DIMM3 = 0xf,
};

#endif /* SUPERIO_ITE_IT8625E_CHIP_H */
