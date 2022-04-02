/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_ITE_IT8625E_CHIP_H
#define SUPERIO_ITE_IT8625E_CHIP_H

#include <superio/ite/common/env_ctrl_chip.h>

struct it8625e_tmpin_config {
	uint8_t tss1[6];
};

struct superio_ite_it8625e_config {
	struct ite_ec_config ec;
	struct it8625e_tmpin_config extra;
};

#endif /* SUPERIO_ITE_IT8625E_CHIP_H */
