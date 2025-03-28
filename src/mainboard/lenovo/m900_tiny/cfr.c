/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot/coreboot_tables.h>
#include <dasharo/cfr.h>

struct sm_object wifi_slot_enable = SM_DECLARE_BOOL({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "wifi_slot_enable",
	.ui_name	= "Enable Wi-Fi card slot",
	.ui_helptext	= "Enable or disable detection of devices in the Wi-Fi card slot",
	.default_value 	= true,
});

struct sm_object ssd_slot_enable = SM_DECLARE_BOOL({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "ssd_slot_enable",
	.ui_name	= "Enable SSD slot",
	.ui_helptext	= "Enable or disable detection of devices in the SSD slot",
	.default_value 	= true,
});

struct sm_object hdd_slot_enable = SM_DECLARE_BOOL({
	.flags		= CFR_OPTFLAG_RUNTIME,
	.opt_name	= "hdd_slot_enable",
	.ui_name	= "Enable 2.5 inch disk slot",
	.ui_helptext	= "Enable or disable detection of devices in the 2.5 inch disk slot",
	.default_value 	= true,
});

static struct sm_obj_form devices = {
	.ui_name	= "Devices",
	.obj_list	= (const struct sm_object *[]) {
		&wifi_slot_enable,
		&ssd_slot_enable,
		&hdd_slot_enable,
		NULL
	},
};

static struct sm_obj_form security = {
	.ui_name	= "Security",
	.obj_list	= (const struct sm_object *[]) {
		&smm_bwp,
		NULL
	},
};

static struct sm_obj_form chipset = {
	.ui_name	= "Chipset",
	.obj_list	= (const struct sm_object *[]) {
		&me_mode,
		&ps2_enable,
		NULL
	},
};

static struct sm_obj_form processor = {
	.ui_name	= "Processor",
	.obj_list	= (const struct sm_object *[]) {
		&hyper_threading,
		&throttle_offset,
		&throttle_temp,
		NULL
	},
};

static struct sm_obj_form bootloader = {
	.ui_name	= "Bootloader",
	.obj_list	= (const struct sm_object *[]) {
		&network_boot,
		&uefi_usb_stack,
		&uefi_usb_msc,
		NULL
	},
};

static struct sm_obj_form *sm_root[] = {
	&security,
	&processor,
	&chipset,
	&devices,
	&bootloader,
	NULL
};

void mb_cfr_setup_menu(struct lb_cfr *cfr_root)
{
	cfr_write_setup_menu(cfr_root, sm_root);
}
