/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/psp_efs.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <drivers/amd/promontory21/chip.h>
#include <PROM/PromClass-api.h>
#include <vendorcode/amd/opensil/opensil.h>
#include <xSIM-api.h>
#include <static.h>

#include "prom21.h"

static void prom21_boolean_to_opensil(enum prom21_boolean b, uint8_t *out)
{
	if (b == HwDefault)
		return;

	*out = (b == Enable) ? 1 : 0;
}

static uint8_t xhci_gen_to_opensil(enum prom21_xhci_gen gen)
{
	return (gen == XhciGenDefault) ? 0xf : gen - 1;
}

static uint8_t port_gen_to_opensil(enum prom21_xhci_port_gen gen)
{
	return (gen == XhciPortGenDefault) ? 0xf : gen - 1;
}

static void apply_usb3_phy(const struct prom21_usb3_phy *src,
			   PROM21_USB3_PHY_TUNING *dst)
{
	if (!src->override)
		return;

	dst->PT21USB3PortGen1Swing          = src->gen1_swing;
	dst->PT21USB3PortGen1EmpLevelEn     = src->gen1_emp_level_en;
	dst->PT21USB3PortGen1EmpLevel       = src->gen1_emp_level;
	dst->PT21USB3PortGen1PreshootEn     = src->gen1_preshoot_en;
	dst->PT21USB3PortGen1Preshoot       = src->gen1_preshoot;
	dst->PT21USB3PortGen2Swing          = src->gen2_swing;
	dst->PT21USB3PortGen2Cp0EmpLevelEn  = src->gen2_cp0_emp_level_en;
	dst->PT21USB3PortGen2Cp0EmpLevel    = src->gen2_cp0_emp_level;
	dst->PT21USB3PortGen2Cp0PreshootEn  = src->gen2_cp0_preshoot_en;
	dst->PT21USB3PortGen2Cp0Preshoot    = src->gen2_cp0_preshoot;
	dst->PT21USB3PortGen2Cp13EmpLevelEn = src->gen2_cp13_emp_level_en;
	dst->PT21USB3PortGen2Cp13EmpLevel   = src->gen2_cp13_emp_level;
	dst->PT21USB3PortGen2Cp13PreshootEn = src->gen2_cp13_preshoot_en;
	dst->PT21USB3PortGen2Cp13Preshoot   = src->gen2_cp13_preshoot;
	dst->PT21USB3PortGen2Cp14EmpLevelEn = src->gen2_cp14_emp_level_en;
	dst->PT21USB3PortGen2Cp14EmpLevel   = src->gen2_cp14_emp_level;
	dst->PT21USB3PortGen2Cp14PreshootEn = src->gen2_cp14_preshoot_en;
	dst->PT21USB3PortGen2Cp14Preshoot   = src->gen2_cp14_preshoot;
	dst->PT21USB3PortGen2Cp15EmpLevelEn = src->gen2_cp15_emp_level_en;
	dst->PT21USB3PortGen2Cp15EmpLevel   = src->gen2_cp15_emp_level;
	dst->PT21USB3PortGen2Cp15PreshootEn = src->gen2_cp15_preshoot_en;
	dst->PT21USB3PortGen2Cp15Preshoot   = src->gen2_cp15_preshoot;
	dst->PT21USB3PortGen2Cp16EmpLevelEn = src->gen2_cp16_emp_level_en;
	dst->PT21USB3PortGen2Cp16EmpLevel   = src->gen2_cp16_emp_level;
	dst->PT21USB3PortGen2Cp16PreshootEn = src->gen2_cp16_preshoot_en;
	dst->PT21USB3PortGen2Cp16Preshoot   = src->gen2_cp16_preshoot;
}

static void apply_usb2_phy(const struct prom21_usb2_phy *src,
			   PROM21_USB2_PHY_TUNING *dst)
{
	if (!src->override)
		return;

	dst->PT21USB2SlewRate       = src->slew_rate;
	dst->PT21USB2DrivingCurrent = src->driving_current;
	dst->PT21USB2Termination    = src->termination;
}

static void apply_sata_phy(const struct prom21_sata_phy *src,
			   PROM21_SATA_PHY_TUNING *dst)
{
	if (!src->override)
		return;

	dst->PT21SataPortGen1Swing    = src->gen1_swing;
	dst->PT21SataPortGen2Swing    = src->gen2_swing;
	dst->PT21SataPortGen3Swing    = src->gen3_swing;
	dst->PT21SataPortGen1EmpLevel = src->gen1_emp_level;
	dst->PT21SataPortGen2EmpLevel = src->gen2_emp_level;
	dst->PT21SataPortGen3EmpLevel = src->gen3_emp_level;
}

