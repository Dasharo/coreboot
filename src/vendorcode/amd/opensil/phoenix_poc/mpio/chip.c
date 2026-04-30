/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbmem.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <GFX/GfxClass-api.h>
#include <Mpio/MpioClass-api.h>
#include <Nbio/NbioClass-api.h>
#include <RcMgr/DfX/RcManager-api.h>
#include <vendorcode/amd/opensil/opensil.h>
#include <xSIM-api.h>
#include <static.h>

#include "chip.h"

static void mpio_params_config(SIL_CONTEXT *SilContext)
{
	MPIOCLASS_COMMON_INPUT_BLK *mpio_data = SilFindStructure(SilContext, SilId_MpioClass, 0);
	mpio_data->CfgDxioClockGating                  = 1;
	mpio_data->PcieDxioTimingControlEnable         = 0;
	mpio_data->PCIELinkReceiverDetectionPolling    = 0;
	mpio_data->PCIELinkResetToTrainingTime         = 0;
	mpio_data->PCIELinkL0Polling                   = 0;
	mpio_data->PCIeExactMatchEnable                = 0;
	mpio_data->DxioPhyValid                        = 1;
	mpio_data->DxioPhyProgramming                  = 1;
	mpio_data->CfgSkipPspMessage                   = 1;
	mpio_data->DxioSaveRestoreModes                = 0xff;
	mpio_data->AmdAllowCompliance                  = 0;
	mpio_data->AmdAllowCompliance                  = 0xff;
	mpio_data->SrisEnableMode                      = 0xff;
	mpio_data->SrisSkipInterval                    = 0;
	mpio_data->SrisSkpIntervalSel                  = 1;
	mpio_data->SrisCfgType                         = 0;
	mpio_data->SrisAutoDetectMode                  = 0xff;
	mpio_data->SrisAutodetectFactor                = 0;
	mpio_data->SrisLowerSkpOsGenSup                = 0;
	mpio_data->SrisLowerSkpOsRcvSup                = 0;
	mpio_data->AmdCxlOnAllPorts                    = 1;
	mpio_data->CxlCorrectableErrorLogging          = 1;
	mpio_data->CxlUnCorrectableErrorLogging        = 1;
	  // This is also available in Nbio. How to handle duplicate entries?
	mpio_data->CfgAEREnable                        = 1;
	mpio_data->CfgMcCapEnable                      = 0;
	mpio_data->CfgRcvErrEnable                     = 0;
	mpio_data->EarlyBmcLinkTraining                = 1;
	mpio_data->SurpriseDownFeature                 = 1;
	mpio_data->LcMultAutoSpdChgOnLastRateEnable    = 0;
	mpio_data->AmdRxMarginEnabled                  = 1;
	mpio_data->CfgPcieCVTestWA                     = 1;
	mpio_data->CfgPcieAriSupport                   = 1;
	mpio_data->CfgNbioCTOtoSC                      = 0;
	mpio_data->CfgNbioCTOIgnoreError               = 1;
	mpio_data->CfgNbioSsid                         = 0;
	mpio_data->CfgIommuSsid                        = 0;
	mpio_data->CfgPspccpSsid                       = 0;
	mpio_data->CfgNtbccpSsid                       = 0;
	mpio_data->CfgNbifF0Ssid                       = 0;
	mpio_data->CfgNtbSsid                          = 0;
	mpio_data->AmdPcieSubsystemDeviceID            = 0x1453;
	mpio_data->AmdPcieSubsystemVendorID            = 0x1022;
	mpio_data->GppAtomicOps                        = 1;
	mpio_data->GfxAtomicOps                        = 1;
	mpio_data->AmdNbioReportEdbErrors              = 0;
	mpio_data->OpnSpare                            = 0;
	mpio_data->AmdPreSilCtrl0                      = 0;
	mpio_data->MPIOAncDataSupport                  = 1;
	mpio_data->AfterResetDelay                     = 0;
	mpio_data->CfgEarlyLink                        = 0;
	mpio_data->AmdCfgExposeUnusedPciePorts         = 1; // Show all ports
	mpio_data->CfgForcePcieGenSpeed                = 0;
	mpio_data->PcieLinkComplianceModeAllPorts      = 0;
	mpio_data->AmdMCTPEnable                       = 0;
	mpio_data->SbrBrokenLaneAvoidanceSup           = 1;
	mpio_data->AutoFullMarginSup                   = 1;
	  // A getter and setter, both are needed for this PCD.
	mpio_data->AmdPciePresetMask8GtAllPort         = 0xffffffff;
	  // A getter and setter, both are needed for this PCD.
	mpio_data->AmdPciePresetMask16GtAllPort        = 0xffffffff;
	  // A getter and setter, both are needed for this PCD.
	mpio_data->AmdPciePresetMask32GtAllPort        = 0xffffffff;
	mpio_data->PcieLinkAspmAllPort                 = 0xff;

	mpio_data->SyncHeaderByPass                    = 1;
	mpio_data->CxlTempGen5AdvertAltPtcl            = 0;

	/* TODO handle this differently on multisocket */
	mpio_data->PcieTopologyData.PlatformData[0].Flags = DESCRIPTOR_TERMINATE_LIST;
	mpio_data->PcieTopologyData.PlatformData[0].PciePortList = mpio_data->PcieTopologyData.PortList;
}

WEAK_DEV_PTR(usb4_router_0);
WEAK_DEV_PTR(usb4_pcie_bridge_0);
WEAK_DEV_PTR(usb4_router_1);
WEAK_DEV_PTR(usb4_pcie_bridge_1);

