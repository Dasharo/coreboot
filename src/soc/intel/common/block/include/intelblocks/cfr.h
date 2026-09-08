/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * CFR enums and structs which are used to control various Intel common block settings.
 */

#ifndef SOC_INTEL_CMN_CFR_H
#define SOC_INTEL_CMN_CFR_H

#include <cpu/x86/msr.h>
#include <drivers/option/cfr_frontend.h>
#include <intelblocks/msr.h>
#include <intelblocks/pcie_rp.h>
#include <intelblocks/pmclib.h>
#include <intelblocks/power_limit.h>

/* Intel ME State */
static const struct sm_object me_state = SM_DECLARE_ENUM({
	.opt_name	= "me_state",
	.ui_name	= "Intel Management Engine",
	.ui_helptext	= "Enable or disable the Intel Management Engine",
	.default_value	= CONFIG(CSE_DEFAULT_CFR_OPTION_STATE_DISABLED),
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		1		},
				{ "Enabled",		0		},
				SM_ENUM_VALUE_END			},
});

/* Intel ME State Counter */
static const struct sm_object me_state_counter = SM_DECLARE_NUMBER({
	.opt_name	= "me_state_counter",
	.ui_name	= "ME State Counter",
	.flags		= CFR_OPTFLAG_SUPPRESS,
	.default_value	= 0,
});

/*
 * Power state after power loss
 * Use this option or the one below, but not both
 */
static const struct sm_object power_on_after_fail = SM_DECLARE_ENUM({
	.opt_name	= "power_on_after_fail",
	.ui_name	= "Restore AC power after loss",
	.ui_helptext	= "Specify what to do when power is re-applied after a power loss.",
	.default_value	= CONFIG_MAINBOARD_POWER_FAILURE_STATE,
	.values		= (const struct sm_enum_value[]) {
				{ "Power off (S5)", MAINBOARD_POWER_STATE_OFF		},
				{ "Power on  (S0)", MAINBOARD_POWER_STATE_ON		},
				{ "Previous state", MAINBOARD_POWER_STATE_PREVIOUS	},
				SM_ENUM_VALUE_END					},
});

/*
 * Automatic power-on toggle
 * Use this option or the one above, but not both
 */
static const struct sm_object power_on_after_fail_bool = SM_DECLARE_BOOL({
	.opt_name	= "power_on_after_fail",
	.ui_name	= "Power on after failure",
	.ui_helptext	= "Automatically turn on after a power failure",
	.default_value	= CONFIG_MAINBOARD_POWER_FAILURE_STATE,
});

/* PCIe PCH RP ASPM */
static const struct sm_object pciexp_aspm = SM_DECLARE_ENUM({
	.opt_name	= "pciexp_aspm",
	.ui_name	= "PCIe PCH RP ASPM",
	.ui_helptext	= "Controls the Active State Power Management for PCIe devices."
			  " Enabling this feature can reduce power consumption of"
			  " PCIe-connected devices during idle times.",
	.default_value	= ASPM_AUTO,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",	ASPM_DISABLE	},
				{ "L0s",	ASPM_L0S	},
				{ "L1",		ASPM_L1		},
				{ "L0sL1",	ASPM_L0S_L1	},
				{ "Auto",	ASPM_AUTO	},
				SM_ENUM_VALUE_END		},
});

/* PCIe CPU RP ASPM */
static const struct sm_object pciexp_aspm_cpu = SM_DECLARE_ENUM({
	.opt_name	= "pciexp_aspm_cpu",
	.ui_name	= "PCIe CPU RP ASPM",
	.ui_helptext	= "Controls the Active State Power Management for PCIe devices."
			  " Enabling this feature can reduce power consumption of"
			  " PCIe-connected devices during idle times.",
	.default_value	= ASPM_L0S_L1,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",	ASPM_DISABLE	},
				{ "L0s",	ASPM_L0S	},
				{ "L1",		ASPM_L1		},
				{ "L0sL1",	ASPM_L0S_L1	},
				SM_ENUM_VALUE_END		},
});

/* PCIe Clock PM */
static const struct sm_object pciexp_clk_pm = SM_DECLARE_BOOL({
	.opt_name	= "pciexp_clk_pm",
	.ui_name	= "PCIe Clock Power Management",
	.ui_helptext	= "Enables or disables power management for the PCIe clock. When"
			  " enabled, it reduces power consumption during idle states."
			  " This can help lower overall energy use but may impact"
			  " performance in power-sensitive tasks.",
	.default_value	= true,
});

/* PCIe L1 Substates */
static const struct sm_object pciexp_l1ss = SM_DECLARE_ENUM({
	.opt_name	= "pciexp_l1ss",
	.ui_name	= "PCIe L1 Substates",
	.ui_helptext	= "Controls deeper power-saving states for PCIe devices."
			  " Enabling this feature allows supported devices to achieve"
			  " lower power states at the cost of slightly increased"
			  " latency when exiting these states.",
	.default_value	= L1_SS_L1_2,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",	L1_SS_DISABLED	},
				{ "L1.1",	L1_SS_L1_1	},
				{ "L1.2",	L1_SS_L1_2	},
				SM_ENUM_VALUE_END		},
});

