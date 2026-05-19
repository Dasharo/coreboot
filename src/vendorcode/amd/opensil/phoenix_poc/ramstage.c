/* SPDX-License-Identifier: GPL-2.0-only */

#include <CCX/CcxClass-api.h>
#include <CCX/Common/CcxApic.h>
#include <FCH/Common/FchCommon.h>
#include <FCH/FchUsb-api.h>
#include <FCH/Tacoma/FchCore/FchUsb/FchUsbOemTc.h>
#include <PROM/PromClass-api.h>
#include <RcMgr/DfX/RcManager-api.h>
#include <amdblocks/reset.h>
#include <bootstate.h>
#include <cbmem.h>
#include <cpu/amd/microcode.h>
#include <cpu/amd/mtrr.h>
#include <cpu/cpu.h>
#include <cpu/x86/smm.h>
#include <console/console.h>
#include <device/device.h>
#include <soc/amd/phoenix/chip.h>
#include <soc/aoac_defs.h>
#include <soc/iomap.h>
#include <static.h>
#include <stdio.h>
#include <xSIM-api.h>

#include "opensil_console.h"
#include "../opensil.h"

void SIL_STATUS_report(const char *function, const int status)
{
	const int log_level = status == SilPass ? BIOS_DEBUG : BIOS_ERR;
	const char *error_string = "Unknown error";

	const struct error_string_entry {
		SIL_STATUS status;
		const char *string;
	} errors[] = {
		{SilPass, "SilPass"},
		{SilUnsupportedHardware, "SilUnsupportedHardware"},
		{SilUnsupported, "SilUnsupported"},
		{SilInvalidParameter, "SilInvalidParameter"},
		{SilAborted, "SilAborted"},
		{SilOutOfResources, "SilOutOfResources"},
		{SilNotFound, "SilNotFound"},
		{SilOutOfBounds, "SilOutOfBounds"},
		{SilDeviceError, "SilDeviceError"},
		{SilResetRequestColdImm, "SilResetRequestColdImm"},
		{SilResetRequestColdDef, "SilResetRequestColdDef"},
		{SilResetRequestWarmImm, "SilResetRequestWarmImm"},
		{SilResetRequestWarmDef, "SilResetRequestWarmDef"},
	};

	int i;
	for (i = 0; i < ARRAY_SIZE(errors); i++) {
		if (errors[i].status == status)
			error_string = errors[i].string;
	}
	printk(log_level, "%s returned %d (%s)\n", function, status, error_string);
}

static void setup_rc_manager_default(SIL_CONTEXT *SilContext)
{
	DFX_RCMGR_INPUT_BLK *rc_mgr_input_block = SilFindStructure(SilContext, SilId_RcManager,  0);

	if (!rc_mgr_input_block)
		return;

	/* Let openSIL distribute the resources to the different PCI roots */
	rc_mgr_input_block->SetRcBasedOnNv = false;
	rc_mgr_input_block->SocketNumber = 1;
	rc_mgr_input_block->RbsPerSocket = 1; /* PCI root bridges per socket */
	rc_mgr_input_block->PciExpressBaseAddress = CONFIG_ECAM_MMCONF_BASE_ADDRESS;
	rc_mgr_input_block->MmioSizePerRbForNonPciDevice = 16 * MiB;
	rc_mgr_input_block->BottomMmioReservedForPrimaryRb = 4ull * GiB - 32 * MiB;
	/* MmioAbove4GLimit will be adjusted down in openSIL */
	rc_mgr_input_block->MmioAbove4GLimit = POWER_OF_2(cpu_phys_address_size());
	rc_mgr_input_block->Above4GMmioSizePerRbForNonPciDevice = 0;
	/* Enforce remapping and address space reduction, as this is what AGESA does */
	rc_mgr_input_block->AmdFabric1TbRemap = 1;
	rc_mgr_input_block->AmdSmee = true;
}

#define TACOMA_USB_STRUCT_MAJOR_VERSION 0xf
#define TACOMA_USB_STRUCT_MINOR_VERSION 0x1

static FCH_TC_USB_OEM_PLATFORM_TABLE usb_config = { 0 };

