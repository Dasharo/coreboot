/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <fsp/api.h>
#include <pc80/i8254.h>
#include <security/vboot/vboot_common.h>
#include <soc/ramstage.h>

static void mainboard_final(void *chip_info)
{
	if (CONFIG(VBOOT))
		vboot_clear_recovery_request();

	beep(1500, 200);
}

void mainboard_silicon_init_params(FSP_S_CONFIG *silconfig)
{
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
