/* SPDX-License-Identifier: GPL-2.0-only */

#include <northbridge/amd/amdfam10/amdfam10.h>

typedef void (*process_ap_t) (u32 apicid, void *gp);

void for_each_ap(u32 bsp_apicid, u32 core_range, s8 node,
		 process_ap_t process_ap, void *gp);

u32 initialize_cores(struct sys_info *sysinfo);
void setup_bsp(struct sys_info *sysinfo, u8 power_on_reset);
void early_cpu_finalize(struct sys_info *sysinfo, u32 bsp_apicid);
void wait_all_other_cores_stopped(u32 bsp_apicid);

int init_processor_name(void);
