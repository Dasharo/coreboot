/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_MEMD_H
#define CPU_PPC64_MEMD_H

struct region_device;

void memd_device_init(void);

void memd_device_unmount(void);

const struct region_device *memd_device_ro(void);

#endif /* CPU_PPC64_MEMD_H */
