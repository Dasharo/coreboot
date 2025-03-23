/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef DASHARO_CFR_H
#define DASHARO_CFR_H

#include <drivers/option/cfr_frontend.h>
#include <option.h>
#include "options.h"

struct sm_object lock_bios = SM_DECLARE_BOOL({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "lock_bios",
	.ui_name	= "Lock the BIOS boot medium",
	.ui_helptext	= "This option locks down the BIOS flash",
	.default_value	= true,
});

static void smm_bwp_callback(const struct sm_object *obj, struct sm_object *new)
{
	if (!CONFIG(BOOTMEDIA_SMM_BWP))
		new->sm_bool.flags |= CFR_OPTFLAG_SUPPRESS;
}

struct sm_object smm_bwp = SM_DECLARE_BOOL({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "smm_bwp",
	.ui_name	= "SMM BIOS Write Protection",
	.ui_helptext	= "This option prevents flash writes outside of SMM",
	.default_value	= false,
}, WITH_CALLBACK(&smm_bwp_callback));

struct sm_object watchdog_enable = SM_DECLARE_BOOL({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "watchdog_enable",
	.ui_name	= "Enable watchdog timer",
	.ui_helptext	= "The watchdog will reset the platform once the timer expires",
	.default_value 	= CONFIG(SOC_INTEL_COMMON_OC_WDT_ENABLE),
});

struct sm_object watchdog_timeout = SM_DECLARE_NUMBER({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "watchdog_timeout",
	.ui_name	= "Watchdog timeout",
	.ui_helptext	= "After this timeout expires, the platform will be re-set",
	.default_value	= CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT_SECONDS,
}, WITH_DEP(&watchdog_enable));

struct sm_object me_mode = SM_DECLARE_ENUM({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "me_mode",
	.ui_name	= "Intel ME mode",
	.ui_helptext	= "Operational mode of Intel Management Engine",
	.default_value	= CONFIG_INTEL_ME_DEFAULT_STATE,
	.values		= (const struct sm_enum_value[]) {
				{ "Enabled",         ME_MODE_ENABLE       },
				{ "Disabled (Soft)", ME_MODE_DISABLE_HECI },
#if CONFIG(HAVE_INTEL_ME_HAP)
				{ "Disabled (HAP)",  ME_MODE_DISABLE_HAP  },
#endif
				SM_ENUM_VALUE_END },
});

static void ps2_enable_callback(const struct sm_object *obj, struct sm_object *new)
{
	if (!CONFIG(EDK2_PS2_SUPPORT))
		new->sm_bool.flags |= CFR_OPTFLAG_SUPPRESS;
}

struct sm_object ps2_enable = SM_DECLARE_BOOL({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "ps2_enable",
	.ui_name	= "Enable PS/2 controller",
	.ui_helptext	= "Enable or disable the PS/2 controller. Enabling PS/2 slightly increases boot time.",
	.default_value	= true,
}, WITH_CALLBACK(&ps2_enable_callback));

struct sm_object hyper_threading = SM_DECLARE_BOOL({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "hyper_threading",
	.ui_name	= "Enable Hyper-Threading",
	.ui_helptext	= "Enable or disable Hyper-Threading",
	.default_value	= true,
});

struct sm_object throttle_offset = SM_DECLARE_NUMBER({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "throttle_offset",
	.ui_name	= "Thermal throttling offset",
	.ui_helptext	= "Adjust the temperature at which the processor will thermal throttle. " \
			  "Lower offset means higher temperature, and vice versa.",
	.default_value	= CONFIG_EDK2_CPU_THROTTLING_THRESHOLD_DEFAULT,
});

static void update_throttling_temp(const struct sm_object *obj, struct sm_object *new)
{
	new->sm_number.default_value = CONFIG_CPU_MAX_TEMPERATURE - get_uint_option("throttle_offset", CONFIG_EDK2_CPU_THROTTLING_THRESHOLD_DEFAULT);
}

struct sm_object throttle_temp = SM_DECLARE_NUMBER({
	.flags		= CFR_OPTFLAG_READONLY | CFR_OPTFLAG_VOLATILE,
	.opt_name	= "throttle_temp",
	.ui_name	= "Current thermal throttling temperature",
}, WITH_CALLBACK(update_throttling_temp));

#endif // DASHARO_CFG_H
