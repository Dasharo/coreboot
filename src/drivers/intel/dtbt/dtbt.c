/* SPDX-License-Identifier: GPL-2.0-only */

#include "chip.h"
#include <acpi/acpigen.h>
#include <console/console.h>
#include <delay.h>
#include <device/device.h>
#include <device/mmio.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <device/pciexp.h>
#include <soc/pm.h>
#include <soc/pmc.h>
#include <timer.h>

#define PCIEXP_TBT_HOTPLUG_BUSES 42
#define PCIEXP_TBT_HOTPLUG_MEM 0xc200000
#define PCIEXP_TBT_HOTPLUG_PREFETCH_MEM 0x1c000000

#define PCIE2TBT 0x54C
#define PCIE2TBT_VALID BIT(0)
#define PCIE2TBT_GO2SX 2
#define PCIE2TBT_GO2SX_NO_WAKE 3
#define PCIE2TBT_SX_EXIT_TBT_CONNECTED 4
#define PCIE2TBT_SX_EXIT_NO_TBT_CONNECTED 5
#define PCIE2TBT_SET_SECURITY_LEVEL 8
#define PCIE2TBT_GET_SECURITY_LEVEL 9
#define PCIE2TBT_BOOT_ON 24
#define PCIE2TBT_FIRMWARE_CM_MODE 34
#define PCIE2TBT_PASS_THROUGH_MODE 45
#define TBT2PCIE 0x548
#define TBT2PCIE_DONE BIT(0)
#define TIMEOUT_MS 1000

static void dtbt_cmd(struct device *dev, u32 command)
{
	printk(BIOS_DEBUG, "DTBT send command %08x\n", command);

	pci_write_config32(dev, PCIE2TBT, (command << 1) | PCIE2TBT_VALID);

	if (!wait_ms(TIMEOUT_MS, pci_read_config32(dev, TBT2PCIE) & TBT2PCIE_DONE)) {
		printk(BIOS_ERR, "DTBT command %08x send timeout\n", command);
	}

	pci_write_config32(dev, PCIE2TBT, 0);
	if (!wait_ms(TIMEOUT_MS, !(pci_read_config32(dev, TBT2PCIE) & TBT2PCIE_DONE))) {
		printk(BIOS_ERR, "DTBT command %08x clear timeout\n", command);
	}
}

static bool is_pcie_port_type(const struct device *dev, u16 type)
{
	unsigned int pciexpos;
	u16 flags;

	pciexpos = pci_find_capability(dev, PCI_CAP_ID_PCIE);
	if (pciexpos) {
		flags = pci_read_config16(dev, pciexpos + PCI_EXP_FLAGS);
		if (((flags & PCI_EXP_FLAGS_TYPE) >> 4) == type)
			return true;
	}

	return false;
}

#if CONFIG(HAVE_ACPI_TABLES)
static void dtbt_write_dsd(void)
{
	struct acpi_dp *dsd = acpi_dp_new_table("_DSD");

	acpi_device_add_hotplug_support_in_d3(dsd);
	acpi_device_add_external_facing_port(dsd);
	acpi_dp_write(dsd);
}

static void dtbt_write_opregion(const struct bus *bus)
{
	uintptr_t mmconf_base = (uintptr_t)CONFIG_ECAM_MMCONF_BASE_ADDRESS
			+ (((uintptr_t)(bus->secondary)) << 20);
	const struct opregion opregion = OPREGION("PXCS", SYSTEMMEMORY, mmconf_base, 0x1000);
	const struct fieldlist fieldlist[] = {
		FIELDLIST_OFFSET(TBT2PCIE),
		FIELDLIST_NAMESTR("TB2P", 32),
		FIELDLIST_OFFSET(PCIE2TBT),
		FIELDLIST_NAMESTR("P2TB", 32),
	};

	acpigen_write_opregion(&opregion);
	acpigen_write_field("PXCS", fieldlist, ARRAY_SIZE(fieldlist),
			    FIELD_DWORDACC | FIELD_NOLOCK | FIELD_PRESERVE);
}

