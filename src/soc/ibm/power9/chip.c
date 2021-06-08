/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <fit.h>

static int dt_platform_fixup(struct device_tree_fixup *fixup,
			      struct device_tree *tree)
{
	struct device_tree_node *node;

	/* Memory devices are always direct children of root */
	list_for_each(node, tree->root->children, list_node) {
		const char *devtype = dt_find_string_prop(node, "device_type");
		if (devtype && !strcmp(devtype, "memory")) {
			dt_add_u32_prop(node, "ibm,chip-id", 0 /* FIXME for second CPU */);
		}
	}

	return 0;
}

static void enable_soc_dev(struct device *dev)
{
	if (CONFIG(PAYLOAD_FIT_SUPPORT)) {
		struct device_tree_fixup *dt_fixup;

		dt_fixup = malloc(sizeof(*dt_fixup));
		if (dt_fixup) {
			dt_fixup->fixup = dt_platform_fixup;
			list_insert_after(&dt_fixup->list_node,
					  &device_tree_fixups);
		}
	}
}

struct chip_operations soc_ibm_power9_ops = {
  CHIP_NAME("POWER9")
  .enable_dev = enable_soc_dev,
};
