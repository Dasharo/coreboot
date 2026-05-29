/* SPDX-License-Identifier: GPL-2.0-only */

/* opensil_config.h needed for PROJ_MAX_* defines */
#include <opensil_config.h>
#include <CCX/CcxClass-api.h>
#include <CCX/Common/CcxApic.h>
#include <DF/DfClass-api.h>
#include <FCH/FchClass-api.h>
#include <FCH/FchHwAcpi-api.h>
#include <FCH/FchIsa-api.h>
#include <FCH/Common/FchCore/FchSata/FchSataBlk.h>
#include <Mpio/MpioClass-api.h>
#include <RcMgr/DfX/RcManager4-api.h>
#include <Sdxi/SdxiClass-api.h>
#include <SMU/SmuClass-api.h>
#include <amdblocks/aoac.h>
#include <amdblocks/reset.h>
#include <bootstate.h>
#include <cbmem.h>
#include <cpu/amd/microcode.h>
#include <cpu/cpu.h>
#include <cpu/x86/smm.h>
#include <dasharo/options.h>
#include <device/device.h>
#include <soc/aoac_defs.h>
#include <soc/iomap.h>
#include <soc/soc_chip.h>
#include <static.h>
#include <stdio.h>
#include <xSIM-api.h>

#include "opensil_console.h"
#include "../opensil.h"


#define TURIN_USB_STRUCT_MAJOR_VERSION	0xd
#define TURIN_USB_STRUCT_MINOR_VERSION	0x13

#define TURIN_USB_PORT_PER_CONTROLLER		2
#define TURIN_USB_CONTROLLERS_PER_SOCKET	2
#define TURIN_NUM_USB_PORTS	\
	(TURIN_USB_PORT_PER_CONTROLLER * TURIN_USB_CONTROLLERS_PER_SOCKET)

/*
 * Structures copied from Genoa POC code, because Turin POC does not define these.
 * It appears the USB structures are identical for Turin and Genoa.
 */
typedef struct {
	uint8_t COMPDSTUNE;
	uint8_t SQRXTUNE;
	uint8_t TXFSLSTUNE;
	uint8_t TXPREEMPAMPTUNE;
	uint8_t TXPREEMPPULSETUNE;
	uint8_t TXRISETUNE;
	uint8_t TXVREFTUNE;
	uint8_t TXHSXVTUNE;
	uint8_t TXRESTUNE;
} __packed FCH_USB20_PHY;

typedef struct {
	uint8_t RX_ANA_IQ_PHASE_ADJUST;
	uint8_t RX_EQ_DELTA_IQ_OVRD_EN;
	uint8_t RX_EQ_DELTA_IQ_OVRD_VAL;
	uint8_t RX_IQ_PHASE_ADJUST;
	uint8_t TX_VBOOST_LVL_EN;
	uint8_t TX_VBOOST_LVL;
	uint8_t RX_VREF_CTRL_EN;
	uint8_t RX_VREF_CTRL;
	uint8_t TX_VBOOST_LVL_EN_X;
	uint8_t TX_VBOOST_LVL_X;
	uint8_t RX_VREF_CTRL_EN_X;
	uint8_t RX_VREF_CTRL_X;
} __packed FCH_USB31_PHY;

typedef struct {
	uint8_t		Version_Major;
	uint8_t		Version_Minor;
	uint8_t		TableLength;
	uint8_t		Reserved0;
	uint8_t		Usb20PhyEnable;
	FCH_USB20_PHY	Usb20PhyPort[TURIN_NUM_USB_PORTS];
	uint8_t		Reserved1;
	uint8_t		S1Usb20PhyEnable;
	FCH_USB20_PHY	S1Usb20PhyPort[TURIN_NUM_USB_PORTS];
	uint8_t		Usb31PhyEnable;
	FCH_USB31_PHY	Usb31PhyPort[TURIN_NUM_USB_PORTS];
	uint8_t		S1Usb31PhyEnable;
	FCH_USB31_PHY	S1Usb31PhyPort[TURIN_NUM_USB_PORTS];
} __packed FCH_USB_OEM_PLATFORM_TABLE;

