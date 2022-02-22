/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef EC_CLEVO_IT5570_CHIP_H
#define EC_CLEVO_IT5570_CHIP_H

#include <acpi/acpi_device.h>

#define IT5570_FAN_CURVE_LEN		0x4	/* Number of fan curve points */
#define IT5570_FAN_CNT			0x3	/* Number of configurable fans */

enum ec_clevo_it5570_fan_mode {
	FAN_MODE_AUTO = 0,
	FAN_MODE_CUSTOM,
};

struct ec_clevo_it5570_fan_curve {
	u8 temperature[IT5570_FAN_CURVE_LEN];
	u8 speed[IT5570_FAN_CURVE_LEN];
};

struct ec_clevo_it5570_fan_config {
	struct ec_clevo_it5570_fan_curve curve;
};

struct ec_clevo_it5570_config {
	enum ec_clevo_it5570_fan_mode fan_mode;
	struct ec_clevo_it5570_fan_config fans[IT5570_FAN_CNT];
};

#endif /* EC_CLEVO_IT5570_CHIP_H */
