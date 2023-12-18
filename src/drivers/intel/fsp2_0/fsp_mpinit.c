/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <device/device.h>
#include <fsp/api.h>

/*
 * As per FSP integration guide:
 * If bootloader needs to take control of APs back, a full AP re-initialization is
 * required after FSP-S is completed and control has been transferred back to bootloader
 */
void do_mpinit_after_fsp(void)
{
	struct device *dev = dev_find_path(NULL, DEVICE_PATH_CPU_CLUSTER);
	assert(dev != NULL);

	mp_cpu_bus_init(dev);
}
