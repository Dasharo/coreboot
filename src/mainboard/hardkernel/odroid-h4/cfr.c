/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot/coreboot_tables.h>
#include <drivers/option/cfr_frontend.h>
#include <intelblocks/cfr.h>
#include <soc/cfr.h>

static const struct cfr_default_override mb_cfr_overrides[] = {
	CFR_OVERRIDE_ENUM("pciexp_aspm", ASPM_DISABLE),
	CFR_OVERRIDE_ENUM("pciexp_l1ss", L1_SS_DISABLED),
	CFR_OVERRIDE_END
};

static struct sm_obj_form pcie_features = {
	.ui_name = "PCI Express",
	.obj_list = (const struct sm_object *[]) {
		&pciexp_speed,
		&pciexp_aspm,
		&pciexp_clk_pm,
		&pciexp_l1ss,
		NULL
	},
};

static struct sm_obj_form *sm_root[] = {
	&pcie_features,
	NULL
};

void mb_cfr_setup_menu(struct lb_cfr *cfr_root)
{
	cfr_register_overrides(mb_cfr_overrides);
	cfr_write_setup_menu(cfr_root, sm_root);
}