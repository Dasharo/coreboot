/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <cpu/x86/msr.h>
#include <dasharo/options.h>
#include <device/device.h>
#include <intelblocks/cse.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>
#include <smbios.h>
#include <string.h>
#include <superio/nuvoton/nct6687d/nct6687d.h>

#define MSR_ATOM_TURBO_RATIO_LIMIT		0x650
#define MSR_ATOM_TURBO_RATIO_LIMIT_CORES	0x651
#define MSR_BIGCORE_TURBO_RATIO_LIMIT		0x1ad
#define MSR_BIGCORE_TURBO_RATIO_LIMIT_CORES	0x1ae


void mainboard_fill_fadt(acpi_fadt_t *fadt)
{
	fadt->preferred_pm_profile = PM_DESKTOP;
	fadt->iapc_boot_arch |= ACPI_FADT_LEGACY_DEVICES | ACPI_FADT_8042;
}


static void mainboard_init(void *chip_info)
{
	struct device *ps2_dev = dev_find_slot_pnp(0x4e, NCT6687D_KBC);
	bool ps2_en = get_ps2_option();

	ps2_dev->enabled = ps2_en & 1;
	printk(BIOS_DEBUG, "PS2 Controller state: %d\n", ps2_en);
}

static void fill_turbo_ratio_limits(FSP_S_CONFIG *params)
{
	msr_t msr;

	msr = rdmsr(MSR_BIGCORE_TURBO_RATIO_LIMIT);
	memcpy(&params->TurboRatioLimitRatio[0], &msr.lo, 4);
	memcpy(&params->TurboRatioLimitRatio[4], &msr.hi, 4);

	msr = rdmsr(MSR_BIGCORE_TURBO_RATIO_LIMIT_CORES);
	memcpy(&params->TurboRatioLimitNumCore[0], &msr.lo, 4);
	memcpy(&params->TurboRatioLimitNumCore[4], &msr.hi, 4);

	msr = rdmsr(MSR_ATOM_TURBO_RATIO_LIMIT);
	memcpy(&params->AtomTurboRatioLimitRatio[0], &msr.lo, 4);
	memcpy(&params->AtomTurboRatioLimitRatio[4], &msr.hi, 4);

	msr = rdmsr(MSR_ATOM_TURBO_RATIO_LIMIT_CORES);
	memcpy(&params->AtomTurboRatioLimitNumCore[0], &msr.lo, 4);
	memcpy(&params->AtomTurboRatioLimitNumCore[4], &msr.hi, 4);
}

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	uint8_t aspm, aspm_l1;

	if (CONFIG(PCIEXP_L1_SUB_STATE) && CONFIG(PCIEXP_CLK_PM))
		aspm_l1 = 2; // 2 - L1.1 and L1.2
	else
		aspm_l1 = 0;

	if (CONFIG(PCIEXP_ASPM)) {
		aspm = CONFIG(PCIEXP_L1_SUB_STATE) ? 3 : 1; // 3 - L0sL1, 1 - L0s
	} else {
		aspm = 0;
		aspm_l1 = 0;
	}

	memset(params->PcieRpEnableCpm, 0, sizeof(params->PcieRpEnableCpm));
	memset(params->CpuPcieRpEnableCpm, 0, sizeof(params->CpuPcieRpEnableCpm));
	memset(params->CpuPcieClockGating, 0, sizeof(params->CpuPcieClockGating));
	memset(params->CpuPciePowerGating, 0, sizeof(params->CpuPciePowerGating));

	params->UsbPdoProgramming = 1;
	params->CpuPcieFiaProgramming = 1;
	params->PcieRpFunctionSwap = 0;
	params->CpuPcieRpFunctionSwap = 0;
	params->PchLegacyIoLowLatency = 1;
	params->PchDmiAspmCtrl = 0;

	params->CpuPcieRpPmSci[0] = 0; // M2_1
	params->CpuPcieRpPmSci[1] = 0; // PCI_E1
	params->PcieRpPmSci[0]    = 0; // PCI_E2
	params->PcieRpPmSci[1]    = 0; // PCI_E4
	params->PcieRpPmSci[2]    = 0; // Ethernet
	params->PcieRpPmSci[4]    = 0; // PCI_E3
	params->PcieRpPmSci[8]    = 0; // M2_3
	params->PcieRpPmSci[20]   = 0; // M2_4
	params->PcieRpPmSci[24]   = 0; // M2_2

	params->PcieRpMaxPayload[0]  = 1; // PCI_E2
	params->PcieRpMaxPayload[1]  = 1; // PCI_E4
	params->PcieRpMaxPayload[2]  = 1; // Ethernet
	params->PcieRpMaxPayload[4]  = 1; // PCI_E3
	params->PcieRpMaxPayload[8]  = 1; // M2_3
	params->PcieRpMaxPayload[20] = 1; // M2_4
	params->PcieRpMaxPayload[24] = 1; // M2_2

	params->CpuPcieRpTransmitterHalfSwing[0] = 1; // M2_1
	params->CpuPcieRpTransmitterHalfSwing[1] = 1; // PCI_E1
	params->PcieRpTransmitterHalfSwing[0]    = 1; // PCI_E2
	params->PcieRpTransmitterHalfSwing[1]    = 1; // PCI_E4
	params->PcieRpTransmitterHalfSwing[2]    = 1; // Ethernet
	params->PcieRpTransmitterHalfSwing[4]    = 1; // PCI_E3
	params->PcieRpTransmitterHalfSwing[8]    = 1; // M2_3
	params->PcieRpTransmitterHalfSwing[20]   = 1; // M2_4
	params->PcieRpTransmitterHalfSwing[24]   = 1; // M2_2

	params->CpuPcieRpEnableCpm[0] = CONFIG(PCIEXP_CLK_PM); // M2_1
	params->CpuPcieRpEnableCpm[1] = CONFIG(PCIEXP_CLK_PM); // PCI_E1
	params->PcieRpEnableCpm[0]    = CONFIG(PCIEXP_CLK_PM); // PCI_E2
	params->PcieRpEnableCpm[1]    = CONFIG(PCIEXP_CLK_PM); // PCI_E4
	params->PcieRpEnableCpm[4]    = CONFIG(PCIEXP_CLK_PM); // PCI_E3
	params->PcieRpEnableCpm[8]    = CONFIG(PCIEXP_CLK_PM); // M2_3
	params->PcieRpEnableCpm[20]   = CONFIG(PCIEXP_CLK_PM); // M2_4
	params->PcieRpEnableCpm[24]   = CONFIG(PCIEXP_CLK_PM); // M2_2

	params->CpuPcieRpL1Substates[0] = aspm_l1; // M2_1
	params->CpuPcieRpL1Substates[1] = aspm_l1; // PCI_E1
	params->PcieRpL1Substates[0]    = aspm_l1; // PCI_E2
	params->PcieRpL1Substates[1]    = aspm_l1; // PCI_E4
	params->PcieRpL1Substates[4]    = aspm_l1; // PCI_E3
	params->PcieRpL1Substates[8]    = aspm_l1; // M2_3
	params->PcieRpL1Substates[20]   = aspm_l1; // M2_4
	params->PcieRpL1Substates[24]   = aspm_l1; // M2_2

	params->CpuPcieRpAspm[0] = aspm; // M2_1
	params->CpuPcieRpAspm[1] = aspm; // PCI_E1
	params->PcieRpAspm[0]    = aspm; // PCI_E2
	params->PcieRpAspm[1]    = aspm; // PCI_E4
	params->PcieRpAspm[4]    = aspm; // PCI_E3
	params->PcieRpAspm[8]    = aspm; // M2_3
	params->PcieRpAspm[20]   = aspm; // M2_4
	params->PcieRpAspm[24]   = aspm; // M2_2

	params->PcieRpAcsEnabled[0]  = 1; // PCI_E2
	params->PcieRpAcsEnabled[1]  = 1; // PCI_E4
	params->PcieRpAcsEnabled[2]  = 1; // Ethernet
	params->PcieRpAcsEnabled[4]  = 1; // PCI_E3
	params->PcieRpAcsEnabled[8]  = 1; // M2_3
	params->PcieRpAcsEnabled[20] = 1; // M2_4
	params->PcieRpAcsEnabled[24] = 1; // M2_2

	params->CpuPcieClockGating[0] = CONFIG(PCIEXP_CLK_PM);
	params->CpuPciePowerGating[0] = CONFIG(PCIEXP_CLK_PM);
	params->CpuPcieRpMultiVcEnabled[0] = 1;
	params->CpuPcieRpPeerToPeerMode[0] = 1;
	params->CpuPcieRpMaxPayload[0] = 2; // 512B
	params->CpuPcieRpAcsEnabled[0] = 1;

	params->CpuPcieClockGating[1] = CONFIG(PCIEXP_CLK_PM);
	params->CpuPciePowerGating[1] = CONFIG(PCIEXP_CLK_PM);
	params->CpuPcieRpPeerToPeerMode[1] = 1;
	params->CpuPcieRpMaxPayload[1] = 2; // 512B
	params->CpuPcieRpAcsEnabled[1] = 1;

	params->SataPortsSolidStateDrive[6] = 1; // M2_3
	params->SataPortsSolidStateDrive[7] = 1; // M2_4
	params->SataLedEnable = 1;

	/*
	 * If OcLock is not set in FSP-M UPD, FSP-S UPD will take and program
	 * zeroed turbo ratio limits. Avoid this by populating defautl values here.
	 */
	fill_turbo_ratio_limits(params);
}

