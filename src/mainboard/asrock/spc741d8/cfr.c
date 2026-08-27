/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot/coreboot_tables.h>
#include <drivers/option/cfr_frontend.h>
#include <defs_iio.h>

/*
 * IIO bifurcation.
 *
 * The Eagle Stream FSP-M exposes one ConfigIOU UPD per IOU (IioConfigIOU0 ..
 * IioConfigIOU6, indexed by socket). Only IOU0 ~ IOU4 are wired up on this
 * board, and the FSP documents exactly six accepted values for those UPDs:
 *
 *   0x00 x4x4x4x4    0x03 xxx8xxx8
 *   0x01 x4x4xxx8    0x04 xxxxxx16
 *   0x02 xxx8x4x4    0xFF AUTO (FSP default)
 *
 * Each name is eight characters, four two-character fields describing root
 * ports A, B, C and D of the IOU in order. A link wider than x4 is written in
 * the last field it spans and the fields it also covers are filled with 'xx',
 * so xxx8x4x4 is an x8 on ports A+B followed by an x4 on C and an x4 on D, and
 * xxxxxx16 is a single x16 spanning all four ports.
 *
 * defs_iio.h also defines the x2-capable encodings (0x05 ~ 0x19), but the UPD
 * documentation does not list them as accepted values, so they are not offered
 * here.
 */
static const struct sm_enum_value iio_bifurcation_values[] = {
	{ "x16",	IIO_BIFURCATE_xxxxxx16	},
	{ "x8x8",	IIO_BIFURCATE_xxx8xxx8	},
	{ "x8x4x4",	IIO_BIFURCATE_xxx8x4x4	},
	{ "x4x4x8",	IIO_BIFURCATE_x4x4xxx8	},
	{ "x4x4x4x4",	IIO_BIFURCATE_x4x4x4x4	},
	SM_ENUM_VALUE_END
};

#define IIO_BIFURCATION_HELPTEXT(slot)						\
	"Split the 16 lanes of " slot " into narrower links. The widths are "	\
	"assigned to the root ports of the IOU in the order listed. Auto lets "	\
	"the FSP pick the bifurcation based on the installed card."

static const struct sm_object iio_bifurcation_iou0 = SM_DECLARE_ENUM({
	.opt_name	= "iio_bifurcation_iou0",
	.ui_name	= "PCIE3 bifurcation",
	.ui_helptext	= IIO_BIFURCATION_HELPTEXT("slot PCIE3"),
	.default_value	= IIO_BIFURCATE_xxxxxx16,
	.values		= iio_bifurcation_values,
});

static const struct sm_object iio_bifurcation_iou1 = SM_DECLARE_ENUM({
	.opt_name	= "iio_bifurcation_iou1",
	.ui_name	= "PCIE5 bifurcation",
	.ui_helptext	= IIO_BIFURCATION_HELPTEXT("slot PCIE5"),
	.default_value	= IIO_BIFURCATE_xxxxxx16,
	.values		= iio_bifurcation_values,
});

static const struct sm_object iio_bifurcation_iou2 = SM_DECLARE_ENUM({
	.opt_name	= "iio_bifurcation_iou2",
	.ui_name	= "PCIE1 bifurcation",
	.ui_helptext	= IIO_BIFURCATION_HELPTEXT("slot PCIE1"),
	.default_value	= IIO_BIFURCATE_xxxxxx16,
	.values		= iio_bifurcation_values,
});

static const struct sm_object iio_bifurcation_iou3 = SM_DECLARE_ENUM({
	.opt_name	= "iio_bifurcation_iou3",
	.ui_name	= "PCIE7 bifurcation",
	.ui_helptext	= IIO_BIFURCATION_HELPTEXT("slot PCIE7"),
	.default_value	= IIO_BIFURCATE_xxxxxx16,
	.values		= iio_bifurcation_values,
});

static const struct sm_object iio_bifurcation_iou4 = SM_DECLARE_ENUM({
	.opt_name	= "iio_bifurcation_iou4",
	.ui_name	= "MCIO bifurcation",
	.ui_helptext	= IIO_BIFURCATION_HELPTEXT("the two MCIO connectors"),
	.default_value	= IIO_BIFURCATE_xxx8xxx8,
	.values		= iio_bifurcation_values,
});

static struct sm_obj_form pcie = {
	.ui_name = "PCI Express",
	.obj_list = (const struct sm_object *[]) {
		&iio_bifurcation_iou2,
		&iio_bifurcation_iou0,
		&iio_bifurcation_iou1,
		&iio_bifurcation_iou3,
		&iio_bifurcation_iou4,
		NULL
	},
};

static struct sm_obj_form *sm_root[] = {
	&pcie,
	NULL
};

void mb_cfr_setup_menu(struct lb_cfr *cfr_root)
{
	cfr_write_setup_menu(cfr_root, sm_root);
}
