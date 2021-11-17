/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef EC_CLEVO_IT5570_CHIP_H
#define EC_CLEVO_IT5570_CHIP_H

#include <acpi/acpi_device.h>

struct ec_clevo_it5570_config {
	/* Custom fan curve enable */
	bool has_custom_fan_curve;

	/* Fan curve - temperatures in celsius and corresponding speeds (percentage) */
	u8 temps[4];
	u8 speeds[4];
};

#endif /* EC_CLEVO_IT5570_CHIP_H */
