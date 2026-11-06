/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/x86/smm.h>
#include <dasharo/options.h>
#include <ec/acpi/ec.h>
#include "acpi.h"
#include "driver.h"
#include "smm.h"

// Save the current EC fan curve as a CustomFanCurve in UEFI for persistence
static void dasharo_ec_fan_curve_save(void)
{
	struct custom_fan_curve curve = { .point_count = FAN_CURVE_MAX_POINTS };
	int i;

	for (i = 0; i < FAN_CURVE_MAX_POINTS; i++) {
		curve.curve.points[i].temp = ec_read(DASHARO_EC_REG_FAN_CURVE + sizeof(struct fan_point) * i
						     + offsetof(struct fan_point, temp));
		curve.curve.points[i].duty = ec_read(DASHARO_EC_REG_FAN_CURVE + sizeof(struct fan_point) * i
						     + offsetof(struct fan_point, duty));
	}

	if (!set_custom_fan_curve(&curve))
		printk(BIOS_ERR, "EC: fan curve not saved\n");
}

void dasharo_ec_smi_apmc(u8 apmc)
{
	if (apmc == DASHARO_APM_FAN_CURVE_SAVE)
		dasharo_ec_fan_curve_save();
}

/* Boards using this EC have no APM commands of their own */
int mainboard_smi_apmc(u8 apmc)
{
	dasharo_ec_smi_apmc(apmc);
	return 0;
}
