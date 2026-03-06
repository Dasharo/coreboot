/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <cbmem.h>
#include <Sil-api.h>
#include <SilCommon.h>
#include <xSIM-api.h>
#include <FCH/Common/FchCommon.h>

#include "../opensil.h"

void opensil_fill_fadt(acpi_fadt_t *fadt)
{
	SIL_CONTEXT SilContext = {
		.ApobBaseAddress = CONFIG_PSP_APOB_DRAM_ADDRESS,
		.SilMemBaseAddress = (uintptr_t)cbmem_find(CBMEM_ID_AMD_OPENSIL)
	};

	FCHCLASS_INPUT_BLK *blk = SilFindStructure(&SilContext, SilId_FchClass,  0);

	fadt->pm1a_evt_blk = blk->FchBldCfg.CfgAcpiPm1EvtBlkAddr;
	fadt->pm1a_cnt_blk = blk->FchBldCfg.CfgAcpiPm1CntBlkAddr;
	fadt->pm_tmr_blk = blk->FchBldCfg.CfgAcpiPmTmrBlkAddr;
	fadt->gpe0_blk = blk->FchBldCfg.CfgAcpiGpe0BlkAddr;
}

unsigned long add_opensil_acpi_table(unsigned long current, acpi_rsdp_t *rsdp)
{
	return current;
}
