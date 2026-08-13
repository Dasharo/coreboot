/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot/coreboot_tables.h>
#include <drivers/option/cfr_frontend.h>
#include <intelblocks/cfr.h>
#include <soc/cfr.h>

static struct sm_obj_form cpu_features = {
	.ui_name = "CPU Features",
	.obj_list = (const struct sm_object *[]) {
		&igd_enabled,
		&vtd,
		&vmx,
		NULL
	},
};

static struct sm_obj_form pcie_features = {
	.ui_name = "PCI Express",
	.obj_list = (const struct sm_object *[]) {
		&pciexp_speed,
		&pciexp_aspm,
		&pciexp_aspm_cpu,
		&pciexp_clk_pm,
		&pciexp_l1ss,
		NULL
	},
};

static struct sm_obj_form cpu_power = {
	.ui_name = "CPU Power and Performance",
	.obj_list = (const struct sm_object *[]) {
#if !CONFIG(SOC_INTEL_DISABLE_POWER_LIMITS)
		&pl1_override,
		&pl1_time,
		&pl2_override,
		&pl4_override,
#endif
		&pcore_turbo_ratio_group0,
		&pcore_turbo_ratio_limit0,
		&pcore_turbo_ratio_group1,
		&pcore_turbo_ratio_limit1,
		&pcore_turbo_ratio_group2,
		&pcore_turbo_ratio_limit2,
		&pcore_turbo_ratio_group3,
		&pcore_turbo_ratio_limit3,
		&pcore_turbo_ratio_group4,
		&pcore_turbo_ratio_limit4,
		&pcore_turbo_ratio_group5,
		&pcore_turbo_ratio_limit5,
		&pcore_turbo_ratio_group6,
		&pcore_turbo_ratio_limit6,
		&pcore_turbo_ratio_group7,
		&pcore_turbo_ratio_limit7,
		&ecore_turbo_ratio_group0,
		&ecore_turbo_ratio_limit0,
		&ecore_turbo_ratio_group1,
		&ecore_turbo_ratio_limit1,
		&ecore_turbo_ratio_group2,
		&ecore_turbo_ratio_limit2,
		&ecore_turbo_ratio_group3,
		&ecore_turbo_ratio_limit3,
		&ecore_turbo_ratio_group4,
		&ecore_turbo_ratio_limit4,
		&ecore_turbo_ratio_group5,
		&ecore_turbo_ratio_limit5,
		&ecore_turbo_ratio_group6,
		&ecore_turbo_ratio_limit6,
		&ecore_turbo_ratio_group7,
		&ecore_turbo_ratio_limit7,
		NULL
	},
};

static const struct sm_object spd_mem_profile = SM_DECLARE_ENUM({
	.opt_name	= "spd_mem_profile",
	.ui_name	= "Memory SPD Profile",
	.ui_helptext	= "This option selects memory profile applied to RAM modules.\n",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{"JEDEC (safe non-overclocked default)",	0	},
				{"XMP#1 (predefined extreme memory profile)",	2	},
				{"XMP#2 (predefined extreme memory profile)",	3	},
#if CONFIG(BOARD_MSI_Z790_P_PRO_WIFI_DDR5)
				{"XMP#3 (predefined extreme memory profile)",	4	},
#endif
				SM_ENUM_VALUE_END		},
}, WITH_DEP_VALUES(&oc_support, 1));

static struct sm_obj_form cpu_oc = {
	.ui_name = "CPU Overclocking (EXPERIMENTAL)\n"
		   "Changing options below may lead to system instability or CPU damage!\n"
		   "Use at your own responsibility",
	.obj_list = (const struct sm_object *[]) {
		&oc_support,
		&oc_lock,
		&spd_mem_profile,
		&core_volt_mode,
		&core_volt_ovr,
		&core_volt_adapt,
		&core_volt_off,
		&core_volt_off_sign,
		&atom_core_volt_mode,
		&atom_core_volt_ovr,
		&atom_core_volt_adapt,
		&atom_core_volt_off,
		&atom_core_volt_off_sign,
		&gt_volt_mode,
		&gt_volt_ovr,
		&gt_volt_adapt,
		&gt_volt_off,
		&gt_volt_off_sign,
		&ring_downbin,
		&undervolt_protection,
		&ia_cep,
		&gt_cep,
		&ia_vr_config,
		&ia_ac_ll,
		&ia_dc_ll,
		&ia_vr_vlimit,
		&gt_vr_config,
		&gt_ac_ll,
		&gt_dc_ll,
		&gt_vr_vlimit,
		NULL
	},
};

#if CONFIG(BOARD_MSI_Z790_P_PRO_WIFI_DDR4)
static const struct sm_object ram_ov_config = SM_DECLARE_ENUM({
	.opt_name	= "ram_ov_config",
	.ui_name	= "RAM Overvolting",
	.ui_helptext	= "Enable or disable the RAM Overvolting.\n\n"
			  "By default RAM voltage will be set according to selected Memory SPD Profile",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Use voltage from Memory SPD profile",	0	},
				{ "Manual			",		1	},
				SM_ENUM_VALUE_END					},
});

static const struct sm_object dram_volt = SM_DECLARE_NUMBER({
	.opt_name	= "dram_voltage",
	.ui_name	= "DRAM Voltage",
	.ui_helptext	= "Configure DRAM Voltage. Unit is 1mV, 10mV step.",
	.default_value	= 1200,
	.min		= 850,
	.max		= 2200,
	.step		= 10,
}, WITH_DEP_VALUES(&ram_ov_config, 1));

static const struct sm_object dram_vtt = SM_DECLARE_NUMBER({
	.opt_name	= "dram_vtt",
	.ui_name	= "DRAM VTT Voltage",
	.ui_helptext	= "Configure DRAM VTT Voltage. Unit is 1mV, 10mV step.",
	.default_value	= 600,
	.min		= 120,
	.max		= 1100,
	.step		= 10,
}, WITH_DEP_VALUES(&ram_ov_config, 1));

static const struct sm_object dram_vpp = SM_DECLARE_NUMBER({
	.opt_name	= "dram_vpp",
	.ui_name	= "DRAM VPP Voltage",
	.ui_helptext	= "Configure DRAM VPP Voltage. Unit is 1mV, 10mV step.",
	.default_value	= 2500,
	.min		= 1240,
	.max		= 3300,
	.step		= 10,
}, WITH_DEP_VALUES(&ram_ov_config, 1));

static struct sm_obj_form ram_ov = {
	.ui_name = "RAM Overvolting (EXPERIMENTAL)\n"
		   "Changing options below may lead to system instability or CPU/RAM damage!\n"
		   "Use at your own responsibility",
	.obj_list = (const struct sm_object *[]) {
		&ram_ov_config,
		&dram_volt,
		&dram_vtt,
		&dram_vpp,
		NULL
	},
};
#endif

static struct sm_obj_form *sm_root[] = {
	&cpu_features,
	&pcie_features,
	&cpu_power,
	&cpu_oc,
#if CONFIG(BOARD_MSI_Z790_P_PRO_WIFI_DDR4)
	&ram_ov,
#endif
	NULL
};

void mb_cfr_setup_menu(struct lb_cfr *cfr_root)
{
	cfr_write_setup_menu(cfr_root, sm_root);
}
