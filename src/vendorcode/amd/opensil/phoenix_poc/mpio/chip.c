/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbmem.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <GFX/GfxClass-api.h>
#include <Mpio/MpioClass-api.h>
#include <Mpio/Phx/MpioPhxData.h>
#include <Nbio/NbioClass-api.h>
#include <RcMgr/DfX/RcManager-api.h>
#include <soc/iomap.h>
#include <vendorcode/amd/opensil/opensil.h>
#include <xSIM-api.h>
#include <static.h>

#include "chip.h"
#include "../prom21/prom21.h"

MPIO_DDI_DESCRIPTOR ddi_descriptor_list[MAX_DDI_PORTS];

static void mpio_params_config(SIL_CONTEXT *SilContext)
{
	MPIOCLASS_COMMON_INPUT_BLK *mpio_data = SilFindStructure(SilContext, SilId_MpioClass, 0);
	MPIOCLASS_PHX_INPUT_BLK *phx_data = SilFindStructure(SilContext, SilId_MpioClass, 1);
	struct device *gnb = DEV_PTR(gnb);
	struct device *iommu = DEV_PTR(iommu);
	struct device *psp = DEV_PTR(crypto);
	struct device *nbif = pcidev_on_root(8, 0);
	struct device *acp = DEV_PTR(acp);
	struct device *hda = DEV_PTR(hda);
	struct device *mp2 = DEV_PTR(mp2);
	struct device *gfx = DEV_PTR(gfx);
	struct device *gfx_hda = DEV_PTR(gfx_hda);
	struct device *nbifrc = DEV_PTR(gpp_bridge_a);

	phx_data->AcpController = is_dev_enabled(acp);
	phx_data->CfgHdAudioEnable = is_dev_enabled(hda);
	phx_data->CfgSensorHubEnable = is_dev_enabled(mp2);

	if (acp)
		phx_data->CfgAcpSsid = acp->subsystem_vendor |
				       ((uint32_t)acp->subsystem_device << 16);
	if (gfx)
		phx_data->AmdCfgGnbIGPUSSID = gfx->subsystem_vendor |
					      ((uint32_t)gfx->subsystem_device << 16);
	if (gfx_hda)
		phx_data->AmdCfgGnbIGPUAudioSSID = gfx_hda->subsystem_vendor |
						   ((uint32_t)gfx_hda->subsystem_device << 16);
	if (nbifrc)
		phx_data->CfgNbifRCSsid = nbifrc->subsystem_vendor |
					  ((uint32_t)nbifrc->subsystem_device << 16);
	if (gnb)
		mpio_data->CfgNbioSsid   = gnb->subsystem_vendor |
					  ((uint32_t)gnb->subsystem_device << 16);
	if (iommu)
		mpio_data->CfgIommuSsid  = iommu->subsystem_vendor |
					  ((uint32_t)iommu->subsystem_device << 16);
	if (psp)
		mpio_data->CfgPspccpSsid = psp->subsystem_vendor |
					  ((uint32_t)psp->subsystem_device << 16);
	if (nbif)
		mpio_data->CfgNbifF0Ssid = nbif->subsystem_vendor |
					  ((uint32_t)nbif->subsystem_device << 16);

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
	mpio_data->CfgNtbccpSsid                       = 0;
	mpio_data->CfgNtbSsid                          = 0;
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
	ddi_descriptor_list[0].Flags = DESCRIPTOR_TERMINATE_LIST;
	mpio_data->PcieTopologyData.PlatformData[0].DdiLinkList = ddi_descriptor_list;
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
	input->IoApicMMIOAddressReservedEnable = false;
	input->CfgGnbIoapicAddress        = GNB_IO_APIC_ADDR;
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
#define MPIO_ENGINE_DATA_INITIALIZER(mType, mStartLane, mEndLane, mHotplug, mGpioGroupId) \
	{ \
		.EngineType = mType, \
		.HotPluggable = mHotplug, \
		.StartLane = mStartLane, \
		.EndLane = mEndLane, \
		.GpioGroupId = mGpioGroupId, \
	}
#endif
#ifndef MPIO_PORT_DATA_INITIALIZER_PCIE
#define MPIO_PORT_DATA_INITIALIZER_PCIE(mPortPresent, mDevAddress, mDevFunction, mHotplug, \
					mMaxLinkSpeed, mMaxLinkCap, mAspm, mAspmL1_1, \
					mAspmL1_2, mClkPmSupport) \
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
#ifndef MPIO_DDI_DATA_INITIALIZER
#define MPIO_DDI_DATA_INITIALIZER(mConnectorType, mAuxIndex, mHdpIndex) \
	{ \
		.ConnectorType = mConnectorType, \
		.AuxIndex = mAuxIndex, \
		.HdpIndex = mHdpIndex, \
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

	const uint32_t domain = dev_get_domain_id(dev);
	const uint32_t devfn = dev->path.pci.devfn;
	const struct drivers_amd_opensil_mpio_config *const config = dev->chip_info;
	static int ddi_port = 0;
	if (is_pci(dev))
		printk(BIOS_DEBUG, "Setting MPIO port for domain 0x%x, PCI %d:%d\n",
		       domain, PCI_SLOT(devfn), PCI_FUNC(devfn));
	else if (config->type == IFTYPE_DDI)
		printk(BIOS_DEBUG, "Setting DDI port %u\n", ddi_port);

	if (config->type == IFTYPE_UNUSED) {
		if (is_dev_enabled(dev)) {
			printk(BIOS_WARNING, "Unused MPIO chip, disabling PCI device.\n");
			dev->enabled = false;
		} else {
			printk(BIOS_DEBUG, "Unused MPIO chip, skipping.\n");
		}
		return;
	}

	if (config->type == IFTYPE_PCIE) {
		static uint32_t slot_num;
		static int mpio_port = 0;

		MPIO_PORT_DESCRIPTOR port = { .Flags = DESCRIPTOR_TERMINATE_LIST };
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

		if (dev->subsystem_vendor && dev->subsystem_device) {
			mpio_data->AmdPcieSubsystemVendorID = dev->subsystem_vendor;
			mpio_data->AmdPcieSubsystemDeviceID = dev->subsystem_device;
		}

		if (CONFIG(DRIVERS_AMD_PROMONTORY21) && config->sb_link)
			opensil_promontory21_config(&SilContext, dev);

		port.Port.AlwaysExpose = 1;
		port.Port.SlotNum = ++slot_num;
		mpio_data->PcieTopologyData.PortList[mpio_port] = port;
		/* Update TERMINATE list */
		if (mpio_port > 0)
			mpio_data->PcieTopologyData.PortList[mpio_port - 1].Flags = 0;
		mpio_port++;
	} else if (config->type == IFTYPE_DDI) {
		MPIO_DDI_DESCRIPTOR ddi = { .Flags = DESCRIPTOR_TERMINATE_LIST };
		const MPIO_DDI_DATA ddi_data = MPIO_DDI_DATA_INITIALIZER(config->ddi_connector,
									 config->aux,
									 config->hdp);

		if (ddi_port >= MAX_DDI_PORTS) {
			printk(BIOS_WARNING, "Exceeded maximum number of DDI ports.\n");
			return;
		}

		ddi.Ddi = ddi_data;
		ddi_descriptor_list[ddi_port] = ddi;
		/* Update TERMINATE list */
		if (ddi_port > 0)
			ddi_descriptor_list[ddi_port - 1].Flags = 0;
		ddi_port++;
	}
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
