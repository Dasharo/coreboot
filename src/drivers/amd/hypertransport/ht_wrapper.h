/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef AMD_HT_WRAPPER_H
#define AMD_HT_WRAPPER_H

#include <northbridge/amd/amdfam10/amdfam10.h>
#include <stdint.h>

void amd_ht_fixup(struct sys_info *sysinfo);
u32 get_nodes(void);
void amd_ht_init(struct sys_info *sysinfo);
bool amd_cpu_find_capability(u8 node, u8 cap_count, u8 *offset);
u32 amd_check_link_type(u8 node, u8 regoff);

#endif
