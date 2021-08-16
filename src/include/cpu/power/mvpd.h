/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_MVPD_H
#define CPU_PPC64_MVPD_H

struct region_device;

void mvpd_device_init(void);

void mvpd_device_unmount(void);

const struct region_device *mvpd_device_ro(void);

#endif /* CPU_PPC64_MVPD_H */
