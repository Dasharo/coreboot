/* SPDX-License-Identifier: GPL-2.0-only */

#include <CCX/CcxClass-api.h>
#include <CCX/Common/CcxApic.h>
#include <FCH/Common/FchCommon.h>
#include <RcMgr/DfX/RcManager-api.h>
#include <amdblocks/reset.h>
#include <bootstate.h>
#include <cbmem.h>
#include <cpu/amd/microcode.h>
#include <cpu/cpu.h>
#include <device/device.h>
#include <soc/amd/phoenix/chip.h>
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
	rc_mgr_input_block->BottomMmioReservedForPrimaryRb = 4ull * GiB - 32 * MiB;
	rc_mgr_input_block->MmioSizePerRbForNonPciDevice = 16 * MiB;
	/* MmioAbove4GLimit will be adjusted down in openSIL */
	rc_mgr_input_block->MmioAbove4GLimit = POWER_OF_2(cpu_phys_address_size());
	rc_mgr_input_block->Above4GMmioSizePerRbForNonPciDevice = 0;
}

#define NUM_XHCI_CONTROLLERS 4
static void configure_usb(SIL_CONTEXT *SilContext)
{
	struct device *usb_ctrlr[NUM_XHCI_CONTROLLERS] = {
		DEV_PTR(xhci_0),
		DEV_PTR(xhci_1),
		DEV_PTR(usb4_xhci_0),
		DEV_PTR(usb4_xhci_1)
	};
	FCHUSB_INPUT_BLK *fch_usb_data = SilFindStructure(SilContext, SilId_FchUsb, 0);

	if (!fch_usb_data)
		return;

	fch_usb_data->Xhci0Enable = usb_ctrlr[0] && usb_ctrlr[0]->enabled;
	fch_usb_data->Xhci1Enable = usb_ctrlr[1] && usb_ctrlr[1]->enabled;
	fch_usb_data->Xhci2Enable = usb_ctrlr[2] && usb_ctrlr[2]->enabled;
	fch_usb_data->Xhci3Enable = usb_ctrlr[3] && usb_ctrlr[3]->enabled;

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


void setup_opensil(void)
{
	SIL_CONTEXT SilContext;
	const size_t mem_req = xSimQueryMemoryRequirements();
	void *buf = cbmem_add(CBMEM_ID_AMD_OPENSIL, mem_req);

	if (!buf)
		die("Could not allocate OpenSIL memory in cbmem!");

	SilContext.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS;
	SilContext.SilMemBaseAddress = (uintptr_t)buf;

	/* We run all openSIL timepoints in the same stage so using TP1 as argument is fine. */
	const SIL_STATUS assign_mem_ret = xSimAssignMemoryTp1(&SilContext, mem_req);
	SIL_STATUS_report("xSimAssignMemory", assign_mem_ret);


	setup_rc_manager_default(&SilContext);
	configure_usb(&SilContext);
	configure_ccx(&SilContext);
}

static void opensil_entry(SIL_TIMEPOINT timepoint)
{
	SIL_STATUS ret;
	SIL_TIMEPOINT tp = (uintptr_t)timepoint;
	SIL_CONTEXT SilContext;
	void *buf = cbmem_find(CBMEM_ID_AMD_OPENSIL);

	if (!buf)
		die("OpenSIL cbmem memory not found!\n");

	SilContext.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS;
	SilContext.SilMemBaseAddress = (uintptr_t)buf;

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
