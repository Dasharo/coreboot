/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <device/device.h>

#if CONFIG(SOUTHBRIDGE_AMD_SUBTYPE_SP5100)
#define SB_NAME "ATI SP5100"
#else
#define SB_NAME "ATI SB700"
#endif

static void sb7xx_51xx_enable(struct device *dev)
{
}

struct chip_operations southbridge_amd_sb700_ops = {
	CHIP_NAME(SB_NAME)
	.enable_dev = sb7xx_51xx_enable,
};