/* PCIe PCH Root Port Speed */
static const struct sm_object pciexp_speed = SM_DECLARE_ENUM({
	.opt_name	= "pciexp_speed",
	.ui_name	= "PCIe PCH Root Port Speed",
	.ui_helptext	= "Sets the maximum port speed for PCIe devices attached to PCH root ports.",
	.default_value	= SPEED_AUTO,
	.values		= (const struct sm_enum_value[]) {
				{ "Auto",	SPEED_AUTO	},
				{ "Gen1",	SPEED_GEN1	},
				{ "Gen2",	SPEED_GEN2	},
				{ "Gen3",	SPEED_GEN3	},
				{ "Gen4",	SPEED_GEN4	},
				SM_ENUM_VALUE_END		},
});

#if !CONFIG(SOC_INTEL_DISABLE_POWER_LIMITS)

static void update_power_limits(struct sm_object *new)
{
	msr_t msr;
	unsigned int power_unit, min_power, max_power;
	struct soc_power_limits_config *default_conf = get_power_limits_default();

	if (default_conf != NULL) {
		if (strstr(new->sm_number.opt_name, "pl1"))
			new->sm_number.default_value = default_conf->tdp_pl1_override;
		else if (strstr(new->sm_number.opt_name, "pl2"))
			new->sm_number.default_value = default_conf->tdp_pl2_override;
		else if (strstr(new->sm_number.opt_name, "pl4"))
			new->sm_number.default_value = default_conf->tdp_pl4;
	}

	/* Get units */
	msr = rdmsr(MSR_PKG_POWER_SKU_UNIT);
	power_unit = 1 << (msr.lo & 0xf);

	/* Get power defaults for this SKU */
	msr = rdmsr(MSR_PKG_POWER_SKU);
	min_power = (msr.lo >> 16) & 0x7fff;
	max_power = msr.hi & 0x7fff;

	/* If unlimited, set to the maximum value allowed by bitfield width */
	if (max_power == 0)
		max_power = 0x7fff;

	/* Convert to Watts */
	min_power /= power_unit;
	max_power /= power_unit;

	/* If unlimited, set to 1, 0 will cause programming defaults */
	if (min_power == 0)
		min_power = 1;

	new->sm_number.min = min_power;
	new->sm_number.max = max_power;
}

static const struct sm_object pl1_override = SM_DECLARE_NUMBER({
	.opt_name	= "pl1_override",
	.ui_name	= "Power Limit PL1",
	.ui_helptext	= "Power Limit 1 value in Watts.",
	.default_value	= 0,
	.min		= 0,
	.max		= 0x7fff,
	.step		= 1,
}, WITH_CALLBACK(update_power_limits));

static const struct sm_object pl2_override = SM_DECLARE_NUMBER({
	.opt_name	= "pl2_override",
	.ui_name	= "Power Limit PL2",
	.ui_helptext	= "Power Limit 2 value in Watts.",
	.default_value	= 0,
	.min		= 0,
	.max		= 0x7fff,
	.step		= 1,
}, WITH_CALLBACK(update_power_limits));

static const struct sm_object pl4_override = SM_DECLARE_NUMBER({
	.opt_name	= "tdp_pl4",
	.ui_name	= "Power Limit PL4",
	.ui_helptext	= "Power Limit 4 value in Watts.",
	.default_value	= 0,
	.min		= 0,
	.max		= 0x7fff,
	.step		= 1,
}, WITH_CALLBACK(update_power_limits));

static const struct sm_object pl1_time = SM_DECLARE_ENUM({
	.opt_name	= "pl1_time",
	.ui_name	= "Power Limit PL1 Time Window",
	.ui_helptext	= "Power Limit 1 Time Window value in seconds.\n\n"
			  "Auto means use the default HW value (28s Mobile and 56s Desktop).",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Auto",	0x00,	},
				{ "1s",		0x0a,	},
				{ "2s",		0x0b,	},
				{ "3s",		0x4b,	},
				{ "4s",		0x0c,	},
				{ "5s",		0x2c,	},
				{ "6s",		0x4c,	},
				{ "7s",		0x6c,	},
				{ "8s",		0x0d,	},
				{ "10s",	0x2d,	},
				{ "12s",	0x4d,	},
				{ "14s",	0x6d,	},
				{ "16s",	0x0e,	},
				{ "20s",	0x2e,	},
				{ "24s",	0x4e,	},
				{ "28s",	0x6e,	},
				{ "32s",	0x0f,	},
				{ "40s",	0x2f,	},
				{ "48s",	0x4f,	},
				{ "56s",	0x6f,	},
				{ "64s",	0x10,	},
				{ "80s",	0x30,	},
				{ "96s",	0x50,	},
				{ "112s",	0x70,	},
				{ "128s",	0x11,	},
				SM_ENUM_VALUE_END	},
});

#endif

#endif /* SOC_INTEL_CMN_CFR_H */
