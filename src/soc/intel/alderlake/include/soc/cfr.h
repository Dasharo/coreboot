/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * CFR enums and structs which are used to control SoC settings.
 */

#ifndef _ALDERLAKE_CFR_H_
#define _ALDERLAKE_CFR_H_

#include <drivers/option/cfr_frontend.h>
#include <soc/soc_chip.h>

/* FSP hyperthreading */
static const struct sm_object hyper_threading = SM_DECLARE_ENUM({
	.opt_name	= "hyper_threading",
	.ui_name	= "Hyper-Threading",
	.ui_helptext	= "Enable or disable Hyper-Threading",
	.default_value	= CONFIG(FSP_HYPERTHREADING),
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

/* IGD Enabled */
static const struct sm_object igd_enabled = SM_DECLARE_ENUM({
	.opt_name	= "igd_enabled",
	.ui_name	= "Enable the Intel iGPU",
	.ui_helptext	= "Enable or disable the Intel iGPU",
	.default_value	= !CONFIG(SOC_INTEL_DISABLE_IGD),
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

/* IGD Aperture Size */
static const struct sm_object igd_aperture = SM_DECLARE_ENUM({
	.opt_name	= "igd_aperture_size",
	.ui_name	= "IGD Aperture Size",
	.ui_helptext	= "Select the Aperture Size",
	.default_value	= IGD_AP_SZ_256MB,
	.values		= (const struct sm_enum_value[]) {
				{ " 128 MB",		IGD_AP_SZ_128MB		},
				{ " 256 MB",		IGD_AP_SZ_256MB		},
#if CONFIG(ALWAYS_ALLOW_ABOVE_4G_ALLOCATION)
				{ " 512 MB (4G MMIO)",	IGD_AP_SZ_4G_512MB	},
				{ "1024 MB (4G MMIO)",	IGD_AP_SZ_4G_1024MB	},
				{ "2048 MB (4G MMIO)",	IGD_AP_SZ_4G_2048MB	},
#else
				{ " 512 MB",		IGD_AP_SZ_512MB		},
#endif
				SM_ENUM_VALUE_END				},
}, WITH_DEP_VALUES(&igd_enabled, 1));

/* IGD DVMT pre-allocated memory */
static const struct sm_object igd_dvmt = SM_DECLARE_ENUM({
	.opt_name	= "igd_dvmt_prealloc",
	.ui_name	= "IGD DVMT Size",
	.ui_helptext	= "Size of memory preallocated for internal graphics",
	.default_value	= IGD_SM_60MB,
	.values		= (const struct sm_enum_value[]) {
				{ " 32 MB",		IGD_SM_32MB	},
				{ " 60 MB",		IGD_SM_60MB	},
				{ " 64 MB",		IGD_SM_64MB	},
				{ " 96 MB",		IGD_SM_96MB	},
				{ "128 MB",		IGD_SM_128MB	},
				{ "160 MB",		IGD_SM_160MB	},
				SM_ENUM_VALUE_END			},
}, WITH_DEP_VALUES(&igd_enabled, 1));

/* Legacy 8254 Timer */
static const struct sm_object legacy_8254_timer = SM_DECLARE_ENUM({
	.opt_name	= "legacy_8254_timer",
	.ui_name	= "Legacy 8254 Timer",
	.ui_helptext	= "Enable the legacy 8254 timer by disabling clock gating.",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

/* S0ix Enable */
static const struct sm_object s0ix_enable = SM_DECLARE_ENUM({
	.opt_name	= "s0ix_enable",
	.ui_name	= "Modern Standby (S0ix)",
	.ui_helptext	= "Enabled: use Modern Standby / S0ix. Disabled: use ACPI S3 sleep",
	.default_value	= 1,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

/* VMX */
static const struct sm_object vmx = SM_DECLARE_ENUM({
	.opt_name	= "vmx",
	.ui_name	= "Intel (VMX) Virtualization Technology",
	.ui_helptext	= "Enable or disable Intel VMX (VT-x virtualization extension)",
	.default_value	= CONFIG(ENABLE_VMX),
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

/* VT-d */
static const struct sm_object vtd = SM_DECLARE_ENUM({
	.opt_name	= "vtd",
	.ui_name	= "VT-d",
	.ui_helptext	= "Enable or disable Intel VT-d (IOMMU)",
	.default_value	= 1,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

static const struct sm_object oc_lock = SM_DECLARE_ENUM({
	.opt_name	= "oc_lock",
	.ui_name	= "OC Lock",
	.ui_helptext	= "Lock OC settings",
	.default_value	= 1,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

static const struct sm_object oc_support = SM_DECLARE_ENUM({
	.opt_name	= "oc_support",
	.ui_name	= "OC Support",
	.ui_helptext	= "Enable Overclocking support",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

static const struct sm_object ia_vr_config = SM_DECLARE_ENUM({
	.opt_name	= "ia_vr_config_enable",
	.ui_name	= "IA VR Configuration",
	.ui_helptext	= "Configure Core/IA VR settings. Disabled means use HW default",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

static const struct sm_object ia_ac_ll = SM_DECLARE_NUMBER({
	.opt_name	= "ia_ac_ll",
	.ui_name	= "IA AC LoadLine",
	.ui_helptext	= "Configure Core/IA AC Loadline. 0 means use HW default. Unit is 1/100 mOhms, e.g 1250 means 12.50 mOhms",
	.default_value	= 0,
	.min		= 0,
	.max		= 6249,
	.step		= 0,
}, WITH_DEP_VALUES(&ia_vr_config, 1));

static const struct sm_object ia_dc_ll = SM_DECLARE_NUMBER({
	.opt_name	= "ia_dc_ll",
	.ui_name	= "IA DC LoadLine",
	.ui_helptext	= "Configure Core/IA DC Loadline. 0 means use HW default. Unit is 1/100 mOhms, e.g 1250 means 12.50 mOhms",
	.default_value	= 0,
	.min		= 0,
	.max		= 6249,
	.step		= 0,
}, WITH_DEP_VALUES(&ia_vr_config, 1));

static const struct sm_object ia_vr_vlimit = SM_DECLARE_NUMBER({
	.opt_name	= "ia_vr_vlimit",
	.ui_name	= "IA VR Voltage Limit",
	.ui_helptext	= "Configure Core/IA VR Voltage Limit. 0 means use HW default. Unit is 1mV",
	.default_value	= 0,
	.min		= 0,
	.max		= 7999,
	.step		= 0,
}, WITH_DEP_VALUES(&ia_vr_config, 1));

static const struct sm_object gt_vr_config = SM_DECLARE_ENUM({
	.opt_name	= "gt_vr_config_enable",
	.ui_name	= "GT VR Configuration",
	.ui_helptext	= "Configure GT VR settings. Disabled means use HW default",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
});

static const struct sm_object gt_ac_ll = SM_DECLARE_NUMBER({
	.opt_name	= "gt_ac_ll",
	.ui_name	= "GT AC LoadLine",
	.ui_helptext	= "Configure GT AC Loadline. 0 means use HW default. Unit is 1/100 mOhms, e.g 1250 means 12.50 mOhms",
	.default_value	= 0,
	.min		= 0,
	.max		= 6249,
	.step		= 0,
}, WITH_DEP_VALUES(&gt_vr_config, 1));

static const struct sm_object gt_dc_ll = SM_DECLARE_NUMBER({
	.opt_name	= "gt_dc_ll",
	.ui_name	= "GT DC LoadLine",
	.ui_helptext	= "Configure GT DC Loadline. 0 means use HW default. Unit is 1/100 mOhms, e.g 1250 means 12.50 mOhms",
	.default_value	= 0,
	.min		= 0,
	.max		= 6249,
	.step		= 0,
}, WITH_DEP_VALUES(&gt_vr_config, 1));

static const struct sm_object gt_vr_vlimit = SM_DECLARE_NUMBER({
	.opt_name	= "gt_vr_vlimit",
	.ui_name	= "GT VR Voltage Limit",
	.ui_helptext	= "Configure GT VR Voltage Limit. 0 means use HW default. Unit is 1mV",
	.default_value	= 0,
	.min		= 0,
	.max		= 7999,
	.step		= 0,
}, WITH_DEP_VALUES(&gt_vr_config, 1));

static const struct sm_object undervolt_protection = SM_DECLARE_ENUM({
	.opt_name	= "undervolt_prot",
	.ui_name	= "Undervolt Protection",
	.ui_helptext	= "Enable Undervolt Protection",
	.default_value	= 1,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object ia_cep = SM_DECLARE_ENUM({
	.opt_name	= "ia_cep",
	.ui_name	= "IA Current Excursion Protection (CEP)",
	.ui_helptext	= "Enable/disable IA Current Excursion Protection (CEP)",
	.default_value	= 1,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object gt_cep = SM_DECLARE_ENUM({
	.opt_name	= "gt_cep",
	.ui_name	= "GT Current Excursion Protection (CEP)",
	.ui_helptext	= "Enable/disable GT Current Excursion Protection (CEP)",
	.default_value	= 1,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object ring_downbin = SM_DECLARE_ENUM({
	.opt_name	= "ring_downbin",
	.ui_name	= "Rign Downbin",
	.ui_helptext	= "When enabled, the CPU will force the ring ratio to be lower than the core ratio.",
	.default_value	= 1,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		0	},
				{ "Enabled",		1	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object pcore_turbo_ratio_group0 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_group0",
	.ui_name	= "P-core Turbo Ratio Limit Group 0",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Group 0 defines the core range.\n\n"
			  "The turbo ratio is defined in P-core Turbo Ratio Limit Ratio 0.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_group1 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_group1",
	.ui_name	= "P-core Turbo Ratio Limit Group 1",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Group 1 defines the core range.\n\n"
			  "The turbo ratio is defined in P-core Turbo Ratio Limit Ratio 1.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_group2 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_group2",
	.ui_name	= "P-core Turbo Ratio Limit Group 2",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Group 2 defines the core range.\n\n"
			  "The turbo ratio is defined in P-core Turbo Ratio Limit Ratio 2.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_group3 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_group3",
	.ui_name	= "P-core Turbo Ratio Limit Group 3",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Group 3 defines the core range.\n\n"
			  "The turbo ratio is defined in P-core Turbo Ratio Limit Ratio 3.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_group4 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_group4",
	.ui_name	= "P-core Turbo Ratio Limit Group 4",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Group 4 defines the core range.\n\n"
			  "The turbo ratio is defined in P-core Turbo Ratio Limit Ratio 4.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_group5 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_group5",
	.ui_name	= "P-core Turbo Ratio Limit Group 5",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Group 5 defines the core range.\n\n"
			  "The turbo ratio is defined in P-core Turbo Ratio Limit Ratio 5.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_group6 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_group6",
	.ui_name	= "P-core Turbo Ratio Limit Group 6",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Group 6 defines the core range.\n\n"
			  "The turbo ratio is defined in P-core Turbo Ratio Limit Ratio 6.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_group7 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_group7",
	.ui_name	= "P-core Turbo Ratio Limit Group 7",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Group 7 defines the core range.\n\n"
			  "The turbo ratio is defined in P-core Turbo Ratio Limit Ratio 7.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_limit0 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_limit0",
	.ui_name	= "P-core Turbo Ratio Limit Ratio 0",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Ratio0 defines the turbo ratio.\n\n"
			  "The core range is defined in P-core Turbo Ratio Limit Group 0.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_limit1 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_limit1",
	.ui_name	= "P-core Turbo Ratio Limit Ratio 1",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Ratio 1 defines the turbo ratio.\n\n"
			  "The core range is defined in P-core Turbo Ratio Limit Group 1.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_limit2 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_limit2",
	.ui_name	= "P-core Turbo Ratio Limit Ratio 2",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Ratio 2 defines the turbo ratio.\n\n"
			  "The core range is defined in P-core Turbo Ratio Limit Group 2.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_limit3 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_limit3",
	.ui_name	= "P-core Turbo Ratio Limit Ratio 3",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Ratio 3 defines the turbo ratio.\n\n"
			  "The core range is defined in P-core Turbo Ratio Limit Group 3.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_limit4 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_limit4",
	.ui_name	= "P-core Turbo Ratio Limit Ratio 4",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Ratio 4 defines the turbo ratio.\n\n"
			  "The core range is defined in P-core Turbo Ratio Limit Group 4.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_limit5 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_limit5",
	.ui_name	= "P-core Turbo Ratio Limit Ratio 5",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Ratio 5 defines the turbo ratio.\n\n"
			  "The core range is defined in P-core Turbo Ratio Limit Group 5.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_limit6 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_limit6",
	.ui_name	= "P-core Turbo Ratio Limit Ratio 6",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Ratio 6 defines the turbo ratio.\n\n"
			  "The core range is defined in P-core Turbo Ratio Limit Group 6.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object pcore_turbo_ratio_limit7 = SM_DECLARE_NUMBER({
	.opt_name	= "pcore_turbo_ratio_limit7",
	.ui_name	= "P-core Turbo Ratio Limit Ratio 7",
	.ui_helptext	= "Performance-core Turbo Ratio Limit Ratio 7 defines the turbo ratio.\n\n"
			  "The core range is defined in P-core Turbo Ratio Limit Group 7.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});


static const struct sm_object ecore_turbo_ratio_group0 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_group0",
	.ui_name	= "E-core Turbo Ratio Limit Group 0",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Group 0 defines the core range.\n\n"
			  "The turbo ratio is defined in E-coreTurbo Ratio Limit Ratio 0.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_group1 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_group1",
	.ui_name	= "E-core Turbo Ratio Limit Group 1",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Group 1 defines the core range.\n\n"
			  "The turbo ratio is defined in E-coreTurbo Ratio Limit Ratio 1.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_group2 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_group2",
	.ui_name	= "E-core Turbo Ratio Limit Group 2",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Group 2 defines the core range.\n\n"
			  "The turbo ratio is defined in E-coreTurbo Ratio Limit Ratio 2.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_group3 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_group3",
	.ui_name	= "E-core Turbo Ratio Limit Group 3",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Group 3 defines the core range.\n\n"
			  "The turbo ratio is defined in E-coreTurbo Ratio Limit Ratio 3.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_group4 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_group4",
	.ui_name	= "E-core Turbo Ratio Limit Group 4",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Group 4 defines the core range.\n\n"
			  "The turbo ratio is defined in E-coreTurbo Ratio Limit Ratio 4.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_group5 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_group5",
	.ui_name	= "E-core Turbo Ratio Limit Group 5",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Group 5 defines the core range.\n\n"
			  "The turbo ratio is defined in E-coreTurbo Ratio Limit Ratio 5.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_group6 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_group6",
	.ui_name	= "E-core Turbo Ratio Limit Group 6",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Group 6 defines the core range.\n\n"
			  "The turbo ratio is defined in E-coreTurbo Ratio Limit Ratio 6.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_group7 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_group7",
	.ui_name	= "E-core Turbo Ratio Limit Group 7",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Group 7 defines the core range.\n\n"
			  "The turbo ratio is defined in E-coreTurbo Ratio Limit Ratio 7.\n\n"
			  "If value is zero, this entry is ignored.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_limit0 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_limit0",
	.ui_name	= "E-core Turbo Ratio Limit Ratio 0",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Ratio0 defines the turbo ratio.\n\n"
			  "The core range is defined in E-core Turbo Ratio Limit Group 0.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_limit1 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_limit1",
	.ui_name	= "E-core Turbo Ratio Limit Ratio 1",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Ratio 1 defines the turbo ratio.\n\n"
			  "The core range is defined in E-core Turbo Ratio Limit Group 1.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_limit2 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_limit2",
	.ui_name	= "E-core Turbo Ratio Limit Ratio 2",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Ratio 2 defines the turbo ratio.\n\n"
			  "The core range is defined in E-core Turbo Ratio Limit Group 2.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_limit3 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_limit3",
	.ui_name	= "E-core Turbo Ratio Limit Ratio 3",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Ratio 3 defines the turbo ratio.\n\n"
			  "The core range is defined in E-core Turbo Ratio Limit Group 3.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_limit4 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_limit4",
	.ui_name	= "E-core Turbo Ratio Limit Ratio 4",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Ratio 4 defines the turbo ratio.\n\n"
			  "The core range is defined in E-core Turbo Ratio Limit Group 4.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_limit5 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_limit5",
	.ui_name	= "E-core Turbo Ratio Limit Ratio 5",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Ratio 5 defines the turbo ratio.\n\n"
			  "The core range is defined in E-core Turbo Ratio Limit Group 5.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_limit6 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_limit6",
	.ui_name	= "E-core Turbo Ratio Limit Ratio 6",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Ratio 6 defines the turbo ratio.\n\n"
			  "The core range is defined in E-core Turbo Ratio Limit Group 6.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object ecore_turbo_ratio_limit7 = SM_DECLARE_NUMBER({
	.opt_name	= "ecore_turbo_ratio_limit7",
	.ui_name	= "E-core Turbo Ratio Limit Ratio 7",
	.ui_helptext	= "Efficient-core Turbo Ratio Limit Ratio 7 defines the turbo ratio.\n\n"
			  "The core range is defined in E-core Turbo Ratio Limit Group 7.",
	.default_value	= 0,
	.min		= 0,
	.max		= 255,
});

static const struct sm_object core_volt_mode = SM_DECLARE_ENUM({
	.opt_name	= "core_volt_mode",
	.ui_name	= "Core Voltage Mode",
	.ui_helptext	= "Core Voltage mode",
	.default_value	= 2,
	.values		= (const struct sm_enum_value[]) {
				{ "Adaptive",		0	},
				{ "Override",		1	},
				{ "Auto",		2	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object core_volt_ovr = SM_DECLARE_NUMBER({
	.opt_name	= "core_volt_ovr",
	.ui_name	= "Core Voltage Override",
	.ui_helptext	= "The core voltage override which is applied to the entire range of cpu core frequencies. Unit is mV",
	.default_value	= 0,
	.min		= 0,
	.max		= 2000,
}, WITH_DEP_VALUES(&core_volt_mode, 1));

static const struct sm_object core_volt_adapt = SM_DECLARE_NUMBER({
	.opt_name	= "core_volt_adapt",
	.ui_name	= "Core Adaptive Voltage",
	.ui_helptext	= "Extra Turbo voltage applied to the cpu core when the cpu is operating in turbo mode. Unit is mV",
	.default_value 	= 0,
	.min		= 0,
	.max		= 2000,
}, WITH_DEP_VALUES(&core_volt_mode, 0));

static const struct sm_object core_volt_off = SM_DECLARE_NUMBER({
	.opt_name	= "core_volt_off",
	.ui_name	= "Core Voltage Offset",
	.ui_helptext	= "Core Voltage Offset applied on top of all other voltage modes.\n\n"
			  "This offset is applied over the entire frequency range. Unit is mV",
	.default_value	= 0,
	.min		= 0,
	.max		= 1000,
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object core_volt_off_sign = SM_DECLARE_ENUM({
	.opt_name	= "core_volt_off_sign",
	.ui_name	= "Core Voltage Offset Sign",
	.ui_helptext	= "Core Voltage Offset sign.\n\n"
			  "Negative means the offset will be subtracted from voltage, otherwise added.",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Positive",		0	},
				{ "Negative",		1	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object gt_volt_mode = SM_DECLARE_ENUM({
	.opt_name	= "gt_volt_mode",
	.ui_name	= "GT Voltage Mode",
	.ui_helptext	= "GT Voltage mode",
	.default_value	= 2,
	.values		= (const struct sm_enum_value[]) {
				{ "Adaptive",		0	},
				{ "Override",		1	},
				{ "Auto",		2	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object gt_volt_ovr = SM_DECLARE_NUMBER({
	.opt_name	= "gt_volt_ovr",
	.ui_name	= "GT Voltage Override",
	.ui_helptext	= "The gt voltage override which is applied to the entire range of GT frequencies. Unit is mV",
	.default_value	= 0,
	.min		= 0,
	.max		= 2000,
}, WITH_DEP_VALUES(&gt_volt_mode, 1));

static const struct sm_object gt_volt_adapt = SM_DECLARE_NUMBER({
	.opt_name	= "gt_volt_adapt",
	.ui_name	= "GT Adaptive Voltage",
	.ui_helptext	= "Extra Turbo voltage applied to the GT when operating in turbo mode. Unit is mV",
	.default_value 	= 0,
	.min		= 0,
	.max		= 2000,
}, WITH_DEP_VALUES(&gt_volt_mode, 0));

static const struct sm_object gt_volt_off = SM_DECLARE_NUMBER({
	.opt_name	= "gt_volt_off",
	.ui_name	= "GT Voltage Offset",
	.ui_helptext	= "GT Voltage Offset applied on top of all other voltage modes.\n\n"
			  "This offset is applied over the entire frequency range. Unit is mV",
	.default_value	= 0,
	.min		= 0,
	.max		= 1000,
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object gt_volt_off_sign = SM_DECLARE_ENUM({
	.opt_name	= "gt_volt_off_sign",
	.ui_name	= "GT Voltage Offset Sign",
	.ui_helptext	= "GT Voltage Offset sign.\n\n"
			  "Negative means the offset will be subtracted from voltage, otherwise added.",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Positive",		0	},
				{ "Negative",		1	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object atom_core_volt_mode = SM_DECLARE_ENUM({
	.opt_name	= "atom_core_volt_mode",
	.ui_name	= "Atom Core Voltage Mode",
	.ui_helptext	= "Atom Core Voltage mode",
	.default_value	= 2,
	.values		= (const struct sm_enum_value[]) {
				{ "Adaptive",		0	},
				{ "Override",		1	},
				{ "Auto",		2	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object atom_core_volt_ovr = SM_DECLARE_NUMBER({
	.opt_name	= "atom_core_volt_ovr",
	.ui_name	= "Atom Core Voltage Override",
	.ui_helptext	= "The atom core voltage override which is applied to the entire range of cpu atom core frequencies. Unit is mV",
	.default_value	= 0,
	.min		= 0,
	.max		= 2000,
}, WITH_DEP_VALUES(&atom_core_volt_mode, 1));

static const struct sm_object atom_core_volt_adapt = SM_DECLARE_NUMBER({
	.opt_name	= "atom_core_volt_adapt",
	.ui_name	= "Atom Core Adaptive Voltage",
	.ui_helptext	= "Extra Turbo voltage applied to the cpu atom core when the cpu is operating in turbo mode. Unit is mV",
	.default_value 	= 0,
	.min		= 0,
	.max		= 2000,
}, WITH_DEP_VALUES(&atom_core_volt_mode, 0));

static const struct sm_object atom_core_volt_off = SM_DECLARE_NUMBER({
	.opt_name	= "atom_core_volt_off",
	.ui_name	= "Atom Core Voltage Offset",
	.ui_helptext	= "Atom Core Voltage Offset applied on top of all other voltage modes.\n\n"
			  "This offset is applied over the entire frequency range. Unit is mV",
	.default_value	= 0,
	.min		= 0,
	.max		= 1000,
}, WITH_DEP_VALUES(&oc_support, 1));

static const struct sm_object atom_core_volt_off_sign = SM_DECLARE_ENUM({
	.opt_name	= "atom_core_volt_off_sign",
	.ui_name	= "Atom Core Voltage Offset Sign",
	.ui_helptext	= "Atom Core Voltage Offset sign.\n\n"
			  "Negative means the offset will be subtracted from voltage, otherwise added.",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Positive",		0	},
				{ "Negative",		1	},
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

#endif /* _ALDERLAKE_CFR_H_ */
