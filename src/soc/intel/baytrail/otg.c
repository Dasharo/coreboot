/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <acpi/acpi_gnvs.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <device/pci_ids.h>
#include <stdint.h>
#include <reg_script.h>

#include <soc/device_nvs.h>
#include <soc/gpio.h>
#include <soc/iomap.h>
#include <soc/iosf.h>
#include <soc/irq.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>

#include "chip.h"

const struct reg_script otg_power_save[] = {
	REG_IOSF_OR(IOSF_PORT_OTG, 0x1e0, 1 << 4), /* vlv.usb.xdci_otg.controller.pmctl.iosfprim_trunk_gate_en */
	REG_IOSF_OR(IOSF_PORT_OTG, 0x1e0, 1 << 0), /* vlv.usb.xdci_otg.controller.pmctl.iosfprimclk_gate_en */
	REG_IOSF_OR(IOSF_PORT_OTG, 0x1e0, 1 << 5), /* vlv.usb.xdci_otg.controller.pmctl.iosfsb_trunk_gate_en */
	REG_IOSF_OR(IOSF_PORT_OTG, 0x1e0, 1 << 3), /* vlv.usb.xdci_otg.controller.pmctl.iosfsbclk_gate_en */
	REG_IOSF_OR(IOSF_PORT_OTG, 0x1e0, 1 << 1), /* vlv.usb.xdci_otg.controller.pmctl.ocpclk_gate_en */
	REG_IOSF_OR(IOSF_PORT_OTG, 0x1e0, 1 << 2), /* vlv.usb.xdci_otg.controller.pmctl.ocpclk_trunk_gate_en */
	REG_SCRIPT_END
};

const struct reg_script configure_usb_ulpi[] = {
    /* USB_ULPI_0_CLK */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x338, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x330, ~(0x187), (0x101)),
    /* USB_ULPI_0_DATA0 */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x388, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x380, ~(0x187), (0x101)),
    /* USB_ULPI_0_DATA1 */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x368, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x360, ~(0x187), (0x101)),
    /* USB_ULPI_0_DATA2 */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x318, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x310, ~(0x187), (0x101)),
    /* USB_ULPI_0_DATA3 */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x378, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x370, ~(0x187), (0x101)),
    /* USB_ULPI_0_DATA4 */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x308, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x300, ~(0x187), (0x101)),
    /* USB_ULPI_0_DATA5 */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x398, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x390, ~(0x187), (0x101)),
    /* USB_ULPI_0_DATA6 */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x328, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x320, ~(0x187), (0x101)),
    /* USB_ULPI_0_DATA7 */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x3a8, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x3a0, ~(0x187), (0x101)),
    /* USB_ULPI_0_DIR */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x348, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x340, ~(0x187), (0x81)),
    /* USB_ULPI_0_NXT */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x358, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x350, ~(0x187), (0x101)),
    /* USB_ULPI_0_STP */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x3b8, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x3b0, ~(0x187), (0x81)),
    /* USB_ULPI_0_REFCLK */
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x288, ~(0x7),   (0x2)),
    REG_MMIO_RMW32(GPSSUS_PAD_BASE + 0x280, ~(0x187), (0x101))
};

const struct reg_script otg_init_script[] = {
	REG_SCRIPT_NEXT(configure_usb_ulpi),
	/* Disable USB_ULPI_REFCLK */
	REG_PCI_OR32(0xa0, 1 << 17),
	/* VCO calibration code starting point = 0xa */
	REG_RES_RMW16(PCI_BASE_ADDRESS_0, 0x1000ac, ~0x1f, 0xa),
	/* PLL_VCO_calibration_trim_code = 7 */
	REG_RES_RMW16(PCI_BASE_ADDRESS_0, 0x100784, ~(7 << 4), 7 << 4),
	/* vlv.usb.xdci_otg.usb3_cadence.otg3_phy.u1_power_state_definition.tx_en = 1 */
	REG_RES_OR16(PCI_BASE_ADDRESS_0,  0x100004, 1 << 2),
	/* Configure INTA for PCI mode */
	REG_IOSF_RMW(IOSF_PORT_OTG, OTG_PCICFGCTR1,
		     ~OTG_PCICFGCTR1_INT_PIN_MASK, (INTA << OTG_PCICFGCTR1_INT_PIN_SHIFT)),
	REG_SCRIPT_END
};

static void store_acpi_nvs(struct device *dev)
{
	struct resource *bar;
	struct device_nvs *dev_nvs = acpi_get_device_nvs();

	/* Save BAR0 and BAR1 to ACPI NVS */
	bar = probe_resource(dev, PCI_BASE_ADDRESS_0);
	if (bar)
		dev_nvs->otg_bar0 = (u32)bar->base;

	bar = probe_resource(dev, PCI_BASE_ADDRESS_1);
	if (bar)
		dev_nvs->otg_bar1 = (u32)bar->base;
}

static void otg_enable_acpi_mode(struct device *dev)
{
	struct reg_script ops[] = {
		/* Disable PCI interrupt, enable Memory and Bus Master */
		REG_PCI_OR16(PCI_COMMAND,
			     PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER | PCI_COMMAND_INT_DISABLE),
		/* Enable ACPI mode */
		REG_IOSF_OR(IOSF_PORT_OTG, OTG_PCICFGCTR1,
			    OTG_PCICFGCTR1_PCI_CFG_DIS | OTG_PCICFGCTR1_ACPI_INT_EN),

		REG_SCRIPT_END
	};
	struct device_nvs *dev_nvs = acpi_get_device_nvs();

	store_acpi_nvs(dev);

	/* Device is enabled in ACPI mode */
	dev_nvs->otg_en = 1;

	/* Put device in ACPI mode */
	reg_script_run_on_dev(dev, ops);
}

static void otg_init(struct device *dev)
{
	struct soc_intel_baytrail_config *config = config_of(dev);

	/* Configure OTG device */
	reg_script_run_on_dev(dev, otg_init_script);

	if (config->otg_acpi_mode)
		otg_enable_acpi_mode(dev);
	else
		store_acpi_nvs(dev);
}

static void otg_final(struct device *dev)
{
	/*
	 * S0ix PnP setting for OTG. Was in perf_power_settings, but
	 * the IOSF writes hang the platform when OTG is disabled.
	 */
	reg_script_run_on_dev(dev, otg_power_save);
}

static struct device_operations xhci_device_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= otg_init,
	.final			= otg_final,
	.enable			= southcluster_enable_dev,
	.ops_pci		= &soc_pci_ops,
};

static const struct pci_driver baytrail_xhci __pci_driver = {
	.ops    = &xhci_device_ops,
	.vendor	= PCI_VID_INTEL,
	.device = OTG_DEVID
};