static void dtbt_fill_ssdt(const struct device *dev)
{
	struct bus *bus;
	struct device *parent;
	const char *parent_scope;
	const char *dev_name = acpi_device_name(dev);
	u16 dev_id;
	u32 address;

	bus = dev->bus;
	if (!bus) {
		printk(BIOS_ERR, "DTBT bus invalid\n");
		return;
	}

	parent = bus->dev;
	if (!parent || parent->path.type != DEVICE_PATH_PCI) {
		printk(BIOS_ERR, "DTBT parent invalid\n");
		return;
	}

	parent_scope = acpi_device_path(parent);
	if (!parent_scope) {
		printk(BIOS_ERR, "DTBT parent scope not valid\n");
		return;
	}

	dev_id = pci_read_config16(dev, PCI_DEVICE_ID);

	/* Only Maple Ridghe support for now */
	if (dev_id != PCI_DID_INTEL_MAPLE_RIDGE_JHL8340 &&
	    dev_id != PCI_DID_INTEL_MAPLE_RIDGE_JHL8540)
		return;


	if (is_pcie_port_type(dev, PCI_EXP_TYPE_DOWNSTREAM)) {
		/* For downstream port write Device only to make PEP happy */
		acpigen_write_scope(parent_scope);
		acpigen_write_device(dev_name);

		address = PCI_SLOT(dev->path.pci.devfn) & 0xffff;
		address <<= 16;
		address |= PCI_FUNC(dev->path.pci.devfn) & 0xffff;
		acpigen_write_name_dword("_ADR", address);

		acpigen_write_device_end();
		acpigen_write_scope_end();
		return;
	}
	/* Following code needs to be written only for upstream port */

	/* Scope */
	acpigen_write_scope(parent_scope);
	dtbt_write_dsd();

	/* Device */
	acpigen_write_device(dev_name);
	address = PCI_SLOT(dev->path.pci.devfn) & 0xffff;
	address <<= 16;
	address |= PCI_FUNC(dev->path.pci.devfn) & 0xffff;
	acpigen_write_name_dword("_ADR", address);
	dtbt_write_opregion(bus);

	/* Method */
	acpigen_write_method_serialized("PTS", 0);

	acpigen_write_debug_string("DTBT prepare to sleep");
	acpigen_write_store_int_to_namestr(PCIE2TBT_GO2SX_NO_WAKE << 1, "P2TB");
	acpigen_write_delay_until_namestr_int(600, "TB2P", PCIE2TBT_GO2SX_NO_WAKE << 1);

	acpigen_write_debug_namestr("TB2P");
	acpigen_write_store_int_to_namestr(0, "P2TB");
	acpigen_write_delay_until_namestr_int(600, "TB2P", 0);
	acpigen_write_debug_namestr("TB2P");

	acpigen_write_method_end();
	acpigen_write_device_end();
	acpigen_write_scope_end();

	printk(BIOS_DEBUG, "DTBT fill SSDT\n");
	printk(BIOS_DEBUG, "  Dev %s\n", dev_path(dev));
	printk(BIOS_DEBUG, "  Bus %s\n", bus_path(bus));
	printk(BIOS_DEBUG, "  Parent %s\n", dev_path(parent));
	printk(BIOS_DEBUG, "  Scope %s\n", parent_scope);
	printk(BIOS_DEBUG, "    Device %s\n", dev_name);
	
	// \.TBTS Method
	acpigen_write_scope("\\");
	acpigen_write_method("TBTS", 0);
	acpigen_emit_namestring(acpi_device_path_join(dev, "PTS"));
	acpigen_write_method_end();
	acpigen_write_scope_end();
}

static const char *dtbt_downstream_port_acpi_name(const struct device *dev)
{
	static char acpi_name[5];

	/* ACPI 6.3, ASL 20.2.2: (Name Objects Encoding). */
	snprintf(acpi_name, sizeof(acpi_name), "TB%02X",
		 PCI_FUNC(dev->path.pci.devfn));
	return acpi_name;
}

static const char *dtbt_acpi_name(const struct device *dev)
{
	if (!acpi_device_path(dev)) {
		printk(BIOS_INFO, "%s, NULL device path for dev %s\n", __func__, dev_path(dev));
	} else {
		printk(BIOS_INFO, "%s: %s at %s\n", __func__, acpi_device_path(dev),
		       dev_path(dev));
	}

	if (is_pcie_port_type(dev, PCI_EXP_TYPE_UPSTREAM))
		return "DTBT";
	else if (is_pcie_port_type(dev, PCI_EXP_TYPE_DOWNSTREAM))
		return dtbt_downstream_port_acpi_name(dev);
	else
		return "PXSX";
}
#endif // HAVE_ACPI_TABLES


static void clear_vga_bits(struct device *dev)
{
	uint16_t bridge_ctrl;

	if (is_pcie_port_type(dev, PCI_EXP_TYPE_ROOT_PORT)) {
		printk(BIOS_ERR, "DTBT: %s is not a root port\n", dev_path(dev));
		return;
	}

	bridge_ctrl = pci_read_config16(dev, PCI_BRIDGE_CONTROL);
	bridge_ctrl &= ~(PCI_BRIDGE_CTL_VGA | PCI_BRIDGE_CTL_VGA16);
	pci_write_config16(dev, PCI_BRIDGE_CONTROL, bridge_ctrl);
}