static FCH_USB_OEM_PLATFORM_TABLE usb_config = { 0 };

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
		if (errors[i].status == status) {
			error_string = errors[i].string;
			break;
		}
	}
	printk(log_level, "%s returned %d (%s)\n", function, status, error_string);
}

static void setup_rc_manager_default(void)
{
	DFX_RCMGR_INPUT_BLK *rc_mgr_input_block = SilFindStructure(SilId_RcManager, 0);

	/* Let openSIL distribute the resources to the different PCI roots */
	rc_mgr_input_block->SetRcBasedOnNv = false;

	/* Currently 1P is the only supported configuration */
	rc_mgr_input_block->SocketNumber = 1;
	rc_mgr_input_block->RbsPerSocket = 8; /* PCI root bridges per socket */
	rc_mgr_input_block->McptEnable = true;
	rc_mgr_input_block->PciExpressBaseAddress = CONFIG_ECAM_MMCONF_BASE_ADDRESS;
	rc_mgr_input_block->BottomMmioReservedForPrimaryRb = 4ull * GiB - 32 * MiB;
	rc_mgr_input_block->MmioSizePerRbForNonPciDevice = 16 * MiB;
	/*
	 * Turin has 52 bits of available physical address space without SME
	 * enabled. To avoid using 5-level paging (which we do not support yet
	 * in coreboot and UEFI payload) required to cover MMIO above 48 bits
	 * of address space (since 4-level paging can cover only 48 bits of
	 * physical address space), limit the MMIO to maximum of 48 bits.
	 * MmioAbove4GLimit will be adjusted down in openSIL if needed.
	 */
	rc_mgr_input_block->MmioAbove4GLimit = POWER_OF_2(MIN(48, cpu_phys_address_size()));
	rc_mgr_input_block->Above4GMmioSizePerRbForNonPciDevice = 0;

	rc_mgr_input_block->AmdSmee = CONFIG(AMD_SME_ENABLE);
}

static void setup_data_fabric_default(void)
{
	DFCLASS_INPUT_BLK *df_input_block = SilFindStructure(SilId_DfClass, 0);

	if (!df_input_block) {
		printk(BIOS_ERR, "OpenSIL: Data Fabric block not found\n");
		return;
	}

	df_input_block->AmdPciExpressBaseAddress = CONFIG_ECAM_MMCONF_BASE_ADDRESS;
}

#define NUM_XHCI_CONTROLLERS 2
static void configure_usb(void)
{
	const struct soc_amd_turin_poc_config *soc_config = config_of_soc();
	const struct soc_usb_config *usb = &soc_config->usb;

	FCHUSB_INPUT_BLK *fch_usb_data = SilFindStructure(SilId_FchUsb, 0);

	fch_usb_data->Xhci0Enable = is_dev_enabled(DEV_PTR(xhci_0));
	fch_usb_data->Xhci1Enable = is_dev_enabled(DEV_PTR(xhci_1));
	/*
	 * There's no XHCI2/3 on this SoC, but these refer to XHCI on 2nd socket.
	 * But we do not support two socket platforms yet, so force disable.
	 */
	fch_usb_data->Xhci2Enable = false;
	fch_usb_data->Xhci3Enable = false;
	for (int i = 0; i < NUM_XHCI_CONTROLLERS; i++) {
		memcpy(&fch_usb_data->XhciOCpinSelect[i].Usb20OcPin, &usb->usb2_oc_pins[i],
		       sizeof(fch_usb_data->XhciOCpinSelect[i].Usb20OcPin));
		memcpy(&fch_usb_data->XhciOCpinSelect[i].Usb31OcPin, &usb->usb3_oc_pins[i],
		       sizeof(fch_usb_data->XhciOCpinSelect[i].Usb31OcPin));
	}
	fch_usb_data->XhciOcPolarityCfgLow = usb->polarity_cfg_low;
	fch_usb_data->Usb3PortForceGen1 = usb->usb3_force_gen1.raw;

	memset(&usb_config, 0, sizeof(usb_config));

	usb_config.Version_Major = TURIN_USB_STRUCT_MAJOR_VERSION;
	usb_config.Version_Minor = TURIN_USB_STRUCT_MINOR_VERSION;
	usb_config.TableLength = sizeof(FCH_USB_OEM_PLATFORM_TABLE);

	usb_config.Usb31PhyEnable = usb->usb31_phy_enable;
	memcpy(usb_config.Usb31PhyPort, usb->usb31_phy, sizeof(usb_config.Usb31PhyPort));

	usb_config.S1Usb31PhyEnable = usb->s1_usb31_phy_enable;
	memcpy(usb_config.S1Usb31PhyPort, usb->s1_usb31_phy, sizeof(usb_config.S1Usb31PhyPort));

	fch_usb_data->OemUsbConfigurationTable = (uintptr_t)&usb_config;
}