#define NUM_XHCI_CONTROLLERS 4
#define NUM_USB4_CONTROLLERS 2
static void configure_usb(SIL_CONTEXT *SilContext)
{
	struct device *usb_ctrlr[NUM_XHCI_CONTROLLERS] = {
		DEV_PTR(xhci_0),
		DEV_PTR(xhci_1),
		DEV_PTR(usb4_xhci_0),
		DEV_PTR(usb4_xhci_1)
	};

	struct device *usb4_rt[NUM_USB4_CONTROLLERS] = {
		DEV_PTR(usb4_router_0),
		DEV_PTR(usb4_router_1)
	};

	struct device *usb4_pcie[NUM_USB4_CONTROLLERS] = {
		DEV_PTR(usb4_pcie_bridge_0),
		DEV_PTR(usb4_pcie_bridge_1)
	};

	/* In coreboot the USB4 ports are first in order, but openSIL expects the opposite */
	struct device *usb2_ports[USB2_PORT_COUNT] = {
		DEV_PTR(usb2_port2),
		DEV_PTR(usb2_port3),
		DEV_PTR(usb2_port4),
		DEV_PTR(usb2_port5),
		DEV_PTR(usb2_port6),
		DEV_PTR(usb2_port7),
		DEV_PTR(usb2_port0),
		DEV_PTR(usb2_port1)
	};

	struct device *usb3_ports[USB3_PORT_COUNT] = {
		DEV_PTR(usb3_port2),
		DEV_PTR(usb3_port3),
		DEV_PTR(usb3_port7)
	};

	struct device *usb4_xhci_ports[NUM_USB4_CONTROLLERS] = {
		DEV_PTR(usb3_port0),
		DEV_PTR(usb3_port1)
	};

	const struct usb_port_map {
		uint8_t usb2_ports;
		uint8_t usb3_ports;
	} usb_port_map[NUM_XHCI_CONTROLLERS] = {
		{ 5, 2 },
		{ 1, 1 },
		{ 1, 1 },
		{ 1, 1 }
	};

	const struct soc_amd_phoenix_config *soc_config = config_of_soc();
	const struct usb_phy_config *usb_phy = &soc_config->usb_phy;

	FCHUSB_INPUT_BLK *fch_usb_data = SilFindStructure(SilContext, SilId_FchUsb, 0);

	if (!fch_usb_data)
		return;

	fch_usb_data->XhciSsid = usb_ctrlr[0]->subsystem_vendor |
				 ((uint32_t)usb_ctrlr[0]->subsystem_device << 16);

	fch_usb_data->Xhci0Enable = is_dev_enabled(usb_ctrlr[0]);
	fch_usb_data->Xhci1Enable = is_dev_enabled(usb_ctrlr[1]);
	fch_usb_data->Usb4Host[0].Usb3HCDisable = !is_dev_enabled(usb_ctrlr[2]);
	fch_usb_data->Usb4Host[1].Usb3HCDisable = !is_dev_enabled(usb_ctrlr[3]);
	fch_usb_data->Usb4Host[0].HostEnable = is_dev_enabled(usb4_rt[0]);
	fch_usb_data->Usb4Host[1].HostEnable = is_dev_enabled(usb4_rt[1]);
	fch_usb_data->Usb4Host[0].PcieAdpHidden = !is_dev_enabled(usb4_pcie[0]);
	fch_usb_data->Usb4Host[1].PcieAdpHidden = !is_dev_enabled(usb4_pcie[1]);
	if (fch_usb_data->Usb4Host[0].PcieAdpHidden)
		fch_usb_data->Usb4Host[0].PcieTunnelingDisable = 1;
	if (fch_usb_data->Usb4Host[1].PcieAdpHidden)
		fch_usb_data->Usb4Host[1].PcieTunnelingDisable = 1;

	/*
	 * XHCI_OC structure is broken, it tries to fit u32 and u16 into single u32.
	 * It causes the memcpy to incorrectly assign USB3 OC pins. Also the OC pin map
	 * is not separate from USB2 ports, but simply follows the USB2 OC pin map and
	 * the offset depens on the port count,
	 */
	uint32_t oc_pins, mask, shift;
	for (int i = 0; i < NUM_XHCI_CONTROLLERS; i++) {
		oc_pins = 0xffffffff;
		memcpy(&oc_pins, &soc_config->usb2_oc_pins[i], sizeof(soc_config->usb2_oc_pins[i]));

		mask = (1 << (usb_port_map[i].usb2_ports * 4)) - 1;
		fch_usb_data->XhciOCpinSelect[i].OcPinSelect &= ~mask;
		fch_usb_data->XhciOCpinSelect[i].OcPinSelect |= oc_pins & mask;

		oc_pins = 0xffffffff;
		memcpy(&oc_pins, &soc_config->usb3_oc_pins[i], sizeof(soc_config->usb3_oc_pins[i]));

		mask = (1 << (usb_port_map[i].usb3_ports * 4)) - 1;
		shift = usb_port_map[i].usb2_ports * 4;
		fch_usb_data->XhciOCpinSelect[i].OcPinSelect &= ~(mask << shift);
		fch_usb_data->XhciOCpinSelect[i].OcPinSelect |= (oc_pins & mask) << shift;
	}

	for (int i = 0; i < (USB2_PORT_COUNT - NUM_USB4_CONTROLLERS); i++) {
		if (!is_dev_enabled(usb2_ports[i]))
			fch_usb_data->XhciUsb2PortDisable |= (1 << i);
	}

	for (int i = 0; i < USB3_PORT_COUNT; i++) {
		if (!is_dev_enabled(usb3_ports[i]))
			fch_usb_data->XhciUsb3PortDisable |= (1 << i);
	}

	/* USB4 ports have different bit shifts */
	if (!is_dev_enabled(usb2_ports[6]))
		fch_usb_data->XhciUsb2PortDisable |= (1 << 8);

	if (!is_dev_enabled(usb2_ports[7]))
		fch_usb_data->XhciUsb2PortDisable |= (1 << 12);

	if (!is_dev_enabled(usb4_xhci_ports[0])) {
		fch_usb_data->XhciUsb3PortDisable |= (1 << 4);
		fch_usb_data->Usb4Host[0].SSPortDisable |= 1;
	}

	if (!is_dev_enabled(usb4_xhci_ports[1])) {
		fch_usb_data->XhciUsb3PortDisable |= (1 << 6);
		fch_usb_data->Usb4Host[1].SSPortDisable |= 1;
	}

	fch_usb_data->XhciOcPolarityCfgLow = soc_config->polarity_cfg_low;
	fch_usb_data->Usb3PortForceGen1 = soc_config->usb3_force_gen1.raw;

	if (!soc_config->usb_phy_custom)
		return;

	usb_config.Version_Major = TACOMA_USB_STRUCT_MAJOR_VERSION;
	usb_config.Version_Minor = TACOMA_USB_STRUCT_MINOR_VERSION;
	usb_config.TableLength = sizeof(FCH_TC_USB_OEM_PLATFORM_TABLE);

	memcpy(usb_config.Usb3PhyPort, usb_phy->Usb3PhyPort, sizeof(usb_config.Usb3PhyPort));
	memcpy(usb_config.Usb20PhyPort, usb_phy->Usb2PhyPort, sizeof(usb_config.Usb20PhyPort));
	memcpy(usb_config.ComboPhyStaticConfig, usb_phy->ComboPhyStaticConfig,
	       sizeof(usb_config.ComboPhyStaticConfig));
	memcpy(usb_config.Reserved1, usb_phy->Usb4Phy, sizeof(usb_config.Reserved1));

	usb_config.BatteryChargerEnable = usb_phy->BatteryChargerEnable;
	usb_config.PhyP3CpmP4Support = usb_phy->PhyP3CpmP4Support;

	fch_usb_data->OemUsbConfigurationTable = (uintptr_t)&usb_config;
}

