/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/x86/smm.h>
#include <dasharo/options.h>
#include "commands.h"
#include "driver.h"
#include "smm.h"

void dasharo_ec_smi_apmc(u8 apmc)
{
	if (apmc == DASHARO_APM_FAN_CURVE_SAVE)
		dasharo_ec_save_fan_curves();
}

/* Boards using this EC have no APM commands of their own */
int mainboard_smi_apmc(u8 apmc)
{
	dasharo_ec_smi_apmc(apmc);
	return 0;
}