static void dtbt_hotplug_dummy_read_resources(struct device *dev)
{
	struct resource *resource;

	/* Add extra memory space */
	resource = new_resource(dev, 0x10);
	resource->size = PCIEXP_TBT_HOTPLUG_MEM;
	resource->align = 12;
	resource->gran = 12;
	resource->limit = 0xffffffff;
	resource->flags |= IORESOURCE_MEM;

	/* Add extra prefetchable memory space */
	resource = new_resource(dev, 0x14);
	resource->size = PCIEXP_TBT_HOTPLUG_PREFETCH_MEM;
	resource->align = 12;
	resource->gran = 12;
	resource->limit = 0xffffffffffffffff;
	resource->flags |= IORESOURCE_MEM | IORESOURCE_PREFETCH | IORESOURCE_ABOVE_4G;
}

static struct device_operations dtbt_hotplug_dummy_ops = {
	.read_resources   = dtbt_hotplug_dummy_read_resources,
	.set_resources    = noop_set_resources,
};

static void pciexp_dtbt_scan_bridge(struct device *dev)
{
	dev->hotplug_port = 1;
	dev->hotplug_buses = PCIEXP_TBT_HOTPLUG_BUSES;

	/* Normal PCIe Scan */
	pciexp_scan_bridge(dev);

	/* Add dummy slot to preserve resources, must happen after bus scan */
	struct device *dummy;
	struct device_path dummy_path = { .type = DEVICE_PATH_NONE };
	dummy = alloc_dev(dev->link_list, &dummy_path);
	dummy->ops = &dtbt_hotplug_dummy_ops;
}


static void dtbt_enable(struct device *dev)
{
	/* Override the scan_bus operation for devices hotpluggable to downstream TB4 ports */
	if (is_pcie_port_type(dev, PCI_EXP_TYPE_DOWNSTREAM)) {
		/* Only odd device numbers are PCIe TBT root ports */
		if (!CONFIG(PCIEXP_HOTPLUG) && ((dev->path.pci.devfn >> 3) & 1))
			dev->ops->scan_bus = pciexp_dtbt_scan_bridge;

		return;
	}

	/* Enable routine needs to be done only for upstream port */
	if (!is_pcie_port_type(dev, PCI_EXP_TYPE_UPSTREAM))
		return;

	printk(BIOS_INFO, "DTBT controller found at %s\n", dev_path(dev));

	/* Set tPCH25 to 10ms */
	setbits32(pmc_mmio_regs() + PM_CFG, 3); 

	clear_vga_bits(dev->bus->dev);

	printk(BIOS_DEBUG, "DTBT get security level\n");
	dtbt_cmd(dev, PCIE2TBT_GET_SECURITY_LEVEL);
	printk(BIOS_DEBUG, "DTBT set security level SL0\n");
	dtbt_cmd(dev, PCIE2TBT_SET_SECURITY_LEVEL);
	printk(BIOS_DEBUG, "DTBT get security level\n");
	dtbt_cmd(dev, PCIE2TBT_GET_SECURITY_LEVEL);

	if (acpi_is_wakeup_s3()) {
		printk(BIOS_DEBUG, "DTBT SX exit\n");
		dtbt_cmd(dev, PCIE2TBT_SX_EXIT_TBT_CONNECTED);
	} else {
		printk(BIOS_DEBUG, "DTBT set FW CM\n");
		dtbt_cmd(dev, PCIE2TBT_FIRMWARE_CM_MODE);
		printk(BIOS_DEBUG, "DTBT boot on\n");
		dtbt_cmd(dev, PCIE2TBT_BOOT_ON);
	}
}

static struct pci_operations dtbt_device_ops_pci = {
	.set_subsystem = 0,
};

static struct device_operations dtbt_device_ops = {
	.read_resources   = pci_bus_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_bus_enable_resources,
#if CONFIG(HAVE_ACPI_TABLES)
	.acpi_name        = dtbt_acpi_name,
	.acpi_fill_ssdt   = dtbt_fill_ssdt,
#endif
	.scan_bus         = pciexp_scan_bridge,
	.reset_bus        = pci_bus_reset,
	.ops_pci          = &dtbt_device_ops_pci,
	.enable           = dtbt_enable
};

static const unsigned short pci_device_ids[] = {
	PCI_DID_INTEL_MAPLE_RIDGE_JHL8340,
	PCI_DID_INTEL_MAPLE_RIDGE_JHL8540,
	0
};

static const struct pci_driver intel_dtbt_driver __pci_driver = {
	.ops     = &dtbt_device_ops,
	.vendor  = PCI_VID_INTEL,
	.devices = pci_device_ids,
};
