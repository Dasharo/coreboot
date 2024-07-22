/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/mmio.h>
#include <device/device.h>
#include <pc80/i8254.h>
#include <device/pci_ids.h>
#include <smbios.h>
#include <soc/iomap.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>

#define FSP_PCH_PCIE_ASPM_L1 2

smbios_enclosure_type smbios_mainboard_enclosure_type(void)
{
	return SMBIOS_ENCLOSURE_LOW_PROFILE_DESKTOP;
}

u8 smbios_mainboard_feature_flags(void)
{
	return SMBIOS_FEATURE_FLAGS_HOSTING_BOARD | SMBIOS_FEATURE_FLAGS_REPLACEABLE;
}

const char *smbios_system_sku(void)
{
	return CONFIG_MAINBOARD_PART_NUMBER "-01";
}

const char *smbios_chassis_sku(void)
{
	return CONFIG_MAINBOARD_PART_NUMBER "-001";
}

 const char *smbios_chassis_version(void)
{
	return "1.2";
}

static int smbios_type41_write_ethernet(struct device *parent, int *handle,
					unsigned long *current)
{
	struct device *eth;
	static u8 instance = 0;

	if (!parent || !parent->enabled)
		return 0;

	eth = pcidev_path_behind(parent->link_list, PCI_DEVFN(0, 0));
	if (!eth || !eth->enabled)
		return 0;

	/* Only i226 Ehternet Controller */
	if (eth->vendor != PCI_VID_INTEL || eth->device != 0x125c)
		return 0;

	return smbios_write_type41(
		current, handle,
		"Onboard - Ethernet",		/* name */
		instance++,			/* instance */
		0,				/* segment */
		eth->bus->secondary,		/* bus */
		PCI_SLOT(eth->path.pci.devfn),	/* device */
		PCI_FUNC(eth->path.pci.devfn),	/* function */
		SMBIOS_DEVICE_TYPE_ETHERNET);	/* device type */
}

static int smbios_type41_write_ethernet_behind_switch(struct device *root_port, int *handle,
						      unsigned long *current)
{
	struct device *switch_dev;
	struct device *child;
	int len = 0;

	if (!root_port || !root_port->enabled)
		return 0;

	switch_dev = pcidev_path_behind(root_port->link_list, PCI_DEVFN(0, 0));
	if (!switch_dev || !switch_dev->enabled)
		return 0;

	/* Only ASMedia switch */
	if (switch_dev->vendor != 0x1b21 || switch_dev->device != 0x2806)
		return 0;

	/* After upstream port there is also one downstream port */
	switch_dev = pcidev_path_behind(switch_dev->link_list, PCI_DEVFN(0, 0));
	if (!switch_dev || !switch_dev->enabled)
		return 0;

	/* Only ASMedia switch */
	if (switch_dev->vendor != 0x1b21 || switch_dev->device != 0x2806)
		return 0;

	/* Loop through all downstream ports of the switch */
	for (child = switch_dev->bus->children; child; child = child->sibling) {
		printk(BIOS_DEBUG, "Switch DS port %s [%04x:%04x]\n",
			dev_path(child), child->vendor, child->device);
		len += smbios_type41_write_ethernet(child, handle, current);
	}

	return len;
}

