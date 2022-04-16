/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_ITE_IT8625E_CHIP_H
#define SUPERIO_ITE_IT8625E_CHIP_H

#include <superio/ite/common/env_ctrl_chip.h>
#include "it8625e.h"

enum it8625e_thermal_source {
	PIN1 = 0,
	PIN2 = 1,
	PIN3 = 2,
	PECI1 = 4,
	PECI2 = 5,
	PECI3 = 6,
	PECI4 = 7,
	PECI5 = 8,
};

struct it8625e_thermal_config {	
	enum it8625e_thermal_source source;
	/* Offset is used for diode sensors and PECI */
	u8 offset;
	/* Limits */
	u8 min;
	u8 max;
};

struct superio_ite_it8625e_config {
	struct ite_ec_config ec;
	struct it8625e_thermal_config thermal[IT8625E_EC_TMP_REG_CNT];
};

/* devicetree helpers */

#define TEMP1 thermal[0]
#define TEMP2 thermal[1]
#define TEMP3 thermal[2]
#define TEMP4 thermal[3]
#define TEMP5 thermal[4]
#define TEMP6 thermal[5]

#endif /* SUPERIO_ITE_IT8625E_CHIP_H */
