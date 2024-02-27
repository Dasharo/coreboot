/* SPDX-License-Identifier: GPL-2.0-only */

#include <drivers/option/cfr.h>
#include <inttypes.h>
#include <types.h>

static const struct sm_object nvme_enable = SM_DECLARE_BOOL({
	.opt_name	= "nvme_enable",
	.ui_name	= "NVMe Slot",
	.ui_helptext	= "Enable NVMe SSD storage.",
	.default_value	= true,
});

static const __cfr_form struct sm_obj_form ec_options = {
	.ui_name	= "Mainboard",
	.obj_list	= (const struct sm_object *[]) {
		&nvme_enable,
		NULL
	},
};
