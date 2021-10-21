/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Place in devicetree.cb:
 *
 * chip drivers/ipmi
 *   device pnp e4.0 on end         # IPMI BT
 * end
 */

#include <console/console.h>
#include <device/device.h>
#include <string.h>

#include "ipmi_if.h"
#include "ipmi_bt.h"

static void ipmi_bt_init(struct device *dev)
{
	struct ipmi_devid_rsp rsp;
	struct drivers_ipmi_config *conf = dev->chip_info;

	if (!conf) {
		printk(BIOS_WARNING, "IPMI: chip_info is missing! Skip init.\n");
		return;
	}

	printk(BIOS_DEBUG, "IPMI: PNP BT 0x%x\n", dev->path.pnp.port);

	if (ipmi_process_self_test_result(dev))
		dev->enabled = 0;

	if (!ipmi_get_device_id(dev, &rsp)) {
		uint32_t man_id = 0;
		uint32_t prod_id = 0;

		/* 4 bit encoding */
		u8 ipmi_revision_minor = IPMI_IPMI_VERSION_MINOR(rsp.ipmi_version);
		u8 ipmi_revision_major = IPMI_IPMI_VERSION_MAJOR(rsp.ipmi_version);

		memcpy(&man_id, rsp.manufacturer_id, sizeof(rsp.manufacturer_id));

		memcpy(&prod_id, rsp.product_id, sizeof(rsp.product_id));

		printk(BIOS_INFO, "IPMI: Found man_id 0x%06x, prod_id 0x%04x\n",
		       man_id, prod_id);

		printk(BIOS_INFO, "IPMI: Version %01x.%01x\n",
		       ipmi_revision_major, ipmi_revision_minor);
	} else {
		dev->enabled = 0;
	}

	if (ipmi_bt_clear(dev->path.pnp.port))
		dev->enabled = 0;
}

static void ipmi_set_resources(struct device *dev)
{
	struct resource *res;

	for (res = dev->resource_list; res; res = res->next) {
		if (!(res->flags & IORESOURCE_ASSIGNED))
			continue;

		res->flags |= IORESOURCE_STORED;
		report_resource_stored(dev, res, "");
	}
}

static void ipmi_read_resources(struct device *dev)
{
	struct resource *res = new_resource(dev, 0);
	res->base = dev->path.pnp.port;
	res->size = 3;
	res->flags = IORESOURCE_IO | IORESOURCE_ASSIGNED | IORESOURCE_FIXED;
}

static struct device_operations ops = {
	.read_resources = ipmi_read_resources,
	.set_resources  = ipmi_set_resources,
	.init           = ipmi_bt_init,
};

static void enable_dev(struct device *dev)
{
	if (dev->path.type != DEVICE_PATH_PNP)
		printk(BIOS_ERR, "%s: Unsupported device type\n",
		       dev_path(dev));
	else if (dev->path.pnp.port & 3)
		printk(BIOS_ERR, "%s: Base address needs to be aligned to 4\n",
		       dev_path(dev));
	else
		dev->ops = &ops;
}

struct chip_operations drivers_ipmi_ops = {
	CHIP_NAME("IPMI BT")
	.enable_dev = enable_dev,
};
