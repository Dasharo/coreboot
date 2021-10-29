/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpigen.h>
#include <acpi/acpi_device.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <device/pci.h>
#include <intelblocks/pmc.h>
#include <intelblocks/pmc_ipc.h>
#include "chip.h"

/* PCIe Root Port registers for link status and L23 control. */
#define PCH_PCIE_CFG_SSID 0x40		/* Subsystem ID */
#define PCH_PCIE_CFG_LSTS 0x52	  /* Link Status Register */
#define PCH_PCIE_CFG_SPR 0xe0	  /* Scratchpad */
#define PCH_PCIE_CFG_RPPGEN 0xe2  /* Root Port Power Gating Enable */
#define PCH_PCIE_CFG_LCAP_PN 0x4f /* Root Port Number */

/* ACPI register names corresponding to PCIe root port registers. */
#define ACPI_REG_PCI_LINK_ACTIVE "LASX"	   /* Link active status */
#define ACPI_REG_PCI_L23_RDY_ENTRY "L23E"  /* L23_Rdy Entry Request */
#define ACPI_REG_PCI_L23_RDY_DETECT "L23R" /* L23_Rdy Detect Transition */
#define ACPI_REG_PCI_L23_SAVE_STATE "NCB7" /* Scratch bit to save L23 state */

/* ACPI register names corresponding to GPU port registers */
#define ACPI_REG_PCI_SUBSYSTEM_ID "DVID"   /* Subsystem DID+VID */

/* Called from _ON to get PCIe link back to active state. */
static void nvidia_optimus_acpi_l23_exit(void)
{
	/* Skip if port is not in L2/L3. */
	acpigen_write_if_lequal_namestr_int(ACPI_REG_PCI_L23_SAVE_STATE, 1);

	/* Initiate L2/L3 Ready To Detect transition. */
	acpigen_write_store_int_to_namestr(1, ACPI_REG_PCI_L23_RDY_DETECT);

	/* Wait for transition to detect. */
	acpigen_write_delay_until_namestr_int(320, ACPI_REG_PCI_L23_RDY_DETECT, 0);

	acpigen_write_store_int_to_namestr(0, ACPI_REG_PCI_L23_SAVE_STATE);

	/* Once in detect, wait for link active. */
	acpigen_write_delay_until_namestr_int(128, ACPI_REG_PCI_LINK_ACTIVE, 1);

	acpigen_pop_len(); /* If */
}

/* Called from _OFF to put PCIe link into L2/L3 state. */
static void nvidia_optimus_acpi_l23_entry(void)
{
	/* Initiate L2/L3 Entry request. */
	acpigen_write_store_int_to_namestr(1, ACPI_REG_PCI_L23_RDY_ENTRY);

	/* Wait for L2/L3 Entry request to clear. */
	acpigen_write_delay_until_namestr_int(128, ACPI_REG_PCI_L23_RDY_ENTRY, 0);

	acpigen_write_store_int_to_namestr(1, ACPI_REG_PCI_L23_SAVE_STATE);
}

/* Restore the subsystem ID after _ON to ensure the windows driver works */
static void nvidia_optimus_acpi_subsystem_id_restore(void)
{
	uint32_t dvid = CONFIG_SUBSYSTEM_DEVICE_ID << 16 | CONFIG_SUBSYSTEM_VENDOR_ID;
	acpigen_write_store_int_to_namestr(dvid, ACPI_REG_PCI_SUBSYSTEM_ID);
}

static void
nvidia_optimus_acpi_method_on(unsigned int pcie_rp,
			 const struct drivers_gfx_nvidia_optimus_config *config)
{
	acpigen_write_method_serialized("_ON", 0);
	acpigen_write_if_lequal_namestr_int("_STA", 0);

	/* Assert enable GPIO to turn on device power. */
	if (config->enable_gpio.pin_count) {
		acpigen_enable_tx_gpio(&config->enable_gpio);
		if (config->enable_delay_ms)
			acpigen_write_sleep(config->enable_delay_ms);
	}

	/* Enable SRCCLK for root port if pin is defined. */
	if (config->srcclk_pin >= 0)
		pmc_ipc_acpi_set_pci_clock(pcie_rp, config->srcclk_pin, true);

	/* De-assert reset GPIO to bring device out of reset. */
	if (config->reset_gpio.pin_count) {
		acpigen_disable_tx_gpio(&config->reset_gpio);
		if (config->reset_delay_ms)
			acpigen_write_sleep(config->reset_delay_ms);
	}

	/* Trigger L23 ready exit flow unless disabled by config. */
	if (!config->disable_l23)
		nvidia_optimus_acpi_l23_exit();

	/* Notify EC to start polling the dGPU */
	if (config->ec_notify_method) {
		acpigen_emit_namestring(config->ec_notify_method);
		acpigen_write_integer(1);
	}

	acpigen_pop_len(); /* If */

	acpigen_write_store_int_to_namestr(1, "_STA");
	acpigen_pop_len(); /* Method */
}