static void nbio_params_config(SIL_CONTEXT *SilContext)
{
	NBIOCLASS_DATA_BLOCK *nbio_data = SilFindStructure(SilContext, SilId_NbioClass, 0);
	GFXCLASS_INPUT_BLK *gfx_data = SilFindStructure(SilContext, SilId_GfxClass, 0);
	NBIO_CONFIG_DATA *input = &nbio_data->NbioConfigData;	
	input->EsmEnableAllRootPorts      = false;
	input->EsmTargetSpeed             = 16;
	input->CfgRxMarginPersistenceMode = 1;
	input->SevSnpSupport              = false;
	input->Usb4Rt0En                  = is_dev_enabled(DEV_PTR(usb4_router_0));
	input->Usb4Rt0PcieTnlEn           = is_dev_enabled(DEV_PTR(usb4_pcie_bridge_0));
	input->Usb4Rt1En                  = is_dev_enabled(DEV_PTR(usb4_router_1));
	input->Usb4Rt1PcieTnlEn           = is_dev_enabled(DEV_PTR(usb4_pcie_bridge_1));
	gfx_data->Usb4Rt0En               = input->Usb4Rt0En;
	gfx_data->Usb4Rt1En               = input->Usb4Rt1En;
}

#ifndef MPIO_ENGINE_DATA_INITIALIZER
#define  MPIO_ENGINE_DATA_INITIALIZER(mType, mStartLane, mEndLane, mHotplug, mGpioGroupId) \
        { .EngineType = mType, \
          .HotPluggable = mHotplug, \
          .StartLane = mStartLane, \
          .EndLane = mEndLane, \
          .GpioGroupId = mGpioGroupId, \
        }
#endif
#ifndef MPIO_PORT_DATA_INITIALIZER_PCIE
#define  MPIO_PORT_DATA_INITIALIZER_PCIE(mPortPresent, mDevAddress, mDevFunction, mHotplug, mMaxLinkSpeed, \
          mMaxLinkCap, mAspm, mAspmL1_1, mAspmL1_2, mClkPmSupport) \
        { \
          .PortPresent = mPortPresent, \
          .DeviceNumber = mDevAddress, \
          .FunctionNumber = mDevFunction, \
          .LinkSpeedCapability = mMaxLinkSpeed, \
          .LinkAspm = mAspm, \
          .LinkAspmL1_1 = mAspmL1_1, \
          .LinkAspmL1_2 = mAspmL1_2, \
          .LinkHotplug = mHotplug, \
          .MiscControls = { \
            .LinkSafeMode = mMaxLinkCap, \
            .ClkPmSupport = mClkPmSupport, \
            .TurnOffUnusedLanes = 1, \
          }, \
        }
#endif

void opensil_mpio_per_device_config(struct device *dev)
{
	/* Cache *mpio_data from SilFindStructure */
	static MPIOCLASS_COMMON_INPUT_BLK *mpio_data = NULL;
	SIL_CONTEXT SilContext = {
		.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS,
		.SilMemBaseAddress = (uintptr_t)cbmem_find(CBMEM_ID_AMD_OPENSIL)
	};

	if (mpio_data == NULL) {
		mpio_data = SilFindStructure(&SilContext, SilId_MpioClass, 0);
		if (!mpio_data) {
			printk(BIOS_ERR, "Could not find OpenSIL MPIO data\n");
			return;
		}
	}

	static uint32_t slot_num;
	const uint32_t domain = dev_get_domain_id(dev);
	const uint32_t devfn = dev->path.pci.devfn;
	const struct drivers_amd_opensil_mpio_config *const config = dev->chip_info;
	printk(BIOS_DEBUG, "Setting MPIO port for domain 0x%x, PCI %d:%d\n",
	       domain, PCI_SLOT(devfn), PCI_FUNC(devfn));

	if (config->type == IFTYPE_UNUSED) {
		if (is_dev_enabled(dev)) {
			printk(BIOS_WARNING, "Unused MPIO chip, disabling PCI device.\n");
			dev->enabled = false;
		} else {
			printk(BIOS_DEBUG, "Unused MPIO chip, skipping.\n");
		}
		return;
	}

	static int mpio_port = 0;
	MPIO_PORT_DESCRIPTOR port = { .Flags = DESCRIPTOR_TERMINATE_LIST };
	if (config->type == IFTYPE_PCIE) {
		const MPIO_ENGINE_DATA engine_data =
			MPIO_ENGINE_DATA_INITIALIZER(MpioPcieEngine,
						     config->start_lane, config->end_lane,
						     config->hotplug == HotplugDisabled ? 0 : 1,
						     config->gpio_group);
		port.EngineData = engine_data;
		const MPIO_PORT_DATA port_data =
			MPIO_PORT_DATA_INITIALIZER_PCIE(is_dev_enabled(dev) ?
								MpioPortEnabled : MpioPortDisabled,
							PCI_SLOT(devfn),
							PCI_FUNC(devfn),
							config->hotplug,
							config->speed,
							0, // No backup PCIe speed
							config->aspm,
							config->aspm_l1_1,
							config->aspm_l1_2,
							config->clock_pm);

		port.Port = port_data;
		port.Port.MiscControls.SbLink = config->sb_link;
	}
	port.Port.AlwaysExpose = 1;
	port.Port.SlotNum = ++slot_num;
	mpio_data->PcieTopologyData.PortList[mpio_port] = port;
	/* Update TERMINATE list */
	if (mpio_port > 0)
		mpio_data->PcieTopologyData.PortList[mpio_port - 1].Flags = 0;
	mpio_port++;
}

void opensil_mpio_global_config(void)
{
	SIL_CONTEXT SilContext = {
		.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS,
		.SilMemBaseAddress = (uintptr_t)cbmem_find(CBMEM_ID_AMD_OPENSIL)
	};

	mpio_params_config(&SilContext);
	nbio_params_config(&SilContext);
}
