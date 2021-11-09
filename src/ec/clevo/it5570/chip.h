/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef EC_CLEVO_IT5570_CHIP_H
#define EC_CLEVO_IT5570_CHIP_H

#include <acpi/acpi_device.h>

struct ec_clevo_it5570_config {
	u8 fans;

	u8 temps[4];
	u8 speeds[4];
};

#endif /* EC_CLEVO_IT5570_CHIP_H */
