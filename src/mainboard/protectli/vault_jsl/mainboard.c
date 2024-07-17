/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/mmio.h>
#include <device/device.h>
#include <device/pci_ids.h>
#include <fmap.h>
#include <gpio.h>
#include <pc80/i8254.h>
#include <smbios.h>
#include <soc/gpio.h>
#include <soc/intel/common/reset.h>
#include <soc/iomap.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>

#define FSP_PCH_PCIE_ASPM_L1 2

#define IFD_FIA_COMBO_PORT0	0x189
#define  FIA_PCIE		5
#define  FIA_SATA		7

/* Flash Master 1 : HOST/BIOS */
#define FLMSTR1			0x80
/* Flash signature Offset */
#define FLASH_SIGN_OFFSET	0x10
#define FLMSTR_WR_SHIFT_V2	20
#define FLASH_VAL_SIGN		0xFF0A55A
#define SI_DESC_SIZE		0x1000
#define SI_DESC_REGION		"SI_DESC"

/* It checks whether host (Flash Master 1) has write access to the Descriptor Region or not */
static bool is_descriptor_writeable(uint8_t *desc)
{
	/* Check flash has valid signature */
	if (read32((void *)(desc + FLASH_SIGN_OFFSET)) != FLASH_VAL_SIGN) {
		printk(BIOS_ERR, "Flash Descriptor is not valid\n");
		return 0;
	}
	/* Check host has write access to the Descriptor Region */
	if (!((read32((void *)(desc + FLMSTR1)) >> FLMSTR_WR_SHIFT_V2) & BIT(0))) {
		printk(BIOS_ERR, "Host doesn't have write access to Descriptor Region\n");
		return 0;
	}
	return 1;
}

static void descriptor_patch_pcie8_lane(int is_nvme)
{
	uint8_t *si_desc_buf;
	struct region_device desc_rdev;
	uint8_t state = is_nvme ? FIA_PCIE : FIA_SATA;

	si_desc_buf = (uint8_t *)malloc(SI_DESC_SIZE);

	if (!si_desc_buf) {
		printk(BIOS_ERR, "Failed to allocate buffer for %s\n", SI_DESC_REGION);
		return;
	}

	if (fmap_locate_area_as_rdev_rw(SI_DESC_REGION, &desc_rdev) < 0) {
		printk(BIOS_ERR, "Failed to locate %s in the FMAP\n", SI_DESC_REGION);
		free(si_desc_buf);
		return;
	}
	if (rdev_readat(&desc_rdev, si_desc_buf, 0, SI_DESC_SIZE) != SI_DESC_SIZE) {
		printk(BIOS_ERR, "Failed to read Descriptor Region from SPI Flash\n");
		free(si_desc_buf);
		return;
	}
	if (!is_descriptor_writeable(si_desc_buf)) {
		free(si_desc_buf);
		return;
	}

	/*
	 * Offset 0x189 bits [0:3]: 5=PCIe, 7=SATA.
	 * Not patching the descriptor will result in SATA or NVMe not working.
	 */
	if ((si_desc_buf[IFD_FIA_COMBO_PORT0] & 0xf) == state) {
		printk(BIOS_DEBUG, "Update of Descriptor is not required!\n");
		free(si_desc_buf);
		return;
	}

	si_desc_buf[IFD_FIA_COMBO_PORT0] &= 0xf0;
	si_desc_buf[IFD_FIA_COMBO_PORT0] |= state;

	/* FIT als sets reserved offset 0x1a4 bits [0:3]: 5=PCIe, 1=SATA */
	if (state == FIA_PCIE) {
		si_desc_buf[0x1a4] &= 0xf0;
		si_desc_buf[0x1a4] |= 5;
	} else {
		si_desc_buf[0x1a4] &= 0xf0;
		si_desc_buf[0x1a4] |= 1;
	}

	if (rdev_eraseat(&desc_rdev, 0, SI_DESC_SIZE) != SI_DESC_SIZE) {
		printk(BIOS_ERR, "Failed to erase Descriptor Region area\n");
		free(si_desc_buf);
		return;
	}

	if (rdev_writeat(&desc_rdev, si_desc_buf, 0, SI_DESC_SIZE) != SI_DESC_SIZE) {
		printk(BIOS_ERR, "Failed to update Descriptor Region\n");
		free(si_desc_buf);
		return;
	}

	printk(BIOS_DEBUG, "Update of Descriptor successful\n");
	free(si_desc_buf);
	do_global_reset();
}

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

	eth = pcidev_path_behind(parent->downstream, PCI_DEVFN(0, 0));
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
		eth->upstream->secondary,		/* bus */
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

	switch_dev = pcidev_path_behind(root_port->downstream, PCI_DEVFN(0, 0));
	if (!switch_dev || !switch_dev->enabled)
		return 0;

	/* Only ASMedia switch */
	if (switch_dev->vendor != 0x1b21 || switch_dev->device != 0x2806)
		return 0;

	/* After upstream port there is also one downstream port */
	switch_dev = pcidev_path_behind(switch_dev->downstream, PCI_DEVFN(0, 0));
	if (!switch_dev || !switch_dev->enabled)
		return 0;

	/* Only ASMedia switch */
	if (switch_dev->vendor != 0x1b21 || switch_dev->device != 0x2806)
		return 0;

	/* Loop through all downstream ports of the switch */
	for (child = switch_dev->upstream->children; child; child = child->sibling) {
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

	if (CONFIG(BOARD_PROTECTLI_V1410)) {
		len += smbios_type41_write_ethernet(PCH_DEV_PCIE1, handle, current);
		len += smbios_type41_write_ethernet(PCH_DEV_PCIE2, handle, current);
	} else if (CONFIG(BOARD_PROTECTLI_V1610)) {
		len += smbios_type41_write_ethernet_behind_switch(PCH_DEV_PCIE1, handle,
								  current);
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

	for (uint8_t i = 0; i < CONFIG_MAX_ROOT_PORTS; i++) {
		params->PcieRpAcsEnabled[i] = 1;
		params->PcieRpAdvancedErrorReporting[i] = 1;
		params->PcieRpLtrEnable[i] = 1;
		params->PcieRpMaxPayload[i] = 1; // 256B
		params->PcieRpSlotImplemented[i] = 1;
	}

	if (CONFIG(BOARD_PROTECTLI_V1610)) {
		/* Read GPP_C6 (M2_PEDET) to check for disk type: 1=NVME, 0=SATA */
		gpio_input_pullup(GPP_C6);
		/*
		 * Patch the descriptor accordingly, by default the lanes 7 and 8
		 * are in 1x2 mode. Lane 8 can be statically assigned to either
		 * PCIe or SATA
		 */
		descriptor_patch_pcie8_lane(gpio_get(GPP_C6));
	}

	/*
	 * Enable only L1 for WiFi, L0s doesn't work reliably for Atheros QCA6174.
	 * On V1610, WiFi is connected to different root port through PCIe switch
	 * (ASMedia ASM1806), but the switch doesn't support L0s on upstream port
	 * so this workaround isn't needed there.
	 */
	if (!CONFIG(BOARD_PROTECTLI_V1610))
		params->PcieRpAspm[4] = FSP_PCH_PCIE_ASPM_L1;

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