static void configure_ccx(SIL_CONTEXT *SilContext)
{
	CCXCLASS_DATA_BLK *ccx_data = SilFindStructure(SilContext, SilId_CcxClass, 0);
	UCODEPATCH_BIOSENTRYINFO *ucode_info;
	void *ucode;

	if (!ccx_data)
		return;

	if (CONFIG(XAPIC_ONLY) || CONFIG(X2APIC_LATE_WORKAROUND))
		ccx_data->CcxInputBlock.AmdApicMode = xApicMode;
	else if (CONFIG(X2APIC_ONLY))
		ccx_data->CcxInputBlock.AmdApicMode = x2ApicMode;
	else
		ccx_data->CcxInputBlock.AmdApicMode = ApicAutoMode;

	ccx_data->CcxInputBlock.EnableSvmX2AVIC = true;
	ccx_data->CcxInputBlock.EnableSvmAVIC = true;
	ccx_data->CcxInputBlock.AmdCStateIoBaseAddress = ACPI_CSTATE_CONTROL;

	ucode = amd_microcode_find();
	if (!ucode) {
		printk(BIOS_ERR, "OpenSIL: CPU microcode not found\n");
		return;
	}

	ucode_info = &ccx_data->CcxInputBlock.UcodePatchEntryInfo;
	ucode_info->UcodePatchEntryAddress = (uint64_t)ucode;
}