#if CONFIG(GENERATE_SMBIOS_TABLES)
static const struct port_information smbios_type8_info[] = {
	{
		.internal_reference_designator = "PS2_USB1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "Keyboard",
		.external_connector_type       = CONN_PS_2,
		.port_type                     = TYPE_KEYBOARD_PORT
	},
	{
		.internal_reference_designator = "PS2_USB1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "PS2Mouse",
		.external_connector_type       = CONN_PS_2,
		.port_type                     = TYPE_MOUSE_PORT
	},
	{
		.internal_reference_designator = "PS2_USB1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 2.0 Type-A",
		.external_connector_type       = CONN_ACCESS_BUS_USB,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "PS2_USB1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 2.0 Type-A (Flash BIOS)",
		.external_connector_type       = CONN_ACCESS_BUS_USB,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "JTPM1 - TPM HDR",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "LAN_USB1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "Ethernet",
		.external_connector_type       = CONN_RJ_45,
		.port_type                     = TYPE_NETWORK_PORT
	},
	{
		.internal_reference_designator = "LAN_USB1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 3.2 Gen2x2 Type-C",
		.external_connector_type       = CONN_USB_TYPE_C,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "LAN_USB1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 3.2 Gen2 Type-A",
		.external_connector_type       = CONN_ACCESS_BUS_USB,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "SATA1",
		.internal_connector_type       = CONN_SAS_SATA,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_SATA
	},
	{
		.internal_reference_designator = "SATA2",
		.internal_connector_type       = CONN_SAS_SATA,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_SATA
	},
	{
		.internal_reference_designator = "SATA3",
		.internal_connector_type       = CONN_SAS_SATA,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_SATA
	},
	{
		.internal_reference_designator = "SATA4",
		.internal_connector_type       = CONN_SAS_SATA,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_SATA
	},
	{
		.internal_reference_designator = "SATA5",
		.internal_connector_type       = CONN_SAS_SATA,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_SATA
	},
	{
		.internal_reference_designator = "SATA6",
		.internal_connector_type       = CONN_SAS_SATA,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_SATA
	},
	{
		.internal_reference_designator = "JTBT1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_THUNDERBOLT
	},
	{
		.internal_reference_designator = "JC1 - CHASSIS INTRUSION",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "JAUD1 - FRONT AUDIO",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_AUDIO_PORT
	},
	{
		.internal_reference_designator = "AUDIO1 - REAR AUDIO",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "Audio",
		.external_connector_type       = CONN_OTHER,
		.port_type                     = TYPE_AUDIO_PORT
	},
	{
		.internal_reference_designator = "JFP1 - FRONT PANEL",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "JFP2 - PC SPEAKER",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "JBAT1 - CLEAR CMOS",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "JDASH1 - TUNING CONTROLLER",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "JRGB1 - 5050 RGB LED",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "JRAINBOW1 - WS2812B RGB LED",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "JRAINBOW2 - WS2812B RGB LED",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "CPU_FAN1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "PUMP_FAN1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "SYS_FAN1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "SYS_FAN2",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "SYS_FAN3",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "SYS_FAN4",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "SYS_FAN5",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "SYS_FAN6",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "DP_HDMI1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "HDMI",
		.external_connector_type       = CONN_OTHER,
		.port_type                     = TYPE_VIDEO_PORT
	},
	{
		.internal_reference_designator = "DP_HDMI1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "Display Port",
		.external_connector_type       = CONN_OTHER,
		.port_type                     = TYPE_VIDEO_PORT
	},
	{
		.internal_reference_designator = "USB2",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 2.0 Type-A (Upper)",
		.external_connector_type       = CONN_ACCESS_BUS_USB,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "USB2",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 2.0 Type-A (Lower)",
		.external_connector_type       = CONN_ACCESS_BUS_USB,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "USB2",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 3.2 Gen1 Type-A (Upper)",
		.external_connector_type       = CONN_ACCESS_BUS_USB,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "USB2",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 3.2 Gen1 Type-A (Lower)",
		.external_connector_type       = CONN_ACCESS_BUS_USB,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "JUSB1 - USB 2.0 ",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "JUSB2 - USB 2.0",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "JUSB3 - USB 3.2 GEN 1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "JUSB4 - USB 3.2 GEN 1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "JUSB5 - USB-C",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "ATX_PWR1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "CPU_PWR1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "CPU_PWR2",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
};

static int mainboard_smbios_data(struct device *dev, int *handle, unsigned long *current)
{
	int len = 0;

	len += cse_write_smbios_type14(handle, current);

	// add port information
	len += smbios_write_type8(
		current, handle,
		smbios_type8_info,
		ARRAY_SIZE(smbios_type8_info)
		);

	return len;
}
#endif

static void mainboard_enable(struct device *dev)
{
#if CONFIG(GENERATE_SMBIOS_TABLES)
	dev->ops->get_smbios_data = mainboard_smbios_data;
#endif
}

static void mainboard_final(void *chip_info)
{
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};
