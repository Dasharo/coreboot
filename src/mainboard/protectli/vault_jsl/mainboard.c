/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/mmio.h>
#include <device/device.h>
#include <gpio.h>
#include <pc80/i8254.h>
#include <smbios.h>
#include <soc/gpio.h>
#include <soc/iomap.h>
#include <soc/ramstage.h>

#define FSP_PCH_PCIE_ASPM_DISABLE 0
#define FSP_PCH_PCIE_ASPM_L0S 1

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

	/*
	 * Disable ASPM for WiFi, neither L0s nor L1 works reliably on Atheros
	 * QCA6174.
	 */
	params->PcieRpAspm[4] = FSP_PCH_PCIE_ASPM_DISABLE;

	/*
	 * Disable ASPM L1 for SSD slot, as it does not work reliably with Samsung
	 * NVMe SSDs.
	 */
	params->PcieRpAspm[0] = FSP_PCH_PCIE_ASPM_L0S;

	/*
	 * HWP is too aggressive in power savings and does not let using full
	 * bandwidth of Ethernet controllers without additional stressing of
	 * the CPUs (2Gb/s vs 2.35Gb/s with stressing, measured with iperf3).
	 * Let the Linux use acpi-cpufreq governor driver instead of
	 * intel_pstate by disabling HWP.
	 */
	params->Hwp = 0;

	/*
	 * Skip PCI Subsystem IDs programming to match proprietary FW.
	 * It also makes some Windows default drivers to probe successfully, e.g. audio.
	 */
	params->SiSkipSsidProgramming = 1;
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