WEAK_DEV_PTR(i2c_0);
WEAK_DEV_PTR(i2c_1);
WEAK_DEV_PTR(i2c_2);
WEAK_DEV_PTR(i2c_3);
WEAK_DEV_PTR(uart_0);
WEAK_DEV_PTR(uart_1);
WEAK_DEV_PTR(uart_2);
WEAK_DEV_PTR(uart_3);
WEAK_DEV_PTR(uart_4);
WEAK_DEV_PTR(i3c_0);
WEAK_DEV_PTR(i3c_1);
WEAK_DEV_PTR(i3c_2);
WEAK_DEV_PTR(i3c_3);
WEAK_DEV_PTR(hfp);
WEAK_DEV_PTR(hid_2);
WEAK_DEV_PTR(hid);
WEAK_DEV_PTR(lpc_bridge);

#define FCH_DEV_ENABLE(dev, aoac_bit) \
	fch_data->FchRunTime.FchDeviceEnableMap |= \
		(is_dev_enabled(DEV_PTR(dev)) ? (1ul << aoac_bit) : 0)

static void configure_fch_acpi(SIL_CONTEXT *SilContext)
{
	FCHHWACPI_INPUT_BLK *fch_hwacpi_data = SilFindStructure(SilContext, SilId_FchHwAcpi, 0);
	FCHCLASS_INPUT_BLK *fch_data = SilFindStructure(SilContext, SilId_FchClass, 0);
	struct device *smb = DEV_PTR(smbus);
	struct device *xhci = DEV_PTR(xhci_0);
	struct device *hda = DEV_PTR(hda);

	if (!fch_hwacpi_data) {
		printk(BIOS_ERR, "OpenSIL: FCH HW ACPI data not found\n");
	} else {
		if (CONFIG_MAINBOARD_POWER_FAILURE_STATE == 2)
			fch_hwacpi_data->PwrFailShadow = UsePrevious;
		else if (CONFIG_MAINBOARD_POWER_FAILURE_STATE == 1)
			fch_hwacpi_data->PwrFailShadow = AlwaysOn;
		else
			fch_hwacpi_data->PwrFailShadow = AlwaysOff;
	}

	if (!fch_data) {
		printk(BIOS_ERR, "OpenSIL: FCH Class data not found\n");
		return;
	}

	if (smb) {
		fch_data->Smbus.SmbusSsid = smb->subsystem_vendor |
					    ((uint32_t)smb->subsystem_device << 16);
		fch_data->FchBldCfg.CfgSmbusSsid = fch_data->Smbus.SmbusSsid;
	}

	if (hda)
		fch_data->FchBldCfg.CfgAzaliaSsid = hda->subsystem_vendor |
						    ((uint32_t)hda->subsystem_device << 16);

	if (xhci)
		fch_data->FchBldCfg.CfgXhciSsid = xhci->subsystem_vendor |
						  ((uint32_t)xhci->subsystem_device << 16);

	fch_data->FchBldCfg.CfgSioPmeBaseAddress = 0;
	fch_data->FchBldCfg.CfgAcpiPm1EvtBlkAddr = ACPI_PM_EVT_BLK;
	fch_data->FchBldCfg.CfgAcpiPm1CntBlkAddr = ACPI_PM1_CNT_BLK;
	fch_data->FchBldCfg.CfgAcpiPmTmrBlkAddr = ACPI_PM_TMR_BLK;
	fch_data->FchBldCfg.CfgCpuControlBlkAddr = ACPI_CSTATE_CONTROL;
	fch_data->FchBldCfg.CfgAcpiGpe0BlkAddr = ACPI_GPE0_BLK;
	fch_data->FchBldCfg.CfgSmiCmdPortAddr = APM_CNT;

	if (CONFIG(IOAPIC_PREDEFINED_ID)) {
		fch_data->CfgIoApicIdPreDefEnable = 1;
		fch_data->FchIoApicId = 32;
	}

	fch_data->WdtEnable = false;

	/* eSPI always enabled (bit 27) */
	fch_data->FchRunTime.FchDeviceEnableMap = (1 << 27);
	FCH_DEV_ENABLE(lpc_bridge, 4);
	FCH_DEV_ENABLE(i2c_0, FCH_AOAC_DEV_I2C0);
	FCH_DEV_ENABLE(i2c_1, FCH_AOAC_DEV_I2C1);
	FCH_DEV_ENABLE(i2c_2, FCH_AOAC_DEV_I2C2);
	FCH_DEV_ENABLE(i2c_3, FCH_AOAC_DEV_I2C3);
	FCH_DEV_ENABLE(uart_0, FCH_AOAC_DEV_UART0);
	FCH_DEV_ENABLE(uart_1, FCH_AOAC_DEV_UART1);
	FCH_DEV_ENABLE(uart_2, FCH_AOAC_DEV_UART2);
	FCH_DEV_ENABLE(uart_3, FCH_AOAC_DEV_UART3);
	FCH_DEV_ENABLE(uart_4, FCH_AOAC_DEV_UART4);
	FCH_DEV_ENABLE(i3c_0, FCH_AOAC_DEV_I3C0);
	FCH_DEV_ENABLE(i3c_1, FCH_AOAC_DEV_I3C1);
	FCH_DEV_ENABLE(i3c_2, FCH_AOAC_DEV_I3C2);
	FCH_DEV_ENABLE(i3c_3, FCH_AOAC_DEV_I3C3);
	FCH_DEV_ENABLE(hfp, 29);
	FCH_DEV_ENABLE(hid_2, 30);
	FCH_DEV_ENABLE(hid, 31);
}