static int mainboard_smbios_data(struct device *dev, int *handle,
				 unsigned long *current)
{
	int len = 0;

	len += smbios_write_type41(
		current, handle,
		"Onboard - USB Controller",	/* name */
		0,				/* instance */
		0,				/* segment */
		0,				/* bus */
		PCH_DEV_SLOT_XHCI,		/* device */
		0,				/* function */
		SMBIOS_DEVICE_TYPE_OTHER);	/* device type */

	len += smbios_write_type41(
		current, handle,
		"Onboard - HECI",		/* name */
		0,				/* instance */
		0,				/* segment */
		0,				/* bus */
		PCH_DEV_SLOT_CSE,		/* device */
		0,				/* function */
		SMBIOS_DEVICE_TYPE_OTHER);	/* device type */

	len += smbios_write_type41(
		current, handle,
		"Onboard - eMMC",		/* name */
		0,				/* instance */
		0,				/* segment */
		0,				/* bus */
		PCH_DEV_SLOT_STORAGE,		/* device */
		0,				/* function */
		SMBIOS_DEVICE_TYPE_OTHER);	/* device type */

	if (!CONFIG(BOARD_PROTECTLI_V1610)) {
		len += smbios_type41_write_ethernet(PCH_DEV_PCIE1, handle, current);
		len += smbios_type41_write_ethernet(PCH_DEV_PCIE2, handle, current);
	} else {
		len += smbios_type41_write_ethernet_behind_switch(PCH_DEV_PCIE1, handle,
								  current);
		len += smbios_type41_write_ethernet(PCH_DEV_PCIE5, handle, current);
	}
	len += smbios_type41_write_ethernet(PCH_DEV_PCIE6, handle, current);
	len += smbios_type41_write_ethernet(PCH_DEV_PCIE7, handle, current);

	return len;
}

static void mainboard_enable(struct device *dev)
{
	dev->ops->get_smbios_data = mainboard_smbios_data;
}

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	params->UsbPdoProgramming = 0;

	params->PcieRpAcsEnabled[0] = 1;
	params->PcieRpAcsEnabled[1] = 1;
	params->PcieRpAcsEnabled[3] = 1;
	params->PcieRpAcsEnabled[4] = 1;
	params->PcieRpAcsEnabled[5] = 1;
	params->PcieRpAcsEnabled[6] = 1;

	params->PcieRpAdvancedErrorReporting[0] = 1;
	params->PcieRpAdvancedErrorReporting[1] = 1;
	params->PcieRpAdvancedErrorReporting[3] = 1;
	params->PcieRpAdvancedErrorReporting[4] = 1;
	params->PcieRpAdvancedErrorReporting[5] = 1;
	params->PcieRpAdvancedErrorReporting[6] = 1;

	params->PcieRpLtrEnable[0] = 1;
	params->PcieRpLtrEnable[1] = 1;
	params->PcieRpLtrEnable[3] = 1;
	params->PcieRpLtrEnable[4] = 1;
	params->PcieRpLtrEnable[5] = 1;
	params->PcieRpLtrEnable[6] = 1;

	/*
	 * Enable L1 only for WiFi, L0s doesn't work reliably for Atheros QCA6174.
	 * On V1610, WiFi is connected to different root port through PCIe switch
	 * (ASMedia ASM1806), but the switch doesn't support L0s on upstream port
	 * so this workaround isn't needed there.
	 */
	if (!CONFIG(BOARD_PROTECTLI_V1610))
		params->PcieRpAspm[4] = FSP_PCH_PCIE_ASPM_L1;

	/* Set Max Payload to 256B */
	params->PcieRpMaxPayload[0] = 1;
	params->PcieRpMaxPayload[1] = 1;
	params->PcieRpMaxPayload[3] = 1;
	params->PcieRpMaxPayload[4] = 1;
	params->PcieRpMaxPayload[5] = 1;
	params->PcieRpMaxPayload[6] = 1;

	params->PcieRpSlotImplemented[3] = 1;
	params->PcieRpSlotImplemented[4] = 1;

	params->PcieRpDetectTimeoutMs[1] = 200;

	/* On V1610 the NVMe disk sometimes do not appear in RP with default 0ms timeout*/
	if (CONFIG(BOARD_PROTECTLI_V1610))
		params->PcieRpDetectTimeoutMs[3] = 200;

	/*
	 * HWP is too aggressive in power savings and does not let using full
	 * bandwidth of Ethernet controllers without additional stressing of
	 * the CPUs (2Gb/s vs 2.35Gb/s with stressing, measured with iperf3).
	 * Let the Linux use acpi-cpufreq governor driver instead of
	 * intel_pstate by disabling HWP.
	 */
	params->Hwp = 0;
}

static void mainboard_final(void *unused)
{
	if (CONFIG(BEEP_ON_BOOT))
		beep(1500, 100);
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
	.enable_dev = mainboard_enable,
};
