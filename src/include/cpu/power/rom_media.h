/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_ROM_MEDIA_H
#define CPU_PPC64_ROM_MEDIA_H

struct mmap_helper_region_device;

void mount_part_from_pnor(const char *part_name,
                          struct mmap_helper_region_device *mdev);

#endif // CPU_PPC64_ROM_MEDIA_H