void setup_opensil(void)
{
	SIL_CONTEXT SilContext;
	const size_t mem_req = xSimQueryMemoryRequirements();
	void *buf = cbmem_add(CBMEM_ID_AMD_OPENSIL, mem_req);

	if (!buf)
		die("Could not allocate OpenSIL memory in cbmem!");

	memset(buf, 0, mem_req);

	SilContext.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS;
	SilContext.SilMemBaseAddress = (uintptr_t)buf;

	/* We run all openSIL timepoints in the same stage so using TP1 as argument is fine. */
	const SIL_STATUS assign_mem_ret = xSimAssignMemoryTp1(&SilContext, mem_req);
	SIL_STATUS_report("xSimAssignMemory", assign_mem_ret);

	setup_rc_manager_default(&SilContext);
	configure_usb(&SilContext);
	configure_ccx(&SilContext);
	configure_fch_acpi(&SilContext);
}

static void opensil_entry(SIL_TIMEPOINT timepoint)
{
	SIL_STATUS ret;
	SIL_TIMEPOINT tp = (uintptr_t)timepoint;
	SIL_CONTEXT SilContext = {
		.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS,
		.SilMemBaseAddress = (uintptr_t)cbmem_find(CBMEM_ID_AMD_OPENSIL)
	};

	if (!SilContext.SilMemBaseAddress)
		die("OpenSIL cbmem memory not found!\n");

	switch (tp) {
	case SIL_TP1:
		ret = InitializeAMDSiTp1(&SilContext);
		break;
	case SIL_TP2:
		ret = InitializeAMDSiTp2(&SilContext);
		break;
	case SIL_TP3:
		ret = InitializeAMDSiTp3(&SilContext);
		break;
	default:
		printk(BIOS_ERR, "Unknown openSIL timepoint\n");
		return;
	}
	char opensil_function[20];
	snprintf(opensil_function, sizeof(opensil_function), "InitializeAMDSiTp%d", tp + 1);
	SIL_STATUS_report(opensil_function, ret);
	if (ret == SilResetRequestColdImm || ret == SilResetRequestColdDef) {
		printk(BIOS_INFO, "openSIL requested a cold reset");
		do_cold_reset();
	} else if (ret == SilResetRequestWarmImm || ret == SilResetRequestWarmDef) {
		printk(BIOS_INFO, "openSIL requested a warm reset");
		do_warm_reset();
	}
}

void opensil_xSIM_timepoint_1(void)
{
	opensil_entry(SIL_TP1);
}

void opensil_xSIM_timepoint_2(void)
{
	opensil_entry(SIL_TP2);
}

void opensil_xSIM_timepoint_3(void)
{
	opensil_entry(SIL_TP3);
}

/* TODO: also call timepoints 2 and 3 from coreboot. Are they NOOP? */