static void
nvidia_optimus_acpi_method_off(int pcie_rp,
			  const struct drivers_gfx_nvidia_optimus_config *config)
{
	acpigen_write_method_serialized("_OFF", 0);
	acpigen_write_if_lequal_namestr_int("_STA", 1);

	/* Notify EC to stop polling the dGPU */
	if (config->ec_notify_method) {
		acpigen_emit_namestring(config->ec_notify_method);
		acpigen_write_integer(0);
	}

	/* Trigger L23 ready entry flow unless disabled by config. */
	if (!config->disable_l23)
		nvidia_optimus_acpi_l23_entry();

	/* Assert reset GPIO to place device into reset. */
	if (config->reset_gpio.pin_count) {
		acpigen_enable_tx_gpio(&config->reset_gpio);
		if (config->reset_off_delay_ms)
			acpigen_write_sleep(config->reset_off_delay_ms);
	}

	/* Disable SRCCLK for this root port if pin is defined. */
	if (config->srcclk_pin >= 0)
		pmc_ipc_acpi_set_pci_clock(pcie_rp, config->srcclk_pin, false);

	/* De-assert enable GPIO to turn off device power. */
	if (config->enable_gpio.pin_count) {
		acpigen_disable_tx_gpio(&config->enable_gpio);
		if (config->enable_off_delay_ms)
			acpigen_write_sleep(config->enable_off_delay_ms);
	}

	acpigen_pop_len(); /* If */

	acpigen_write_store_int_to_namestr(0, "_STA");
	acpigen_pop_len(); /* Method */
}