#define NUM_SATA_CONTROLLERS 4
static void configure_sata(void)
{
	FCHSATA_INPUT_BLK *fch_sata_data = SilFindStructure(SilId_FchSata, 0);
	const struct soc_amd_turin_poc_config *soc_config = config_of_soc();
	const struct soc_sata_config *sata = &soc_config->sata;

	struct device *sata_devs[NUM_SATA_CONTROLLERS] = {
		DEV_PTR(sata_7_0),
		DEV_PTR(sata_7_1),
		DEV_PTR(sata_2_0),
		DEV_PTR(sata_2_1)
	};

	for (int i = 0; i < NUM_SATA_CONTROLLERS; i++) {
		fch_sata_data[i].SataEnable = is_dev_enabled(sata_devs[i]);
		fch_sata_data[i].SataAhciSsid = (sata_devs[i])->subsystem_vendor |
						((uint32_t)((sata_devs[i])->subsystem_device) << 16);
		fch_sata_data[i].SataSetMaxGen2 = false;
		fch_sata_data[i].SataMsiEnable = true;
		fch_sata_data[i].SataRasSupport = true;
		fch_sata_data[i].SataStaggeredSpinupEnable = true;
		fch_sata_data[i].SataRxPolarity = sata->rx_polarity[i].raw;
		fch_sata_data[i].SataPortPower = sata->shutdown_ports[i].raw;
		fch_sata_data[i].SataEspPort = sata->enable_esata[i].raw;
		fch_sata_data[i].SataSgpio0 =  sata->sgpio0_enable[i];
		fch_sata_data[i].SataSgpio1 =  sata->sgpio1_enable[i];
	}
}

static void configure_fch_isa(void)
{
	FCHISA_INPUT_BLK *fch_isa_data = SilFindStructure(SilId_FchIsa, 0);
	struct device *lpc = DEV_PTR(lpc_bridge);

	fch_isa_data->LpcConfig.LpcSsid = lpc->subsystem_vendor |
					  ((uint32_t)lpc->subsystem_device << 16);

	/* Keep defaults that were set by either EFS or coreboot */
	fch_isa_data->SpiConfig.SpiSpeed = 0;
	fch_isa_data->SpiConfig.SpiTpmSpeed = 0;
	fch_isa_data->SpiConfig.WriteSpeed = 0;
}

