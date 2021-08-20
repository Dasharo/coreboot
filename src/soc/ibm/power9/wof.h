/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_WOF_H
#define __SOC_IBM_POWER9_WOF_H

struct region_device;

void wof_device_init(void);

void wof_device_unmount(void);

const struct region_device *wof_device_ro(void);

#endif /* __SOC_IBM_POWER9_WOF_H */