static void config_usb_port_enables(struct device *xhci_dev,
				    PROM21_DATA_BLK *primary)
{
	struct bus *xhci_bus = xhci_dev->downstream;
	if (!xhci_bus)
		return;

	/* The root hub is the first (and only) child on the xHCI PCI link. */
	struct device *root_hub = xhci_bus->children;
	if (!root_hub || !root_hub->downstream)
		return;

	for (struct device *port = root_hub->downstream->children;
	     port; port = port->sibling) {
		unsigned int port_type, port_id;

		if (port->path.type != DEVICE_PATH_USB)
			continue;

		port_type = port->path.usb.port_type;
		port_id   = port->path.usb.port_id;

		if (port_type == 3 && port_id < PROM21_XHCI_NUM_USB3_PORTS)
			primary->PT21Usb3Port[port_id] = port->enabled;
		else if (port_type == 2 && port_id < PROM21_XHCI_NUM_USB2_PORTS)
			primary->PT21Usb2Port[port_id] = port->enabled;
	}
}

void opensil_promontory21_config(SIL_CONTEXT *SilContext, struct device *root_port)
{
	PROMCLASS_DATA_BLK *prom_data = SilFindStructure(SilContext, SilId_PromClass, 0);
	PROMCLASS_INPUT_BLK *input_blk;
	PROM21_DATA_BLK *primary;
	const struct drivers_amd_promontory21_config *cfg;
	const struct prom21_pcie_config *pcie;
	const struct prom21_usb_config *usb;
	const struct prom21_sata_config *sata;
	struct device *usp, *dsp;
	size_t prom_fw_size;
	int i;

	if (!prom_data) {
		printk(BIOS_ERR, "Could not find OpenSIL PROM data\n");
		return;
	}

	input_blk = &prom_data->PromInputBlk;

	prom_fw_size = efs_read_promontory_fw((void *)CONFIG_PROMONTORY_FW_ADDR);
	if (prom_fw_size) {
		input_blk->PT21FwInRamAddress = CONFIG_PROMONTORY_FW_ADDR;
		input_blk->PT21FWLoading = 0;
		input_blk->PT21RuninRam = 1;
	} else {
		input_blk->PT21FwInRamAddress = CONFIG_PROMONTORY_FW_ADDR;
		input_blk->PT21FWLoading = 1;
	}

	input_blk->PT21DisableUnusedPciePort = false;
	input_blk->PT21ClkPMEnable           = CONFIG(PCIEXP_CLK_PM);
	input_blk->PT21L1Enable              = CONFIG(PCIEXP_ASPM);
	input_blk->PT21L1ssEnable            = CONFIG(PCIEXP_L1_SUB_STATE);

	/* The USP (Upstream Switch Port) is the first PCI child of root_port. */
	if (!root_port->downstream)
		return;

	usp = root_port->downstream->children;
	if (!usp || !usp->chip_info)
		return;

	cfg = usp->chip_info;
	primary = &input_blk->Primary;

	if (pcidev_get_ssid(usp)) {
		primary->PT21SsidOverride = 1;
		primary->PT21PcieUspSsid = pcidev_get_ssid(usp);
	}

	/*
	 * Walk USP's downstream PCIe bus to derive port and function enables.
	 * DSP devices: pci 00.0-0b.0 - PCIe ports 0-11
	 *              pci 0c.0      - xHCI (USB port enables come from here)
	 *              pci 0d.0      - SATA
	 */
	for (dsp = usp->downstream ? usp->downstream->children : NULL;
	     dsp; dsp = dsp->sibling) {
		unsigned int slot;

		if (dsp->path.type != DEVICE_PATH_PCI)
			continue;

		slot = PCI_SLOT(dsp->path.pci.devfn);

		if (slot < PROM21_NUM_PCIE_LANES) {
			/* DSP PCIe port: slot 0-11 maps to port index 0-11 */
			primary->PT21PciePortEnable[slot] = dsp->enabled;
			if (dsp->enabled)
				primary->PT21PcieDspSsid = pcidev_get_ssid(dsp);
		} else if (dsp->path.pci.devfn == PROM21_XHCI_DEVFN) {
			struct device *xhci = dsp->downstream ? dsp->downstream->children :
								NULL;
			primary->PT21PcieDspXhciSsid = pcidev_get_ssid(dsp);
			if (xhci) {
				config_usb_port_enables(xhci, primary);
				primary->PT21XhciSsid = pcidev_get_ssid(xhci);
			}
		} else if (dsp->path.pci.devfn == PROM21_SATA_DEVFN) {
			primary->PT21SataEnable = dsp->enabled ? 1 : 0;
			struct device *ahci = dsp->downstream ? dsp->downstream->children :
								NULL;
			primary->PT21PcieDspAhciSsid = pcidev_get_ssid(dsp);
			if (ahci)
				primary->PT21AhciSsid = pcidev_get_ssid(ahci);
		}
	}

	/* PCIe configuration from chip config */
	pcie = &cfg->pcie;

	prom21_boolean_to_opensil(pcie->report_small_ltr, &primary->PT21LtrSmallEnable);
	primary->PT21PcieGen1SwingEnable = pcie->gen1_swing_enable;
	if (pcie->gen1_swing_enable) {
		for (i = 0; i < PROM21_NUM_PCIE_LANES; i++)
			primary->PT21PcieGen1Swing[i] = pcie->pcie_gen1_swing[i];
	}

	primary->PT21EqPreset = pcie->eq_preset;
	primary->PT21GpioPerstEnable = pcie->gpio_perst_enable;
	prom21_boolean_to_opensil(pcie->msi,  &primary->PT21Msi);
	prom21_boolean_to_opensil(pcie->msix, &primary->PT21Msix);

	for (i = 0; i < PROM21_NUM_PCIE_CLKREQ; i++) {
		primary->PT21PcieClkreqPinSelect[i] = pcie->clkreq_pin_select[i];
		primary->PT21PcieClkreqMode[i] = pcie->clkreq_mode[i];
	}

	for (i = 0; i < PROM21_NUM_PCIE_LANES / 2; i++)
		prom21_boolean_to_opensil(pcie->lane_reversal_en[i],
					  &primary->PT21PciePortLaneRev[i]);

	/* Only write port_target_speed when explicitly set (non-zero). */
	for (i = 0; i < PROM21_NUM_PCIE_LANES; i++)
		if (pcie->port_target_speed[i])
			primary->PT21PciePortTargetSpeed[i] = pcie->port_target_speed[i];

	/* Global SI program enable */
	prom21_boolean_to_opensil(cfg->si_prog_enable, &primary->PT21SIProgEnable);

	if (cfg->si_prog_enable == Enable) {
		for (i = 0; i < PROM21_XHCI_NUM_USB3_PORTS; i++)
			apply_usb3_phy(&cfg->usb3_phy[i], &primary->PT21USB3Phy[i]);
		for (i = 0; i < PROM21_XHCI_NUM_USB2_PORTS / 2; i++)
			apply_usb2_phy(&cfg->usb2_phy[i], &primary->PT21USB2Phy[i]);
		for (i = 0; i < PROM21_NUM_SATA_PORTS; i++)
			apply_sata_phy(&cfg->sata_phy[i], &primary->PT21SataPhy[i]);
	}

	/* USB functional settings from chip config */
	usb = &cfg->usb;

	primary->PT21Usb3GenSelect = xhci_gen_to_opensil(usb->usb3_gen);
	for (i = 0; i < PROM21_XHCI_NUM_USB3_PORTS; i++)
		primary->PT21XhciPortGen[i] = port_gen_to_opensil(usb->port_gen[i]);

	prom21_boolean_to_opensil(usb->hw_lpm, &primary->PT21HW_LPM);
	prom21_boolean_to_opensil(usb->dbc,    &primary->PT21DbC);

	/* SATA functional settings from chip config */
	sata = &cfg->sata;
	primary->PT21SataMode = sata->sata_mode;

	prom21_boolean_to_opensil(sata->aggresive_link_pm_cap, &primary->PT21SataAggrLinkPmCap);
	prom21_boolean_to_opensil(sata->psc_cap, &primary->PT21SataPscCap);
	prom21_boolean_to_opensil(sata->ssc_cap, &primary->PT21SataSscCap);
	prom21_boolean_to_opensil(sata->hot_plug, &primary->PT21SataHotPlug);
	prom21_boolean_to_opensil(sata->cccs_cap, &primary->PT21SataPTSataCCCSCap);
	prom21_boolean_to_opensil(sata->msi_cap, &primary->PT21AhciMsiCap);

	for (i = 0; i < PROM21_NUM_SATA_PORTS; i++) {
		primary->PT21SataPortEnable[i] = sata->port_enable[i];
		prom21_boolean_to_opensil(sata->aggressive_dev_slp[i],
				   &primary->PT21SataAggressiveDevSlp[i]);
	}
}