#define FCH_DEV_ENABLE(dev, aoac_bit) \
	do { \
		if (is_dev_enabled(DEV_PTR(dev))) { \
			fch_data->FchRunTime.FchDeviceEnableMap |= (1ul << (aoac_bit)); \
			if (!is_aoac_device_enabled(aoac_bit)) { \
				printk(BIOS_DEBUG, "Enabling " #dev "\n"); \
				power_on_aoac_device(aoac_bit); \
				wait_for_aoac_enabled(aoac_bit); \
			} \
		} \
	} while (0)
	
static void configure_fch_acpi(void)
{
	FCHHWACPI_INPUT_BLK *fch_hwacpi_data = SilFindStructure(SilId_FchHwAcpiP, 0);
	FCHCLASS_INPUT_BLK *fch_data = SilFindStructure(SilId_FchClass, 0);
	struct device *smb = DEV_PTR(smbus);
	uint8_t pwr_fail_state = dasharo_get_power_on_after_fail();

	fch_data->Smbus.SmbusSsid = smb->subsystem_vendor |
				    ((uint32_t)smb->subsystem_device << 16);

	fch_data->FchBldCfg.CfgSioPmeBaseAddress = 0;
	fch_data->FchBldCfg.CfgAcpiPm1EvtBlkAddr = ACPI_PM_EVT_BLK;
	fch_data->FchBldCfg.CfgAcpiPm1CntBlkAddr = ACPI_PM1_CNT_BLK;
	fch_data->FchBldCfg.CfgAcpiPmTmrBlkAddr = ACPI_PM_TMR_BLK;
	fch_data->FchBldCfg.CfgCpuControlBlkAddr = ACPI_CPU_CONTROL;
	fch_data->FchBldCfg.CfgAcpiGpe0BlkAddr = ACPI_GPE0_BLK;
	fch_data->FchBldCfg.CfgSmiCmdPortAddr = APM_CNT;

	fch_data->CfgIoApicIdPreDefEnable = true;
	fch_data->FchIoApicId = 128;

	fch_data->WdtEnable = false;

	/* Servers usually don't have KBC on SIO */
	fch_data->Misc.NoneSioKbcSupport = true;

	if (pwr_fail_state == 2)
		fch_hwacpi_data->PwrFailShadow = UsePrevious;
	else if (pwr_fail_state == 1)
		fch_hwacpi_data->PwrFailShadow = AlwaysOn;
	else
		fch_hwacpi_data->PwrFailShadow = AlwaysOff;

	fch_data->FchRunTime.FchDeviceEnableMap = (1 << FCH_AOAC_DEV_AMBA) |
						  (1 << FCH_AOAC_DEV_ESPI);
	FCH_DEV_ENABLE(i2c_0, FCH_AOAC_DEV_I2C0);
	FCH_DEV_ENABLE(i2c_1, FCH_AOAC_DEV_I2C1);
	FCH_DEV_ENABLE(i2c_2, FCH_AOAC_DEV_I2C2);
	FCH_DEV_ENABLE(i2c_3, FCH_AOAC_DEV_I2C3);
	FCH_DEV_ENABLE(i2c_4, FCH_AOAC_DEV_I2C4);
	FCH_DEV_ENABLE(i2c_5, FCH_AOAC_DEV_I2C5);
	FCH_DEV_ENABLE(uart_0, FCH_AOAC_DEV_UART0);
	FCH_DEV_ENABLE(uart_1, FCH_AOAC_DEV_UART1);
	FCH_DEV_ENABLE(uart_2, FCH_AOAC_DEV_UART2);
	FCH_DEV_ENABLE(i3c_0, FCH_AOAC_DEV_I3C0);
	FCH_DEV_ENABLE(i3c_1, FCH_AOAC_DEV_I3C1);
	FCH_DEV_ENABLE(i3c_2, FCH_AOAC_DEV_I3C2);
	FCH_DEV_ENABLE(i3c_3, FCH_AOAC_DEV_I3C3);
}

static void configure_sdxi(void)
{
	MPIOCLASS_INPUT_BLK *mpio_data = SilFindStructure(SilId_MpioClass, 0);
	SDXICLASS_INPUT_BLK *sdxi_data = SilFindStructure(SilId_SdxiClass, 0);

	mpio_data->AmdFabricSdxi = true;
	sdxi_data->AmdFabricSdxi = true;
}

static void configure_ccx(void)
{
	CCXCLASS_DATA_BLK *ccx_data = SilFindStructure(SilId_CcxClass, 0);
	UCODEPATCH_BIOSENTRYINFO *ucode_info;
	void *ucode;

	if (CONFIG(XAPIC_ONLY) || CONFIG(X2APIC_LATE_WORKAROUND))
		ccx_data->CcxInputBlock.AmdApicMode = xApicMode;
	else if (CONFIG(X2APIC_ONLY))
		ccx_data->CcxInputBlock.AmdApicMode = x2ApicMode;
	else
		ccx_data->CcxInputBlock.AmdApicMode = ApicAutoMode;

	ccx_data->CcxInputBlock.EnableAvx512 = 1;
	ccx_data->CcxInputBlock.EnableSvmX2AVIC = true;
	ccx_data->CcxInputBlock.EnableSvmAVIC = true;
	ccx_data->CcxInputBlock.AmdCStateIoBaseAddress = ACPI_CSTATE_CONTROL;

	ccx_data->CcxInputBlock.AmdSmee = CONFIG(AMD_SME_ENABLE);
	ccx_data->CcxInputBlock.AmdReserved = CONFIG(AMD_SME_HMK_ENABLE);
	ccx_data->CcxInputBlock.AmdVmplEnable = CONFIG(AMD_SEV_SNP_ENABLE);
	ccx_data->CcxInputBlock.AmdSnpMemCover = CONFIG(AMD_SEV_SNP_ENABLE);

	ucode = amd_microcode_find();
	if (!ucode) {
		printk(BIOS_ERR, "OpenSIL: CPU microcode not found\n");
		return;
	}

	ucode_info = &ccx_data->CcxInputBlock.UcodePatchEntryInfo;
	ucode_info->UcodePatchEntryAddress = (uint64_t)ucode;
}

static void configure_smu(void)
{
	SMUCLASS_INPUT_BLK *smu_data = SilFindStructure(SilId_SmuClass, 0);
	const struct soc_amd_turin_poc_config *soc_config = config_of_soc();

	smu_data->CfgPlatformTDP = soc_config->platform_TDP;
	smu_data->CfgPlatformPPT = soc_config->platform_PPT;
	smu_data->CfgPlatformTDC = soc_config->platform_TDC;
	smu_data->CfgPlatformEDC = soc_config->platform_EDC;

}

void setup_opensil(void)
{
	const SIL_STATUS debug_ret = SilDebugSetup(HostDebugService);
	SIL_STATUS_report("SilDebugSetup", debug_ret);
	const size_t mem_req = xSimQueryMemoryRequirements();
	void *buf = cbmem_add(CBMEM_ID_AMD_OPENSIL, mem_req);
	assert(buf);
	/* We run all openSIL timepoints in the same stage so using TP1 as argument is fine. */
	const SIL_STATUS assign_mem_ret = xSimAssignMemoryTp1(buf, mem_req);
	SIL_STATUS_report("xSimAssignMemory", assign_mem_ret);

	setup_rc_manager_default();
	setup_data_fabric_default();
	configure_ccx();
	configure_fch_isa();
	configure_fch_acpi();
	configure_usb();
	configure_sata();
	configure_sdxi();
	configure_smu();
}

static void opensil_entry(SIL_TIMEPOINT timepoint)
{
	SIL_STATUS ret;
	SIL_TIMEPOINT tp = (uintptr_t)timepoint;

	switch (tp) {
	case SIL_TP1:
		ret = InitializeAMDSiTp1();
		break;
	case SIL_TP2:
		ret = InitializeAMDSiTp2();
		break;
	case SIL_TP3:
		ret = InitializeAMDSiTp3();
		break;
	default:
		printk(BIOS_ERR, "Unknown openSIL timepoint\n");
		return;
	}
	char opensil_function[16];
	snprintf(opensil_function, sizeof(opensil_function), "InitializeSiTp%d", tp + 1);
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
