/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _CPU_INTEL_COMMON_H
#define _CPU_INTEL_COMMON_H

#include <types.h>

void set_vmx_and_lock(void);
void set_feature_ctrl_vmx(void);
void set_feature_ctrl_lock(void);

/*
 * Init CPPC block with MSRs for Intel Enhanced Speed Step Technology.
 * Version 2 is suggested--this function's implementation of version 3
 * may have room for improvement.
 */
struct cppc_config;
void cpu_init_cppc_config(struct cppc_config *config, u32 version);

/*
 * Returns true if CPU supports Hyper-Threading.
 */
bool intel_ht_supported(void);

/*
 * Returns true if it's not thread 0 on a hyperthreading enabled core.
 */
bool intel_ht_sibling(void);

#endif
