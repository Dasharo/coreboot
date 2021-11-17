/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef EC_CLEVO_IT5570_CHIP_H
#define EC_CLEVO_IT5570_CHIP_H

#include <acpi/acpi_device.h>

#define IT5570_FAN_CURVE_LEN	0x4	/* Number of fan curve points */

enum ec_clevo_it5570_fan_mode {
	FAN_MODE_AUTO = 0,
	FAN_MODE_CUSTOM,
};

struct ec_clevo_it5570_curve_point {
	u8 temperature;
	u8 speed;
};

struct ec_clevo_it5570_fan_config {
	enum ec_clevo_it5570_fan_mode mode;
	struct ec_clevo_it5570_curve_point curve[IT5570_FAN_CURVE_LEN];
};

struct ec_clevo_it5570_config {
	struct ec_clevo_it5570_fan_config fans;
};

#endif /* EC_CLEVO_IT5570_CHIP_H */
