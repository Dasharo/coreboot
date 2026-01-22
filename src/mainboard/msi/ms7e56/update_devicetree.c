/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <soc/amd/phoenix/chip.h>
#include <soc/soc_util.h>
#include <static.h>
#include <drivers/amd/opensil/mpio/chip.h>
#include "update_devicetree.h"

static void mainboard_update_mpio(void)
{
}

static void mainboard_update_ddi(void)
{
}

void mainboard_update_devicetree_opensil(void)
{
	mainboard_update_mpio();
	mainboard_update_ddi();
}