static void nvidia_optimus_acpi_fill_ssdt(const struct device *dev)
{
	const struct drivers_gfx_nvidia_optimus_config *config = config_of(dev);
	static const char *const power_res_states[] = {"_PR0", "_PR3"};
	const struct device *parent = dev->bus->dev;
	const char *scope = acpi_device_path(parent);

	/* PCH RP PCIe configuration space */
	const struct opregion rp_pci_config = OPREGION("PXCS", SYSTEMMEMORY, CONFIG_MMCONF_BASE_ADDRESS | (parent->path.pci.devfn << 12), 0x1000);
	const struct fieldlist rp_fieldlist[] = {
		FIELDLIST_OFFSET(PCH_PCIE_CFG_LSTS),
		FIELDLIST_RESERVED(13),
		FIELDLIST_NAMESTR(ACPI_REG_PCI_LINK_ACTIVE, 1),
		FIELDLIST_OFFSET(PCH_PCIE_CFG_SPR),
		FIELDLIST_RESERVED(7),
		FIELDLIST_NAMESTR(ACPI_REG_PCI_L23_SAVE_STATE, 1),
		FIELDLIST_OFFSET(PCH_PCIE_CFG_RPPGEN),
		FIELDLIST_RESERVED(2),
		FIELDLIST_NAMESTR(ACPI_REG_PCI_L23_RDY_ENTRY, 1),
		FIELDLIST_NAMESTR(ACPI_REG_PCI_L23_RDY_DETECT, 1),
	};
	uint8_t pcie_rp;

	/* dGPU PCIe configuration space */
	const struct opregion gpu_pci_config = OPREGION("PCIC", PCI_CONFIG, 0, 0x50);
	const struct fieldlist gpu_fieldlist[] = {
		FIELDLIST_OFFSET(PCH_PCIE_CFG_SSID),
		FIELDLIST_NAMESTR(ACPI_REG_PCI_SUBSYSTEM_ID, 32),
	};

	if (!is_dev_enabled(parent)) {
		printk(BIOS_ERR, "%s: root port not enabled\n", __func__);
		return;
	}
	if (!scope) {
		printk(BIOS_ERR, "%s: root port scope not found\n", __func__);
		return;
	}
	if (!config->enable_gpio.pin_count && !config->reset_gpio.pin_count) {
		printk(BIOS_ERR, "%s: Enable and/or Reset GPIO required for %s.\n",
		       __func__, scope);
		return;
	}
	if (config->srcclk_pin > CONFIG_MAX_PCIE_CLOCK_SRC) {
		printk(BIOS_ERR, "%s: Invalid clock pin %u for %s.\n", __func__,
		       config->srcclk_pin, scope);
		return;
	}

	/* Read port number of root port that this device is attached to. */
	pcie_rp = pci_read_config8(parent, PCH_PCIE_CFG_LCAP_PN);
	if (pcie_rp == 0 || pcie_rp > CONFIG_MAX_ROOT_PORTS) {
		printk(BIOS_ERR, "%s: Invalid root port number: %u\n", __func__, pcie_rp);
		return;
	}
	/* Port number is 1-based, PMC IPC method expects 0-based. */
	pcie_rp--;

	printk(BIOS_INFO, "%s: Enable RTD3 for %s (%s)\n", scope, dev_path(parent),
	       config->desc ?: dev->chip_ops->name);

	/* The RTD3 power resource is added to the root port, not the device. */
	acpigen_write_scope(scope);

	if (config->desc)
		acpigen_write_name_string("_DDN", config->desc);

	acpigen_write_opregion(&rp_pci_config);
	acpigen_write_field("PXCS", rp_fieldlist, ARRAY_SIZE(rp_fieldlist),
			    FIELD_ANYACC | FIELD_NOLOCK | FIELD_PRESERVE);

	acpigen_write_power_res("PWRR", 0, 0, power_res_states, ARRAY_SIZE(power_res_states));
	acpigen_write_name_integer("_STA", 1);
	nvidia_optimus_acpi_method_on(pcie_rp, config);
	nvidia_optimus_acpi_method_off(pcie_rp, config);
	acpigen_pop_len(); /* PowerResource */

	/* GPU scope */
	acpigen_write_device("DEV0");
	acpigen_write_name_integer("_ADR", 0x0);
	acpigen_write_name_integer("_S0W", ACPI_DEVICE_SLEEP_D3_COLD);
	acpigen_write_opregion(&gpu_pci_config);
	acpigen_write_field("PCIC", gpu_fieldlist, ARRAY_SIZE(gpu_fieldlist),
			    FIELD_DWORDACC | FIELD_NOLOCK | FIELD_PRESERVE);

	acpigen_write_power_res("PWRR", 0, 0, power_res_states, ARRAY_SIZE(power_res_states));

	acpigen_write_name_integer("_STA", 1);

	acpigen_write_method_serialized("_ON", 0);
	nvidia_optimus_acpi_subsystem_id_restore();
	acpigen_write_store_int_to_namestr(1, "_STA");
	acpigen_pop_len(); /* Method */

	acpigen_write_method_serialized("_OFF", 0);
	acpigen_write_store_int_to_namestr(0, "_STA");
	acpigen_pop_len(); /* Method */

	acpigen_pop_len(); /* PowerResource */
	acpigen_pop_len(); /* Device */
	acpigen_pop_len(); /* Scope */
}

static const char *nvidia_optimus_acpi_name(const struct device *dev)
{
	/* Attached device name must be "PXSX" for the Linux Kernel to recognize it. */
	return "PXSX";
}

static struct device_operations nvidia_optimus_ops = {
	.read_resources = noop_read_resources,
	.set_resources = noop_set_resources,
	.acpi_fill_ssdt = nvidia_optimus_acpi_fill_ssdt,
	.acpi_name = nvidia_optimus_acpi_name,
	.ops_pci = &pci_dev_ops_pci,
};

static void nvidia_optimus_acpi_enable(struct device *dev)
{
	dev->ops = &nvidia_optimus_ops;
}

struct chip_operations drivers_gfx_nvidia_optimus_ops = {
	CHIP_NAME("NVIDIA Optimus Hybrid Graphics")
	.enable_dev = nvidia_optimus_acpi_enable
};
